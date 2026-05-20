#pragma once
#include <QWidget>
#include <QVector3D>
#include <QDateTime>

struct PlanetInfo;

class SolarWidget : public QWidget
{
    Q_OBJECT

public:
    enum class DirLock { None, SunFront, SunBack };

    explicit SolarWidget(QWidget *parent = nullptr);

    void setDateTime(const QDateTime &dt);
    QDateTime dateTime() const { return m_dateTime; }

    void setTrailActive(bool active);  // 再生開始/停止時に呼ぶ
    void clearTrails();                // 手動時刻変更時に呼ぶ

    // -1 = 太陽中心, 0-7 = 惑星中心
    void setCenterBody(int idx);

    // カメラ角度を直接設定（視点プリセット用）
    void setCamera(double lon, double lat);
    void setCameraDistance(double dist);

    // 表示設定の setter（設定復元用）
    void setShowLabels(bool show);
    void setShowDwarfPlanets(bool show);
    void setShowSatellites(bool show);
    void setDirLock(DirLock mode);

    // 設定保存用 getter
    double camLon()           const { return m_camLon; }
    double camLat()           const { return m_camLat; }
    double camDist()          const { return m_camDist; }
    int    centerBody()       const { return m_centerBody; }
    bool   showLabels()       const { return m_showLabels; }
    bool   showDwarfPlanets() const { return m_showDwarfPlanets; }
    bool   showSatellites()   const { return m_showSatellites; }
    DirLock dirLock()          const { return m_dirLock; }

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void contextMenuEvent(QContextMenuEvent *) override;

private:
    // ---- カメラ ----
    double m_camLon, m_camLat, m_camDist;
    QPoint m_dragStartPos;
    QPoint m_lastMousePos;
    bool   m_dragging = false;

    // ---- 時刻 ----
    QDateTime m_dateTime;

    // ---- 軌跡 ----
    static constexpr double TRAIL_ORBITS = 0.8;  // 表示する公転数
    static constexpr int    TRAIL_STEPS  = 180;  // 描画ステップ数（固定）
    bool   m_trailActive  = false;
    double m_trailStartJD = 0.0;   // 再生開始時の JD（軌跡の下限）

    // ---- 中心天体 ----
    int        m_centerBody   = -1;       // -1 = 太陽
    QVector3D  m_centerOffset;            // 中心天体の黄道座標（フレームごとに更新）

    // ---- 表示設定 ----
    bool m_showLabels       = true;
    bool m_showDwarfPlanets = false;
    bool m_showSatellites   = false;
    DirLock m_dirLock       = DirLock::None;

    // ---- 選択惑星 ----
    int m_selectedPlanet = -1;

    // ---- 内部メソッド ----
    QVector3D project(const QVector3D &pos3d) const;

    void drawTrails(QPainter &p, double jd) const;
    void drawOrbit(QPainter &p, int idx, double jd) const;
    void drawOrbit(QPainter &p, const PlanetInfo &planet, double jd) const;
    void drawSaturnRings(QPainter &p, const QVector3D &satPos3d,
                         QPointF sc, bool frontHalf) const;
    void drawPlanet(QPainter &p, int idx, double jd, bool showLabel) const;
    void drawInfoPanel(QPainter &p, int idx, double jd) const;
    void drawRotationAxis(QPainter &p, int idx, double jd) const;

    int hitTest(const QPoint &mp, double jd) const;
};
