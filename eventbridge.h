#pragma once
#include <QObject>
#include <QVariantList>
#include <QDateTime>
#include <qqmlintegration.h>

// QML に天文イベントデータを渡す軽量ブリッジ
class EventBridge : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit EventBridge(QObject *parent = nullptr) : QObject(parent) {}

    // detectEvents() を呼び出してイベントを QVariantList で返す
    Q_INVOKABLE QVariantList computeEvents(double startJd, double endJd, bool includeDwarfs) const;

    // QDateTime → Julian Day
    Q_INVOKABLE static double dateToJd(const QDateTime &dt);
};
