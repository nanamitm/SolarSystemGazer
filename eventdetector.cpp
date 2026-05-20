#include "eventdetector.h"
#include "planetdata.h"
#include "orbitalcalc.h"
#include <cmath>
#include <algorithm>

// 任意の軌道要素から地心離角を返す [−180, 180] 度
static double calcElongFromElem(const OrbitalElements &elem, double jd)
{
    const QVector3D earth = calcHelioEcliptic(getPlanets()[2].elem, jd);
    const QVector3D body  = calcHelioEcliptic(elem, jd);
    const QVector3D geo   = body - earth;

    const double lSun  = std::atan2(-(double)earth.y(), -(double)earth.x()) * 180.0 / M_PI;
    const double lBody = std::atan2( (double)geo.y(),    (double)geo.x())   * 180.0 / M_PI;

    double e = lBody - lSun;
    while (e >  180.0) e -= 360.0;
    while (e <= -180.0) e += 360.0;
    return e;
}

static double calcElong(int idx, double jd)
{
    return calcElongFromElem(getPlanets()[idx].elem, jd);
}

static bool isInner(int idx) { return idx == 0 || idx == 1; }  // 水星・金星

// 合・衝の境界を区別するしきい値
// 0° 通過 (合) ならば |prev|+|curr| が小さく、±180° 通過 (衝) なら大きい
static constexpr double CROSS_THRESH = 45.0;

QVector<AstroEvent> detectEvents(double startJD, double endJD, bool includeDwarfs)
{
    QVector<AstroEvent> events;
    const int N = 8;

    // 各惑星の直前2ステップの離角を保持
    double prev2[N] = {}, prev1[N] = {};
    for (int i = 0; i < N; ++i) {
        if (i == 2) continue;
        prev2[i] = calcElong(i, startJD);
        prev1[i] = calcElong(i, startJD + 1.0);
    }

    for (double jd = startJD + 2.0; jd <= endJD; jd += 1.0) {
        for (int i = 0; i < N; ++i) {
            if (i == 2) continue;

            const double curr = calcElong(i, jd);
            const double prev = prev1[i];
            const double pp   = prev2[i];

            // ─── 合・衝の検出（0° または ±180° の通過） ───────────────
            if (prev * curr < 0.0) {
                const double sumAbs = std::abs(prev) + std::abs(curr);
                // 内挿でイベント JD を精度よく推定
                const double frac   = std::abs(prev) / sumAbs;
                const double jd_ev  = (jd - 1.0) + frac;

                if (sumAbs < CROSS_THRESH) {
                    // ── 0° 通過 → 合（外惑星）or 内合・外合（内惑星）──
                    if (isInner(i)) {
                        const auto   &planets = getPlanets();
                        const QVector3D earth   = calcHelioEcliptic(planets[2].elem, jd_ev);
                        const QVector3D planet  = calcHelioEcliptic(planets[i].elem,  jd_ev);
                        const bool inferior     = (planet - earth).length() < earth.length();
                        events.append({jd_ev, i,
                            inferior ? EventType::InferiorConjunction
                                     : EventType::Conjunction,
                            0.0});
                    } else {
                        events.append({jd_ev, i, EventType::Conjunction, 0.0});
                    }
                } else if (!isInner(i)) {
                    // ── ±180° 通過 → 衝（外惑星のみ）───────────────────
                    events.append({jd_ev, i, EventType::Opposition, 0.0});
                }
            }

            // ─── 内惑星の最大離角検出 ────────────────────────────────────
            if (isInner(i)) {
                const double d1 = prev - pp;    // 前ステップの変化量
                const double d2 = curr - prev;  // 今ステップの変化量
                if (d1 > 0.0 && d2 <= 0.0 && prev > 5.0) {
                    // 正の極大 → 東方最大離角
                    events.append({jd - 1.0, i, EventType::GreatestElongEast, std::abs(prev)});
                } else if (d1 < 0.0 && d2 >= 0.0 && prev < -5.0) {
                    // 負の極小 → 西方最大離角
                    events.append({jd - 1.0, i, EventType::GreatestElongWest, std::abs(prev)});
                }
            }

            prev2[i] = prev1[i];
            prev1[i] = curr;
        }
    }

    // ── 矮小惑星（外惑星扱い: 合・衝のみ）────────────────────────────
    if (includeDwarfs) {
        const auto &dwarfs  = getDwarfPlanets();
        const int   DWOFF   = 8;   // getPlanets().size()

        QVector<double> dp1(dwarfs.size());
        for (int i = 0; i < dwarfs.size(); ++i)
            dp1[i] = calcElongFromElem(dwarfs[i].elem, startJD + 1.0);

        for (double jd = startJD + 2.0; jd <= endJD; jd += 1.0) {
            for (int i = 0; i < dwarfs.size(); ++i) {
                const double curr = calcElongFromElem(dwarfs[i].elem, jd);
                const double prev = dp1[i];

                if (prev * curr < 0.0) {
                    const double sumAbs = std::abs(prev) + std::abs(curr);
                    const double frac   = std::abs(prev) / sumAbs;
                    const double jd_ev  = (jd - 1.0) + frac;
                    events.append({jd_ev, DWOFF + i,
                        sumAbs < CROSS_THRESH ? EventType::Conjunction
                                              : EventType::Opposition,
                        0.0});
                }
                dp1[i] = curr;
            }
        }
    }

    std::sort(events.begin(), events.end(), [](const AstroEvent &a, const AstroEvent &b) {
        return a.jd < b.jd;
    });

    return events;
}

QString eventTypeName(EventType t)
{
    switch (t) {
    case EventType::Conjunction:         return "合";
    case EventType::Opposition:          return "衝";
    case EventType::InferiorConjunction: return "内合";
    case EventType::GreatestElongEast:   return "東方最大離角";
    case EventType::GreatestElongWest:   return "西方最大離角";
    }
    return "";
}
