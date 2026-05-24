#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QStringLiteral>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName("SolarSystemGazer");
    app.setOrganizationName("SolarGazer");

    QQmlApplicationEngine engine;

    // qt_add_qml_module のデフォルト resource prefix は :/qt/qml/<URI>/
    using namespace Qt::StringLiterals;
    const QUrl url(u"qrc:/qt/qml/SolarSystemGazer/qml/main.qml"_s);
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated,
        &app, [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);

    engine.load(url);
    return app.exec();
}
