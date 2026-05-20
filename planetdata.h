#pragma once
#include "orbitalcalc.h"
#include <QColor>
#include <QString>
#include <QVector>

// 天体の表示情報 + 軌道要素
struct PlanetInfo {
    QString     name;            // 名前（日本語）
    QString     nameEn;          // 名前（英語）
    QColor      color;           // 表示色
    double      radius;          // 表示半径（相対値）
    OrbitalElements elem;        // 軌道要素 (NASA JPL J2000.0)
    double      diameterKm  = 0; // 直径 [km]
    double      densityGcm3 = 0; // 密度 [g/cm³]
    double      rotationDays= 0; // 自転周期 [日]（負 = 逆回転）
    int         moonCount   = 0; // 衛星数
    // 自転軸北極の黄道 J2000.0 単位ベクトル（IAU 北極を equatorial→ecliptic 変換）
    QVector3D   poleEcl     = {0.f, 0.f, 1.f};
};

// 衛星の表示情報（軌道要素は親惑星中心座標）
struct SatelliteInfo {
    PlanetInfo  info;       // 軌道要素・表示情報
    int         parentIdx;  // 親惑星のインデックス (getPlanets() 参照)
};

// 8惑星のデータを返す
// 出典: NASA JPL "Keplerian Elements for Approximate Positions of the Planets"
// https://ssd.jpl.nasa.gov/planets/approx_pos.html (Table 1, 1800AD-2050AD)
const QVector<PlanetInfo> &getPlanets();

// 矮小惑星（ケレス・冥王星・ハウメア・マケマケ・エリス）のデータを返す
const QVector<PlanetInfo> &getDwarfPlanets();

// 主要衛星（月・ガリレオ4衛星・タイタン）のデータを返す
const QVector<SatelliteInfo> &getSatellites();
