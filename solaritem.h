#pragma once
#include <QQuickPaintedItem>
#include <QVector3D>
#include <QDateTime>

struct PlanetInfo;

class SolarItem : public QQuickPaintedItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool      showLabels       READ showLabels       WRITE setShowLabels       NOTIFY showLabelsChanged)
    Q_PROPERTY(bool      showDwarfPlanets READ showDwarfPlanets WRITE setShowDwarfPlanets NOTIFY showDwarfPlanetsChanged)
    Q_PROPERTY(bool      showSatellites   READ showSatellites   WRITE setShowSatellites   NOTIFY showSatellitesChanged)
    Q_PROPERTY(int       centerBody       READ centerBody       WRITE setCenterBody       NOTIFY centerBodyChanged)
    Q_PROPERTY(int       selectedPlanet   READ selectedPlanet                             NOTIFY selectedPlanetChanged)
    Q_PROPERTY(int       dirLock          READ dirLockInt       WRITE setDirLockInt       NOTIFY dirLockChanged)
    Q_PROPERTY(double    camLon           READ camLon                                     NOTIFY cameraChanged)
    Q_PROPERTY(double    camLat           READ camLat                                     NOTIFY cameraChanged)
    Q_PROPERTY(double    camDist          READ camDist                                    NOTIFY cameraChanged)
    Q_PROPERTY(QDateTime dateTime         READ dateTime         WRITE setDateTime         NOTIFY dateTimeChanged)

public:
    enum class DirLock { None, SunFront, SunBack };

    explicit SolarItem(QQuickItem *parent = nullptr);

    void paint(QPainter *painter) override;

    bool      showLabels()       const { return m_showLabels; }
    bool      showDwarfPlanets() const { return m_showDwarfPlanets; }
    bool      showSatellites()   const { return m_showSatellites; }
    int       centerBody()       const { return m_centerBody; }
    int       selectedPlanet()   const { return m_selectedPlanet; }
    DirLock   dirLock()          const { return m_dirLock; }
    int       dirLockInt()       const { return (int)m_dirLock; }
    double    camLon()           const { return m_camLon; }
    double    camLat()           const { return m_camLat; }
    double    camDist()          const { return m_camDist; }
    QDateTime dateTime()         const { return m_dateTime; }

    void setShowLabels(bool show);
    void setShowDwarfPlanets(bool show);
    void setShowSatellites(bool show);
    void setCenterBody(int idx);
    void setDirLock(DirLock mode);
    void setDirLockInt(int v) { setDirLock((DirLock)v); }
    void setDateTime(const QDateTime &dt);

    Q_INVOKABLE void setCamera(double lon, double lat);
    Q_INVOKABLE void setCameraDistance(double dist);
    Q_INVOKABLE void setTrailActive(bool active);
    Q_INVOKABLE void clearTrails();

    // タッチ操作 (QML の Handler から呼ぶ)
    Q_INVOKABLE void rotateDelta(double dx, double dy);
    Q_INVOKABLE void pinchZoom(double scaleDelta);
    Q_INVOKABLE void tap(double x, double y);

signals:
    void showLabelsChanged();
    void showDwarfPlanetsChanged();
    void showSatellitesChanged();
    void centerBodyChanged();
    void selectedPlanetChanged();
    void dirLockChanged();
    void cameraChanged();
    void dateTimeChanged();

private:
    int w() const { return qRound(width()); }
    int h() const { return qRound(height()); }

    QVector3D project(const QVector3D &pos3d) const;
    void drawTrails(QPainter &p, double jd) const;
    void drawOrbit(QPainter &p, int idx, double jd) const;
    void drawOrbit(QPainter &p, const PlanetInfo &planet, double jd) const;
    void drawSaturnRings(QPainter &p, const QVector3D &satPos3d,
                         QPointF sc, bool frontHalf) const;
    void drawPlanet(QPainter &p, int idx, double jd, bool showLabel) const;
    void drawInfoPanel(QPainter &p, int idx, double jd) const;
    void drawRotationAxis(QPainter &p, int idx, double jd) const;
    int  hitTest(double x, double y, double jd) const;

    double    m_camLon = 30.0, m_camLat = 30.0, m_camDist = 50.0;
    QDateTime m_dateTime;
    bool      m_showLabels       = true;
    bool      m_showDwarfPlanets = false;
    bool      m_showSatellites   = false;
    DirLock   m_dirLock          = DirLock::None;
    int       m_centerBody       = -1;
    int       m_selectedPlanet   = -1;
    QVector3D m_centerOffset;

    bool   m_trailActive  = false;
    double m_trailStartJD = 0.0;
    static constexpr double TRAIL_ORBITS = 0.8;
    static constexpr int    TRAIL_STEPS  = 180;
};
