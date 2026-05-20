#pragma once
#include <QVector>
#include <QString>

enum class EventType {
    Conjunction,            // 合（外惑星）/ 外合（内惑星）
    Opposition,             // 衝
    InferiorConjunction,    // 内合（内惑星のみ）
    GreatestElongEast,      // 東方最大離角（内惑星のみ）
    GreatestElongWest,      // 西方最大離角（内惑星のみ）
};

struct AstroEvent {
    double    jd;           // ユリウス日
    int       planetIdx;    // getPlanets() のインデックス
    EventType type;
    double    elongDeg;     // 最大離角イベントでの離角 [度]
};

// startJD から endJD の範囲で天文イベントを検出して返す
// includeDwarfs=true のとき矮小惑星（インデックス8〜12）も対象に加える
QVector<AstroEvent> detectEvents(double startJD, double endJD,
                                 bool includeDwarfs = false);

QString eventTypeName(EventType t);
