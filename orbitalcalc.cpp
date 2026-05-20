#include "orbitalcalc.h"
#include <cmath>

static constexpr double DEG2RAD = M_PI / 180.0;

// ユリウス日への変換
double dateTimeToJD(const QDateTime &dt)
{
    // QDateTimeのmsSinceEpoch (Unix epoch = JD 2440587.5) を利用
    qint64 ms = dt.toMSecsSinceEpoch();
    return ms / 86400000.0 + 2440587.5;
}

double currentJD()
{
    return dateTimeToJD(QDateTime::currentDateTimeUtc());
}

// ニュートン法でケプラー方程式を解く
double solveKepler(double M_rad, double e)
{
    // 初期値
    double E = M_rad;
    for (int i = 0; i < 100; ++i) {
        double dE = (M_rad - E + e * std::sin(E)) / (1.0 - e * std::cos(E));
        E += dE;
        if (std::abs(dE) < 1e-12) break;
    }
    return E;
}

// 角度を [-180, 180] 度に正規化
static double normalizeDeg(double deg)
{
    deg = std::fmod(deg, 360.0);
    if (deg > 180.0)  deg -= 360.0;
    if (deg < -180.0) deg += 360.0;
    return deg;
}

// 太陽中心黄道座標を計算
QVector3D calcHelioEcliptic(const OrbitalElements &elem, double jd)
{
    // J2000.0 からのユリウス世紀数
    const double T = (jd - 2451545.0) / 36525.0;

    // 軌道要素を時刻に応じて補間
    const double a     = elem.a0     + elem.da     * T;
    const double e     = elem.e0     + elem.de     * T;
    const double I     = elem.I0     + elem.dI     * T;
    const double L     = elem.L0     + elem.dL     * T;
    const double wbar  = elem.wbar0  + elem.dwbar  * T;
    const double Omega = elem.Omega0 + elem.dOmega * T;

    // 近点引数 ω = w̄ - Ω
    const double omega = wbar - Omega;

    // 平均近点角 M = L - w̄, [-180, 180] に正規化
    const double M_deg = normalizeDeg(L - wbar);
    const double M_rad = M_deg * DEG2RAD;

    // ケプラー方程式を解いて離心近点角 E を得る
    const double E = solveKepler(M_rad, e);

    // 軌道面内座標
    const double xOrb = a * (std::cos(E) - e);
    const double yOrb = a * std::sqrt(1.0 - e * e) * std::sin(E);

    // 黄道座標系への回転
    // R_z(-Ω) * R_x(-I) * R_z(-ω) の逆を適用
    const double cosO = std::cos(Omega * DEG2RAD);
    const double sinO = std::sin(Omega * DEG2RAD);
    const double cosI = std::cos(I     * DEG2RAD);
    const double sinI = std::sin(I     * DEG2RAD);
    const double cosw = std::cos(omega * DEG2RAD);
    const double sinw = std::sin(omega * DEG2RAD);

    // 変換行列の各成分
    const double Px =  cosO * cosw - sinO * sinw * cosI;
    const double Py = -cosO * sinw - sinO * cosw * cosI;
    const double Qx =  sinO * cosw + cosO * sinw * cosI;
    const double Qy = -sinO * sinw + cosO * cosw * cosI;
    const double Rx =  sinw * sinI;
    const double Ry =  cosw * sinI;

    const float x = static_cast<float>(Px * xOrb + Py * yOrb);
    const float y = static_cast<float>(Qx * xOrb + Qy * yOrb);
    const float z = static_cast<float>(Rx * xOrb + Ry * yOrb);

    return QVector3D(x, y, z);
}
