#include "solaritem.h"
#include "planetdata.h"
#include "orbitalcalc.h"

#include <QPainter>
#include <QPainterPath>
#include <QFont>
#include <QFontMetrics>
#include <cmath>
#include <algorithm>

static constexpr double DEG2RAD   = M_PI / 180.0;
static constexpr int    SATURN_IDX = 5;

SolarItem::SolarItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
    , m_dateTime(QDateTime::currentDateTimeUtc())
{
    setFillColor(Qt::black);
    setRenderTarget(QQuickPaintedItem::FramebufferObject);
}

// ============================================================
//  プロパティ setter
// ============================================================
void SolarItem::setDateTime(const QDateTime &dt)
{
    m_dateTime = dt;
    if (m_centerBody >= 0)
        m_centerOffset = calcHelioEcliptic(getPlanets()[m_centerBody].elem, dateTimeToJD(dt));
    update();
    emit dateTimeChanged();
}

void SolarItem::setCenterBody(int idx)
{
    m_centerBody = idx;
    if (idx >= 0) {
        const double a = getPlanets()[idx].elem.a0;
        m_camDist      = qBound(2.5, a * 3.5, 200.0);
        m_centerOffset = calcHelioEcliptic(getPlanets()[idx].elem, dateTimeToJD(m_dateTime));
    } else {
        m_camDist      = 50.0;
        m_centerOffset = {};
    }
    clearTrails();
    update();
    emit centerBodyChanged();
    emit cameraChanged();
}

void SolarItem::setCamera(double lon, double lat)
{
    m_camLon = lon;
    m_camLat = qBound(-89.0, lat, 89.0);
    update();
    emit cameraChanged();
}

void SolarItem::setCameraDistance(double dist)
{
    m_camDist = qBound(0.001, dist, 200.0);
    update();
    emit cameraChanged();
}

void SolarItem::setShowLabels(bool show)
{
    m_showLabels = show;
    update();
    emit showLabelsChanged();
}

void SolarItem::setShowDwarfPlanets(bool show)
{
    m_showDwarfPlanets = show;
    const int dwarfOff = getPlanets().size();
    const int satOff   = dwarfOff + getDwarfPlanets().size();
    if (!show && m_selectedPlanet >= dwarfOff && m_selectedPlanet < satOff)
        m_selectedPlanet = -1;
    update();
    emit showDwarfPlanetsChanged();
}

void SolarItem::setShowSatellites(bool show)
{
    m_showSatellites = show;
    const int satOff = getPlanets().size() + getDwarfPlanets().size();
    if (!show && m_selectedPlanet >= satOff)
        m_selectedPlanet = -1;
    update();
    emit showSatellitesChanged();
}

void SolarItem::setDirLock(DirLock mode)
{
    m_dirLock = mode;
    if (mode == DirLock::SunBack) {
        m_camLat = 3.0;
        emit cameraChanged();   // 緯度変更を通知（バインディング・設定保存のため）
    }
    update();
    emit dirLockChanged();
}

void SolarItem::setTrailActive(bool active)
{
    m_trailActive = active;
    if (active)
        m_trailStartJD = dateTimeToJD(m_dateTime);
    update();
}

void SolarItem::clearTrails()
{
    m_trailActive = false;
    update();
}

// ============================================================
//  タッチ操作
// ============================================================
void SolarItem::rotateDelta(double dx, double dy)
{
    if (m_dirLock == DirLock::None)
        m_camLon -= dx * 0.4;
    m_camLat += dy * 0.4;
    m_camLat  = qBound(-89.0, m_camLat, 89.0);
    update();
    emit cameraChanged();
}

void SolarItem::pinchZoom(double scaleDelta)
{
    if (scaleDelta > 0.01)
        m_camDist = qBound(0.001, m_camDist / scaleDelta, 200.0);
    update();
    emit cameraChanged();
}

void SolarItem::tap(double x, double y)
{
    int hit = hitTest(x, y, dateTimeToJD(m_dateTime));
    m_selectedPlanet = (hit == m_selectedPlanet) ? -1 : hit;
    update();
    emit selectedPlanetChanged();
}

