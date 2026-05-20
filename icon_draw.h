#pragma once
#include <QImage>
#include <QPainter>
#include <cmath>

// アプリアイコンの1サイズ分を描画して返す（appicon.h / icon_gen.cpp 共用）
inline QImage drawPlanetIcon(int size)
{
    QImage img(size, size, QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);

    const double cx   = size / 2.0;
    const double cy   = size / 2.0;
    const double unit = size / 2.0;

    // 背景: 深宇宙
    QRadialGradient bg(cx, cy, unit);
    bg.setColorAt(0.0, QColor(10, 15, 40));
    bg.setColorAt(1.0, QColor( 2,  4, 10));
    p.setBrush(bg);
    p.setPen(Qt::NoPen);
    p.drawEllipse(QPointF(cx, cy), unit, unit);

    // 軌道・惑星の定義（内側から: 水星〜土星）
    struct OrbitInfo { double r; QColor col; double dotR; };
    const QList<OrbitInfo> orbits = {
        { 0.18, QColor(180,180,160), 0.040 },  // 水星
        { 0.28, QColor(230,200,100), 0.060 },  // 金星
        { 0.38, QColor( 70,130,200), 0.065 },  // 地球
        { 0.50, QColor(200, 80, 50), 0.050 },  // 火星
        { 0.72, QColor(200,160,100), 0.110 },  // 木星
        { 0.90, QColor(210,190,130), 0.090 },  // 土星
    };
    const QList<double> angles = { 40, 130, 220, 300, 70, 200 };

    // 軌道線
    p.setBrush(Qt::NoBrush);
    for (int i = 0; i < orbits.size(); ++i) {
        if (i >= 5 && size < 48) continue;
        QColor oc = orbits[i].col;
        oc.setAlpha(60);
        p.setPen(QPen(oc, std::max(0.5, size * 0.008)));
        double r = orbits[i].r * unit;
        p.drawEllipse(QPointF(cx, cy), r, r);
    }

    // 太陽
    {
        double r = unit * 0.11;
        QRadialGradient sg(cx, cy, r * 1.8);
        sg.setColorAt(0.0, QColor(255,255,200,200));
        sg.setColorAt(0.5, QColor(255,220, 50,120));
        sg.setColorAt(1.0, QColor(255,150,  0,  0));
        p.setBrush(sg);
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(cx, cy), r * 1.8, r * 1.8);
        p.setBrush(QColor(255,240,80));
        p.drawEllipse(QPointF(cx, cy), r, r);
    }

    // 惑星
    for (int i = 0; i < orbits.size(); ++i) {
        if (i >= 5 && size < 48) continue;
        const double ang = angles[i] * M_PI / 180.0;
        const double pr  = orbits[i].r * unit;
        const double px_ = cx + pr * std::cos(ang);
        const double py_ = cy + pr * std::sin(ang);
        const double dr  = orbits[i].dotR * unit;

        QRadialGradient pg(px_, py_, dr * 2.2);
        QColor gc = orbits[i].col;
        gc.setAlpha(80);
        pg.setColorAt(0.0, gc);
        pg.setColorAt(1.0, Qt::transparent);
        p.setBrush(pg);
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(px_, py_), dr * 2.2, dr * 2.2);
        p.setBrush(orbits[i].col);
        p.drawEllipse(QPointF(px_, py_), dr, dr);
    }

    p.end();
    return img;
}
