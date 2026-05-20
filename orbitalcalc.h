#pragma once
#include <QVector3D>
#include <QDateTime>

// ユリウス日 (JD) の計算
double dateTimeToJD(const QDateTime &dt);
double currentJD();

// ケプラー方程式 E - e*sin(E) = M をニュートン法で解く
// M_rad: 平均近点角 [ラジアン], e: 離心率
// 戻り値: 離心近点角 E [ラジアン]
double solveKepler(double M_rad, double e);

// 軌道要素 (J2000.0 基準, NASA JPL Approximate Positions)
struct OrbitalElements {
    // J2000.0 時点の値
    double a0;      // 長半径 [AU]
    double e0;      // 離心率
    double I0;      // 軌道傾斜角 [度]
    double L0;      // 平均経度 [度]
    double wbar0;   // 近日点経度 (longitude of perihelion) [度]
    double Omega0;  // 昇交点黄経 [度]

    // 1ユリウス世紀あたりの変化率
    double da, de, dI, dL, dwbar, dOmega;
};

// 指定 JD における太陽中心黄道座標 (J2000.0) [AU] を計算
QVector3D calcHelioEcliptic(const OrbitalElements &elem, double jd);