// ============================================================
//  透視投影
// ============================================================
QVector3D SolarItem::project(const QVector3D &pos3d) const
{
    const double lonR = m_camLon * DEG2RAD;
    const double latR = m_camLat * DEG2RAD;
    const float  cx   = (float)(m_camDist * cos(latR) * cos(lonR));
    const float  cy   = (float)(m_camDist * cos(latR) * sin(lonR));
    const float  cz   = (float)(m_camDist * sin(latR));

    QVector3D fwd = QVector3D(-cx, -cy, -cz).normalized();
    QVector3D up(0, 0, 1);
    if (std::abs(m_camLat) > 89.0)
        up = QVector3D((float)cos(lonR + M_PI/2), (float)sin(lonR + M_PI/2), 0);

    QVector3D right = QVector3D::crossProduct(fwd, up).normalized();
    up = QVector3D::crossProduct(right, fwd).normalized();

    QVector3D local = pos3d - m_centerOffset;
    QVector3D rel(local.x()-cx, local.y()-cy, local.z()-cz);
    float depth = QVector3D::dotProduct(rel, fwd);
    if (depth <= 0.0f) return {0, 0, -1};

    const float focal = (float)(m_camDist * 1.732);
    const float scale = std::min(w(), h()) * 0.45f / (float)m_camDist;
    float px = w() * 0.5f + (QVector3D::dotProduct(rel, right) / depth * focal) * scale;
    float py = h() * 0.5f - (QVector3D::dotProduct(rel, up)    / depth * focal) * scale;
    return {px, py, depth};
}

// ============================================================
//  軌跡描画
// ============================================================
void SolarItem::drawTrails(QPainter &p, double jd) const
{
    if (!m_trailActive) return;

    const auto &planets = getPlanets();
    for (int i = 0; i < planets.size(); ++i) {
        if (i == m_centerBody) continue;
        const double periodDays = std::pow(planets[i].elem.a0, 1.5) * 365.25;
        const double maxDays    = TRAIL_ORBITS * periodDays;
        const double startJD    = std::max(jd - maxDays, m_trailStartJD);
        const double spanDays   = jd - startJD;
        if (spanDays <= 0) continue;

        QColor base = planets[i].color;
        QVector3D prev = project(calcHelioEcliptic(planets[i].elem, startJD));

        for (int s = 1; s <= TRAIL_STEPS; ++s) {
            double jdMid = startJD + spanDays * s / TRAIL_STEPS;
            QVector3D cur = project(calcHelioEcliptic(planets[i].elem, jdMid));

            if (prev.z() > 0 && cur.z() > 0) {
                float alpha = (float)s / TRAIL_STEPS * 0.85f;
                QColor col = base;
                col.setAlphaF(alpha);
                p.setPen(QPen(col, 1.3f));
                p.drawLine(QPointF(prev.x(), prev.y()), QPointF(cur.x(), cur.y()));
            }
            prev = cur;
        }
    }
}

// ============================================================
//  軌道楕円描画
// ============================================================
void SolarItem::drawOrbit(QPainter &p, const PlanetInfo &planet, double jd) const
{
    const double period = std::pow(planet.elem.a0, 1.5) * 365.25;
    const double step   = period / 180.0;
    QPainterPath path;
    bool first = true;
    for (int i = 0; i <= 180; ++i) {
        QVector3D sp = project(calcHelioEcliptic(planet.elem, jd + step * i));
        if (sp.z() <= 0) { first = true; continue; }
        if (first) { path.moveTo(sp.x(), sp.y()); first = false; }
        else          path.lineTo(sp.x(), sp.y());
    }
    QColor oc = planet.color.darker(200);
    oc.setAlpha(110);
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(oc, 1.0));
    p.drawPath(path);
}

void SolarItem::drawOrbit(QPainter &p, int idx, double jd) const
{
    drawOrbit(p, getPlanets()[idx], jd);
}

