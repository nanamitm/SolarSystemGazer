// android_icon_gen.cpp
// drawPlanetIcon() を使って Android ランチャーアイコン (mipmap PNG) を生成する。
// 使い方: android_icon_gen <android/res のパス>
#include "../icon_draw.h"
#include <QCoreApplication>
#include <QDir>
#include <QString>
#include <QList>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    const QString resDir = (argc >= 2) ? QString::fromLocal8Bit(argv[1]) : QStringLiteral("android/res");

    struct Density { const char *name; int size; };
    const QList<Density> densities = {
        { "mdpi",    48 },
        { "hdpi",    72 },
        { "xhdpi",   96 },
        { "xxhdpi",  144 },
        { "xxxhdpi", 192 },
    };

    for (const auto &d : densities) {
        const QString dir = QStringLiteral("%1/mipmap-%2").arg(resDir, QString::fromLatin1(d.name));
        QDir().mkpath(dir);
        const QImage img = drawPlanetIcon(d.size);
        const bool ok1 = img.save(dir + "/ic_launcher.png", "PNG");
        const bool ok2 = img.save(dir + "/ic_launcher_round.png", "PNG");
        if (!ok1 || !ok2) {
            qCritical("Failed to write icons in %s", qPrintable(dir));
            return 1;
        }
        qInfo("Wrote %s/ic_launcher(.png/_round.png)  %dpx", qPrintable(dir), d.size);
    }
    return 0;
}
