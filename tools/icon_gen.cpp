// icon_gen.cpp
// 太陽系アイコン (ICO) を生成するコマンドラインツール
// 使い方: icon_gen <output.ico>
//
// ICO フォーマット仕様:
//   ヘッダ (6 bytes) + ディレクトリ × N + 画像データ (PNG 形式)
// Windows Vista 以降は PNG 埋め込み ICO をサポート

#include "../icon_draw.h"    // drawPlanetIcon() を共有
#include <QCoreApplication>
#include <QBuffer>
#include <QDataStream>
#include <QFile>

// ---- ICO ファイルを書き出す ----
// sizes に指定したピクセルサイズで各画像を PNG として埋め込む
static bool writeIco(const QString &path, const QList<int> &sizes)
{
    // 各サイズの PNG データを生成
    QList<QByteArray> pngList;
    for (int sz : sizes) {
        QImage img = drawPlanetIcon(sz);
        QByteArray ba;
        QBuffer buf(&ba);
        buf.open(QIODevice::WriteOnly);
        img.save(&buf, "PNG");
        pngList.append(ba);
    }

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) return false;
    QDataStream ds(&f);
    ds.setByteOrder(QDataStream::LittleEndian);

    const int count = sizes.size();

    // --- ICONDIR ヘッダ ---
    ds << quint16(0);       // reserved
    ds << quint16(1);       // type = 1 (ICO)
    ds << quint16(count);   // image count

    // ディレクトリエントリのオフセット計算
    const int dirSize = 16 * count;   // 各エントリ 16 bytes
    const int headerSize = 6 + dirSize;

    // --- ICONDIRENTRY × count ---
    int offset = headerSize;
    for (int i = 0; i < count; ++i) {
        int sz = sizes[i];
        quint8 w = (sz >= 256) ? 0 : static_cast<quint8>(sz);
        quint8 h = (sz >= 256) ? 0 : static_cast<quint8>(sz);
        ds << w;                        // width  (0 = 256)
        ds << h;                        // height (0 = 256)
        ds << quint8(0);                // colorCount
        ds << quint8(0);                // reserved
        ds << quint16(1);               // planes
        ds << quint16(32);              // bitCount
        ds << quint32(pngList[i].size()); // sizeInBytes
        ds << quint32(offset);          // imageOffset
        offset += pngList[i].size();
    }

    // --- 画像データ ---
    for (const auto &png : pngList)
        f.write(png);

    f.close();
    return true;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QString outPath = (argc >= 2) ? argv[1] : "app_icon.ico";

    const QList<int> sizes = { 16, 32, 48, 256 };
    if (writeIco(outPath, sizes)) {
        qInfo("Icon written: %s", qPrintable(outPath));
        return 0;
    } else {
        qCritical("Failed to write: %s", qPrintable(outPath));
        return 1;
    }
}