// ============================================================
//  土星の環描画
// ============================================================
void SolarItem::drawSaturnRings(QPainter &p, const QVector3D &satPos3d,
                                 QPointF sc, bool frontHalf) const
{
    static const QVector3D N(0.0815f, 0.4623f, 0.8829f);

    const float lonR = (float)(m_camLon * DEG2RAD);
    const float latR = (float)(m_camLat * DEG2RAD);
    const QVector3D camPos(
        (float)(m_camDist * cos(latR) * cos(lonR)) + m_centerOffset.x(),
        (float)(m_camDist * cos(latR) * sin(lonR)) + m_centerOffset.y(),
        (float)(m_camDist * sin(latR))              + m_centerOffset.z());

    QVector3D viewDir = (satPos3d - camPos).normalized();
    float tiltFactor  = std::abs(QVector3D::dotProduct(N, -viewDir));
    if (tiltFactor < 0.04f) return;

    QVector3D spN = project(satPos3d + 1.0f * N);
    bool negated   = false;
    if (spN.z() <= 0.0f) { spN = project(satPos3d - 1.0f * N); negated = true; }

    QPointF poleDir(0.0, 1.0);
    if (spN.z() > 0.0f) {
        poleDir = QPointF(spN.x()-sc.x(), spN.y()-sc.y());
        if (negated) poleDir = -poleDir;
        float len = (float)std::hypot(poleDir.x(), poleDir.y());
        if (len > 0.01f) poleDir /= len; else poleDir = QPointF(0.0, 1.0);
    }
    QPointF majorDir(-poleDir.y(), poleDir.x());
    float rotAngle = (float)(std::atan2(majorDir.y(), majorDir.x()) * 180.0 / M_PI);
    bool poleTowardCam = QVector3D::dotProduct(N, camPos - satPos3d) > 0;

    const float pr   = (float)getPlanets()[SATURN_IDX].radius;
    const float rBin = pr*1.40f, rBout = pr*1.90f;
    const float rAin = pr*1.96f, rAout = pr*2.48f;

    p.save();
    p.translate(sc);
    p.rotate((double)rotAngle);

    auto annulus = [&](float inner, float outer) -> QPainterPath {
        QPainterPath ring, hole;
        ring.addEllipse(QPointF(0,0), (double)outer, (double)(outer*tiltFactor));
        hole.addEllipse(QPointF(0,0), (double)inner, (double)(inner*tiltFactor));
        return ring.subtracted(hole);
    };

    float ch = rAout * 4.0f;
    QPainterPath clip;
    if (frontHalf == poleTowardCam)
        clip.addRect((double)-ch, (double)-ch, (double)(2*ch), (double)ch);
    else
        clip.addRect((double)-ch, 0.0,         (double)(2*ch), (double)ch);

    auto drawHalf = [&](float rin, float rout, QColor col) {
        QPainterPath shape = annulus(rin, rout).intersected(clip);
        if (shape.isEmpty()) return;
        p.setBrush(col);
        p.setPen(QPen(col.lighter(115), 0.5));
        p.drawPath(shape);
    };

    if (!frontHalf) {
        drawHalf(rBin, rBout, QColor(175,155, 95,145));
        drawHalf(rAin, rAout, QColor(155,135, 80,120));
    } else {
        drawHalf(rBin, rBout, QColor(210,185,115,185));
        drawHalf(rAin, rAout, QColor(188,163, 98,162));
    }
    p.restore();
}

// ============================================================
//  惑星描画
// ============================================================
void SolarItem::drawPlanet(QPainter &p, int idx, double jd, bool showLabel) const
{
    const auto &planet = getPlanets()[idx];
    QVector3D pos = calcHelioEcliptic(planet.elem, jd);
    QVector3D sp  = project(pos);
    if (sp.z() <= 0) return;

    QPointF sc(sp.x(), sp.y());
    float r = (float)planet.radius;

    if (idx == SATURN_IDX) drawSaturnRings(p, pos, sc, false);

    QRadialGradient glow(sc, (double)(r*2.8f));
    QColor gc = planet.color; gc.setAlpha(75);
    glow.setColorAt(0.0, gc); glow.setColorAt(1.0, Qt::transparent);
    p.setBrush(glow); p.setPen(Qt::NoPen);
    p.drawEllipse(sc, (double)(r*2.8f), (double)(r*2.8f));

    p.setBrush(planet.color);
    p.setPen(QPen(planet.color.lighter(150), 0.5));
    p.drawEllipse(sc, (double)r, (double)r);

    if (idx == SATURN_IDX) drawSaturnRings(p, pos, sc, true);

    if (idx == m_selectedPlanet) {
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(planet.color.lighter(200), 1.5, Qt::DashLine));
        p.drawEllipse(sc, (double)(r*3.2f), (double)(r*3.2f));
    }

    if (showLabel) {
        p.setPen(planet.color.lighter(190));
        p.setFont(QFont("Arial", 9));
        p.drawText(QPointF(sc.x()+r+3, sc.y()+4), planet.name);
    }
}

// ============================================================
//  自転軸描画
// ============================================================
void SolarItem::drawRotationAxis(QPainter &p, int idx, double jd) const
{
    if (idx < 0 || idx >= getPlanets().size()) return;
    const PlanetInfo &planet = getPlanets()[idx];

    const QVector3D pos = calcHelioEcliptic(planet.elem, jd);
    const QVector3D sp3 = project(pos);
    if (sp3.z() <= 0) return;
    const QPointF sc(sp3.x(), sp3.y());

    const QVector3D poleScr3 = project(pos + planet.poleEcl);
    QPointF axDir(0.0, -1.0);
    if (poleScr3.z() > 0) {
        const QPointF d(poleScr3.x() - sc.x(), poleScr3.y() - sc.y());
        const double len = std::hypot(d.x(), d.y());
        if (len > 0.5) axDir = d / len;
    }

    const double r       = planet.radius;
    const double axLen   = r * 3.5 + 12.0;
    const QPointF northPt = sc + axDir * axLen;
    const QPointF southPt = sc - axDir * axLen;

    QColor axCol = planet.color.lighter(200);
    axCol.setAlpha(210);

    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(axCol, 1.0, Qt::DashLine));
    p.drawLine(sc, southPt);
    p.setPen(QPen(axCol, 1.5, Qt::SolidLine));
    p.drawLine(sc, northPt);

    const double ang = std::atan2(axDir.y(), axDir.x());
    p.setBrush(axCol);
    p.setPen(Qt::NoPen);
    const double aLen = 6.0;
    QPolygonF tip;
    tip << northPt
        << northPt + QPointF(std::cos(ang + 2.7) * aLen, std::sin(ang + 2.7) * aLen)
        << northPt + QPointF(std::cos(ang - 2.7) * aLen, std::sin(ang - 2.7) * aLen);
    p.drawPolygon(tip);

    p.setPen(axCol);
    p.setFont(QFont("Arial", 7, QFont::Bold));
    p.drawText(northPt + QPointF(4, -2), "N");
}

