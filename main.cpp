#include <QApplication>
#include "mainwindow.h"
#include "appicon.h"

#ifdef Q_OS_WASM
#include <QFontDatabase>
#include <QFont>
#include <QScreen>

// Qt for WebAssembly bundles DejaVu only, which has no CJK glyphs, so every
// Japanese label would render as tofu. resources/fonts holds a Regular
// instance of Noto Sans JP subset to the characters this UI uses.
static void loadJapaneseFont(QApplication& app)
{
    const int id = QFontDatabase::addApplicationFont(":/fonts/resources/fonts/NotoSansJP-subset.ttf");
    if (id < 0) {
        qWarning("failed to load the bundled Japanese font");
        return;
    }
    const QStringList families = QFontDatabase::applicationFontFamilies(id);
    if (families.isEmpty()) return;

    QFont font = app.font();
    font.setFamily(families.first());
    app.setFont(font);
}
#endif

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Solar System Gazer");
    app.setOrganizationName("Qt6 Demo");
    app.setWindowIcon(createAppIcon());

#ifdef Q_OS_WASM
    loadJapaneseFont(app);
#endif

    MainWindow w;
#ifdef Q_OS_WASM
    // ブラウザの表示領域いっぱいに広げ、リサイズにも追従する
    // （Qt for WebAssembly はウィンドウを自動では追従させない）
    if (QScreen* screen = app.primaryScreen()) {
        w.setGeometry(screen->geometry());
        QObject::connect(screen, &QScreen::geometryChanged,
                         &w, [&w](const QRect& g) { w.setGeometry(g); });
    }
#endif
    w.show();
    return app.exec();
}
