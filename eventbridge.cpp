#include "eventbridge.h"
#include "eventdetector.h"
#include "planetdata.h"
#include "orbitalcalc.h"

#include <QDateTime>
#include <QVariantMap>

QVariantList EventBridge::computeEvents(double startJd, double endJd, bool includeDwarfs) const
{
    const QVector<AstroEvent> raw = detectEvents(startJd, endJd, includeDwarfs);
    const auto &planets = getPlanets();
    const auto &dwarfs  = getDwarfPlanets();
    const int   pCnt    = planets.size();

    QVariantList result;
    result.reserve(raw.size());

    for (const auto &ev : raw) {
        const QDateTime dt = QDateTime::fromMSecsSinceEpoch(
            qint64((ev.jd - 2440587.5) * 86400000.0), Qt::UTC);

        const QString planetName = (ev.planetIdx < pCnt)
            ? planets[ev.planetIdx].name
            : dwarfs[ev.planetIdx - pCnt].name;

        QString elongStr;
        if (ev.type == EventType::GreatestElongEast ||
            ev.type == EventType::GreatestElongWest)
            elongStr = QString("%1°").arg(ev.elongDeg, 0, 'f', 1);

        const bool   isDwarf = (ev.planetIdx >= pCnt);
        const double a0      = isDwarf
            ? dwarfs[ev.planetIdx - pCnt].elem.a0
            : planets[ev.planetIdx].elem.a0;

        QVariantMap entry;
        entry["jd"]        = ev.jd;
        entry["dateStr"]   = dt.toString("yyyy-MM-dd  hh:mm");
        entry["planet"]    = planetName;
        entry["eventType"] = eventTypeName(ev.type);
        entry["elong"]     = elongStr;
        entry["planetIdx"] = ev.planetIdx;
        entry["isDwarf"]   = isDwarf;
        entry["a0"]        = a0;
        result.append(entry);
    }
    return result;
}

double EventBridge::dateToJd(const QDateTime &dt)
{
    return dateTimeToJD(dt);
}