// ============================================================
//  情報パネル描画
// ============================================================
void SolarItem::drawInfoPanel(QPainter &p, int idx, double jd) const
{
    const auto &planets = getPlanets();
    const auto &dwarfs  = getDwarfPlanets();
    const auto &sats    = getSatellites();
    const int   satOff  = planets.size() + dwarfs.size();
    const int   total   = satOff + sats.size();
    if (idx < 0 || idx >= total) return;

    const bool isSat   = (idx >= satOff);
    const bool isDwarf = !isSat && (idx >= planets.size());
    const PlanetInfo &planet = isSat   ? sats[idx - satOff].info
                             : isDwarf ? dwarfs[idx - planets.size()]
                             : planets[idx];

    QVector3D relPos;
    QVector3D pos;
    if (isSat) {
        relPos = calcHelioEcliptic(planet.elem, jd);
        pos    = calcHelioEcliptic(planets[sats[idx-satOff].parentIdx].elem, jd) + relPos;
    } else {
        pos = calcHelioEcliptic(planet.elem, jd);
    }
    float distSun   = pos.length();
    QVector3D earth = calcHelioEcliptic(planets[2].elem, jd);
    float distEarth = (pos - earth).length();

    const double a = planet.elem.a0;
    double period  = std::pow(a, 1.5);
    double speed   = 29.783 * std::sqrt(2.0/distSun - 1.0/a);

    // 画面幅に応じてパネル幅を調整（スマートフォン対応）
    const int PW = std::min(230, w() - 20);
    const int PH = 240;
    const int PX = w() - PW - 10;
    const int PY = 48;

    p.setBrush(QColor(15,20,45,215));
    p.setPen(QPen(planet.color.lighter(140), 1.5));
    p.drawRoundedRect(QRectF(PX,PY,PW,PH), 7, 7);

    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.setPen(planet.color.lighter(170));
    p.drawText(QRectF(PX+10,PY+8,PW-20,22),
               Qt::AlignLeft|Qt::AlignVCenter,
               QString("%1   %2").arg(planet.name, planet.nameEn));

    p.setPen(QPen(planet.color.darker(120), 0.5));
    p.drawLine(PX+10, PY+32, PX+PW-10, PY+32);

    int y = PY+48;
    auto row = [&](const QString &label, const QString &val) {
        p.setFont(QFont("Arial", 9));
        p.setPen(QColor(130,145,175)); p.drawText(PX+12, y, label);
        p.setPen(QColor(215,230,255)); p.drawText(PX+125, y, val);
        const_cast<int&>(y) += 20;
    };
    row("太陽からの距離", QString("%1 AU").arg(distSun, 0,'f',3));
    if (isSat) {
        const int parentIdx    = sats[idx - satOff].parentIdx;
        double distParentKm    = relPos.length() * 149597870.7;
        double satPeriodDays   = 360.0 / (planet.elem.dL / 36525.0);
        double orbitSpeed      = 2.0 * M_PI * (a * 149597870.7) / (satPeriodDays * 86400.0);
        row(planets[parentIdx].name + "からの距離",
            QString("%1 万km").arg(distParentKm / 10000.0, 0,'f',1));
        row("公転周期",  QString("%1 日").arg(satPeriodDays, 0,'f',3));
        row("軌道速度",  QString("%1 km/s").arg(orbitSpeed, 0,'f',2));
    } else {
        if (m_centerBody < 0) {
            bool isEarth = !isDwarf && (idx == 2);
            row("地球からの距離", isEarth ? "—" : QString("%1 AU").arg(distEarth,0,'f',3));
        } else {
            QVector3D cPos = calcHelioEcliptic(planets[m_centerBody].elem, jd);
            float distCenter = (pos - cPos).length();
            bool isCenter = !isDwarf && (idx == m_centerBody);
            row("中心からの距離", isCenter ? "—" : QString("%1 AU").arg(distCenter,0,'f',3));
        }
        bool isEarth = !isDwarf && (idx == 2);
        row("公転周期",  isEarth ? "365.25 日" : QString("%1 年").arg(period,0,'f',2));
        row("軌道速度",  QString("%1 km/s").arg(speed,0,'f',1));
    }

    auto fmtKm = [](double km) -> QString {
        QString s = QString::number((qint64)km);
        for (int i = s.length()-3; i > 0; i -= 3) s.insert(i, ',');
        return s + " km";
    };
    auto fmtRot = [](double days) -> QString {
        const bool retro = days < 0;
        const double d   = std::abs(days);
        const QString s  = (d < 1.0)
            ? QString("%1h%2m").arg((int)(d*24))
                               .arg((int)(d*1440)%60, 2, 10, QChar('0'))
            : QString("%1 日").arg(d, 0, 'f', 3);
        return retro ? s + " (逆)" : s;
    };
    row("直径",     fmtKm(planet.diameterKm));
    row("密度",     QString("%1 g/cm³").arg(planet.densityGcm3, 0, 'f', 2));
    row("自転周期", fmtRot(planet.rotationDays));
    row("衛星数",   planet.moonCount == 0 ? "なし"
                                          : QString("%1 個").arg(planet.moonCount));
    if (planet.rotationDays != 0.0)
        row("自転方向", planet.rotationDays < 0.0 ? "↻ 逆行" : "↺ 順行");
}

// ============================================================
//  ヒットテスト
// ============================================================
int SolarItem::hitTest(double x, double y, double jd) const
{
    const auto &planets = getPlanets();
    int   hit = -1; float minD = 1e9f;
    for (int i = 0; i < planets.size(); ++i) {
        QVector3D sp = project(calcHelioEcliptic(planets[i].elem, jd));
        if (sp.z() <= 0) continue;
        float dx = (float)x-sp.x(), dy = (float)y-sp.y();
        float d  = std::sqrt(dx*dx+dy*dy);
        float th = std::max(18.0f, (float)planets[i].radius*3.0f);  // タッチ用に広め
        if (d < th && d < minD) { minD = d; hit = i; }
    }
    if (m_showDwarfPlanets) {
        const auto &dwarfs = getDwarfPlanets();
        for (int i = 0; i < dwarfs.size(); ++i) {
            QVector3D sp = project(calcHelioEcliptic(dwarfs[i].elem, jd));
            if (sp.z() <= 0) continue;
            float dx = (float)x-sp.x(), dy = (float)y-sp.y();
            float d  = std::sqrt(dx*dx+dy*dy);
            float th = std::max(18.0f, (float)dwarfs[i].radius*3.0f);
            if (d < th && d < minD) { minD = d; hit = planets.size() + i; }
        }
    }
    if (m_showSatellites) {
        const auto &sats   = getSatellites();
        const int   satOff = planets.size() + getDwarfPlanets().size();
        for (int i = 0; i < sats.size(); ++i) {
            const bool moonCase = (sats[i].parentIdx == 2) && (m_camDist < 3.0);
            const bool caseA    = (sats[i].parentIdx != 2) && (m_centerBody == sats[i].parentIdx);
            if (!moonCase && !caseA) continue;
            QVector3D parentPos = calcHelioEcliptic(planets[sats[i].parentIdx].elem, jd);
            QVector3D sp = project(parentPos + calcHelioEcliptic(sats[i].info.elem, jd));
            if (sp.z() <= 0) continue;
            float dx = (float)x-sp.x(), dy = (float)y-sp.y();
            float d  = std::sqrt(dx*dx+dy*dy);
            float th = std::max(16.0f, (float)sats[i].info.radius*3.0f);
            if (d < th && d < minD) { minD = d; hit = satOff + i; }
        }
    }
    return hit;
}

// ============================================================
//  paint (QQuickPaintedItem)
// ============================================================
void SolarItem::paint(QPainter *painter)
{
    QPainter &p = *painter;
    p.setRenderHint(QPainter::Antialiasing);
    const double jd = dateTimeToJD(m_dateTime);
    const auto  &planets = getPlanets();

    // 方向固定モード
    if (m_dirLock != DirLock::None) {
        const QVector3D ref = (m_centerBody >= 0)
            ? calcHelioEcliptic(planets[m_centerBody].elem, jd)
            : calcHelioEcliptic(planets[2].elem, jd);
        double angle = std::atan2((double)ref.y(), (double)ref.x()) * 180.0 / M_PI;
        if (m_dirLock == DirLock::SunBack)
            angle += 180.0;
        m_camLon = angle;
    }

    m_centerOffset = (m_centerBody >= 0)
        ? calcHelioEcliptic(getPlanets()[m_centerBody].elem, jd)
        : QVector3D();

    // 1. 軌跡
    drawTrails(p, jd);

    // 2. 軌道楕円（惑星）
    for (int i = 0; i < planets.size(); ++i)
        drawOrbit(p, i, jd);

    // 2b. 矮小惑星
    if (m_showDwarfPlanets) {
        for (const auto &d : getDwarfPlanets())
            drawOrbit(p, d, jd);
    }

    // 2c. 衛星
    if (m_showSatellites) {
        const auto &sats = getSatellites();
        for (const auto &sat : sats) {
            const bool moonCase  = (sat.parentIdx == 2) && (m_camDist < 3.0);
            const bool caseA     = (sat.parentIdx != 2) && (m_centerBody == sat.parentIdx);
            if (!moonCase && !caseA) continue;
            QVector3D parentPos = calcHelioEcliptic(planets[sat.parentIdx].elem, jd);
            double periodDays   = 360.0 / (sat.info.elem.dL / 36525.0);
            double step         = periodDays / 180.0;
            QPainterPath path;
            bool first = true;
            for (int i = 0; i <= 180; ++i) {
                QVector3D sp = project(parentPos + calcHelioEcliptic(sat.info.elem, jd + step * i));
                if (sp.z() <= 0) { first = true; continue; }
                if (first) { path.moveTo(sp.x(), sp.y()); first = false; }
                else          path.lineTo(sp.x(), sp.y());
            }
            QColor oc = sat.info.color.darker(200); oc.setAlpha(90);
            p.setBrush(Qt::NoBrush);
            p.setPen(QPen(oc, 0.8));
            p.drawPath(path);
        }
    }

    // 3-4. 天体を奥行きソートして描画
    const auto &dwarfs = getDwarfPlanets();
    QVector<bool> showLabel(planets.size(), false);
    QVector<bool> showDwarfLabel(dwarfs.size(), false);
    QVector<bool> showSatLabel;
    if (m_showLabels) {
        QFontMetrics fm(QFont("Arial", 9));
        QVector<QRectF> usedRects;

        auto tryAddLabel = [&](const QVector3D &sp, float r,
                               const QString &name, bool &flag) {
            if (sp.z() <= 0) return;
            QPointF lp(sp.x() + r + 3, sp.y() + 4);
            QRectF rect(lp.x() - 2, lp.y() - fm.ascent(),
                        fm.horizontalAdvance(name) + 6, fm.height() + 4);
            bool overlaps = std::any_of(usedRects.begin(), usedRects.end(),
                                        [&](const QRectF &used) { return rect.intersects(used); });
            if (!overlaps) { flag = true; usedRects.append(rect); }
        };

        QVector<int> order(planets.size());
        for (int i = 0; i < planets.size(); ++i) order[i] = i;
        std::sort(order.begin(), order.end(), [&](int a, int b) {
            return planets[a].radius > planets[b].radius;
        });
        for (int idx : order) {
            QVector3D sp = project(calcHelioEcliptic(planets[idx].elem, jd));
            tryAddLabel(sp, (float)planets[idx].radius, planets[idx].name, showLabel[idx]);
        }
        if (m_showDwarfPlanets) {
            for (int i = 0; i < dwarfs.size(); ++i) {
                QVector3D sp = project(calcHelioEcliptic(dwarfs[i].elem, jd));
                tryAddLabel(sp, (float)dwarfs[i].radius, dwarfs[i].name, showDwarfLabel[i]);
            }
        }
        if (m_showSatellites) {
            const auto &sats = getSatellites();
            showSatLabel.resize(sats.size(), false);
            for (int i = 0; i < sats.size(); ++i) {
                const bool moonCase = (sats[i].parentIdx == 2) && (m_camDist < 3.0);
                const bool caseA    = (sats[i].parentIdx != 2) && (m_centerBody == sats[i].parentIdx);
                if (!moonCase && !caseA) continue;
                QVector3D parentPos = calcHelioEcliptic(planets[sats[i].parentIdx].elem, jd);
                QVector3D sp = project(parentPos + calcHelioEcliptic(sats[i].info.elem, jd));
                tryAddLabel(sp, (float)sats[i].info.radius, sats[i].info.name, showSatLabel[i]);
            }
        }
    }

    const QVector3D sunScrn  = project({0, 0, 0});
    const float     sunDepth = (sunScrn.z() > 0) ? sunScrn.z() : 0.0f;
    auto behindSun = [&](float depth) -> bool {
        return sunDepth > 0.0f && depth > 0.0f && depth > sunDepth;
    };
    auto depthOf = [&](const QVector3D &worldPos) -> float {
        QVector3D sp = project(worldPos);
        return sp.z() > 0 ? sp.z() : -1.0f;
    };

    QVector<QVector3D> pPos(planets.size());
    QVector<float>     pDepth(planets.size());
    QVector<int>       pOrder(planets.size());
    for (int i = 0; i < planets.size(); ++i) {
        pPos[i]   = calcHelioEcliptic(planets[i].elem, jd);
        pDepth[i] = depthOf(pPos[i]);
        pOrder[i] = i;
    }
    std::sort(pOrder.begin(), pOrder.end(), [&](int a, int b) {
        return pDepth[a] > pDepth[b];
    });

    QVector<QVector3D> dPos(dwarfs.size());
    QVector<float>     dDepth(dwarfs.size());
    QVector<int>       dOrder(dwarfs.size());
    for (int i = 0; i < dwarfs.size(); ++i) {
        dPos[i]   = calcHelioEcliptic(dwarfs[i].elem, jd);
        dDepth[i] = depthOf(dPos[i]);
        dOrder[i] = i;
    }
    if (m_showDwarfPlanets) {
        std::sort(dOrder.begin(), dOrder.end(), [&](int a, int b) {
            return dDepth[a] > dDepth[b];
        });
    }

    const auto &sats   = getSatellites();
    const int   satOff = planets.size() + dwarfs.size();
    QVector<QVector3D> sPos(sats.size());
    QVector<float>     sDepth(sats.size(), -1.0f);
    QVector<bool>      sVis(sats.size(), false);
    QVector<int>       sOrder(sats.size());
    for (int i = 0; i < sats.size(); ++i) {
        const bool moonCase = (sats[i].parentIdx == 2) && (m_camDist < 3.0);
        const bool caseA    = (sats[i].parentIdx != 2) && (m_centerBody == sats[i].parentIdx);
        sVis[i]   = moonCase || caseA;
        sOrder[i] = i;
        if (sVis[i]) {
            QVector3D pp = calcHelioEcliptic(planets[sats[i].parentIdx].elem, jd);
            sPos[i]   = pp + calcHelioEcliptic(sats[i].info.elem, jd);
            sDepth[i] = depthOf(sPos[i]);
        }
    }
    if (m_showSatellites) {
        std::sort(sOrder.begin(), sOrder.end(), [&](int a, int b) {
            return sDepth[a] > sDepth[b];
        });
    }

    for (int pass = 0; pass < 2; ++pass) {

        if (pass == 1 && sunScrn.z() > 0) {
            const float r = 7.0f;
            QRadialGradient g(sunScrn.x(), sunScrn.y(), r*2.8);
            g.setColorAt(0.0, QColor(255,255,180,220));
            g.setColorAt(0.4, QColor(255,220, 50,160));
            g.setColorAt(1.0, QColor(255,150,  0,  0));
            p.setBrush(g); p.setPen(Qt::NoPen);
            p.drawEllipse(QPointF(sunScrn.x(),sunScrn.y()), r*2.8, r*2.8);
            p.setBrush(QColor(255,240,80));
            p.drawEllipse(QPointF(sunScrn.x(),sunScrn.y()), r, r);
        }

        const bool drawBehind = (pass == 0);

        for (int i : pOrder) {
            if (behindSun(pDepth[i]) == drawBehind)
                drawPlanet(p, i, jd, showLabel[i]);
        }

        if (m_showDwarfPlanets) {
            for (int i : dOrder) {
                if (behindSun(dDepth[i]) != drawBehind) continue;
                QVector3D sp3 = project(dPos[i]);
                if (sp3.z() <= 0) continue;
                QPointF sc(sp3.x(), sp3.y());
                float r = (float)dwarfs[i].radius;
                QRadialGradient glow(sc, r*2.8);
                QColor gc = dwarfs[i].color; gc.setAlpha(70);
                glow.setColorAt(0.0, gc); glow.setColorAt(1.0, Qt::transparent);
                p.setBrush(glow); p.setPen(Qt::NoPen);
                p.drawEllipse(sc, r*2.8, r*2.8);
                p.setBrush(dwarfs[i].color);
                p.setPen(QPen(dwarfs[i].color.lighter(150), 0.5));
                p.drawEllipse(sc, r, r);
                if (m_selectedPlanet == planets.size() + i) {
                    p.setBrush(Qt::NoBrush);
                    p.setPen(QPen(dwarfs[i].color.lighter(200), 1.5, Qt::DashLine));
                    p.drawEllipse(sc, r*3.2, r*3.2);
                }
                if (showDwarfLabel[i]) {
                    p.setPen(dwarfs[i].color.lighter(190));
                    p.setFont(QFont("Arial", 9));
                    p.drawText(QPointF(sc.x()+r+3, sc.y()+4), dwarfs[i].name);
                }
            }
        }

        if (m_showSatellites) {
            for (int i : sOrder) {
                if (!sVis[i]) continue;
                if (behindSun(sDepth[i]) != drawBehind) continue;
                QVector3D sp = project(sPos[i]);
                if (sp.z() <= 0) continue;
                QPointF sc(sp.x(), sp.y());
                float r = (float)sats[i].info.radius;
                QRadialGradient glow(sc, r*2.8);
                QColor gc = sats[i].info.color; gc.setAlpha(70);
                glow.setColorAt(0.0, gc); glow.setColorAt(1.0, Qt::transparent);
                p.setBrush(glow); p.setPen(Qt::NoPen);
                p.drawEllipse(sc, r*2.8, r*2.8);
                p.setBrush(sats[i].info.color);
                p.setPen(QPen(sats[i].info.color.lighter(150), 0.5));
                p.drawEllipse(sc, r, r);
                if (m_selectedPlanet == satOff + i) {
                    p.setBrush(Qt::NoBrush);
                    p.setPen(QPen(sats[i].info.color.lighter(200), 1.5, Qt::DashLine));
                    p.drawEllipse(sc, r*3.2, r*3.2);
                }
                if (showSatLabel.value(i, false)) {
                    p.setPen(sats[i].info.color.lighter(190));
                    p.setFont(QFont("Arial", 9));
                    p.drawText(QPointF(sc.x()+r+3, sc.y()+4), sats[i].info.name);
                }
            }
        }
    }

    if (m_selectedPlanet >= 0)
        drawRotationAxis(p, m_selectedPlanet, jd);

    if (m_selectedPlanet >= 0)
        drawInfoPanel(p, m_selectedPlanet, jd);

    // 横画面では操作UIを出さず惑星表示のみにするため、テキストオーバーレイを省略する
    const bool landscape = w() > h();

    // ステータステキスト（上部左）
    if (!landscape) {
        p.setPen(QColor(150,155,165));
        p.setFont(QFont("Arial", 9));
        p.drawText(8, 18, m_dateTime.toString("yyyy-MM-dd  hh:mm  UTC"));
        const QString centerName = (m_centerBody < 0)
            ? "太陽" : getPlanets()[m_centerBody].name;
        const QString sunLockTag = (m_dirLock == DirLock::SunFront) ? "   [前面=太陽]"
                                 : (m_dirLock == DirLock::SunBack)  ? "   [背面=太陽]"
                                 : "";
        p.drawText(8, 34, QString("中心: %1   距離 %2 AU   仰角 %3°%4")
                   .arg(centerName).arg(m_camDist,0,'f',1).arg(m_camLat,0,'f',1).arg(sunLockTag));

        // ヒント（下部）— モバイル向けテキスト
        p.drawText(8, h()-8, "タッチ: 視点回転   ピンチ: ズーム   タップ: 天体情報");
    }

    // 距離スケールバー（右下）
    {
        const float pxPerAU = 1.732f * std::min(w(), h()) * 0.45f / (float)m_camDist;
        static const double niceVals[] = {
            0.00002, 0.00005,
            0.0001,  0.0002,  0.0005,
            0.001,   0.002,   0.005,
            0.01,    0.02,    0.05,
            0.1,     0.2,     0.5,
            1, 2, 5, 10, 20, 50, 100, 200, 500
        };
        double scaleAU = niceVals[0];
        float  bestDiff = 1e9f;
        for (double v : niceVals) {
            float diff = std::abs((float)v * pxPerAU - 80.0f);
            if (diff < bestDiff) { bestDiff = diff; scaleAU = v; }
        }
        const float barPx = (float)scaleAU * pxPerAU;
        const int   by    = h() - 24;
        const int   bx    = w() - (int)barPx - 24;

        p.setPen(QPen(QColor(150, 155, 165), 1.5f));
        p.drawLine(bx,             by, bx + (int)barPx, by);
        p.drawLine(bx,             by - 4, bx,             by + 4);
        p.drawLine(bx + (int)barPx, by - 4, bx + (int)barPx, by + 4);

        p.setFont(QFont("Arial", 8));
        p.setPen(QColor(150, 155, 165));
        QString scaleLabel;
        if (scaleAU >= 1.0) {
            scaleLabel = QString::number((int)scaleAU) + " AU";
        } else if (scaleAU >= 0.1) {
            scaleLabel = QString::number(scaleAU) + " AU";
        } else {
            const double wankm = scaleAU * 14959.787;
            if      (wankm >= 10.0) scaleLabel = QString("%1 万km").arg(qRound(wankm));
            else if (wankm >=  1.0) scaleLabel = QString("%1 万km").arg(wankm, 0, 'f', 1);
            else                    scaleLabel = QString("%1 万km").arg(wankm, 0, 'f', 2);
        }
        p.drawText(QRectF(bx, by - 17, barPx, 14), Qt::AlignCenter, scaleLabel);
    }
}
