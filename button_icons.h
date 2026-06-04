#pragma once
#include <QIcon>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include <cmath>

// デスクトップ版ツールバーボタン用アイコン生成（QPainter による図形描画）
// Android 版 QML Canvas の icn*() 関数群と同じ形状定義を使用

namespace ButtonIcons {

static constexpr double PI = 3.14159265358979323846;
static inline QColor col() { return QColor("#cccccc"); }

template<typename DrawFn>
static inline QIcon make(int sz, DrawFn fn)
{
    QPixmap pix(sz, sz);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    fn(p, sz);
    return QIcon(pix);
}

// ▶ 再生
inline QIcon play(int sz = 20)
{
    return make(sz, [](QPainter &p, int s) {
        p.setBrush(col()); p.setPen(Qt::NoPen);
        QPolygonF t;
        t << QPointF(s*0.25, s*0.13) << QPointF(s*0.82, s*0.50) << QPointF(s*0.25, s*0.87);
        p.drawPolygon(t);
    });
}

// ⏸ 一時停止
inline QIcon pause(int sz = 20)
{
    return make(sz, [](QPainter &p, int s) {
        p.setBrush(col()); p.setPen(Qt::NoPen);
        p.drawRect(QRectF(s*0.22, s*0.13, s*0.22, s*0.74));
        p.drawRect(QRectF(s*0.56, s*0.13, s*0.22, s*0.74));
    });
}

// |◀ コマ戻し
inline QIcon stepBack(int sz = 20)
{
    return make(sz, [](QPainter &p, int s) {
        p.setBrush(col()); p.setPen(Qt::NoPen);
        p.drawRect(QRectF(s*0.10, s*0.15, s*0.14, s*0.70));
        QPolygonF t;
        t << QPointF(s*0.88, s*0.15) << QPointF(s*0.30, s*0.50) << QPointF(s*0.88, s*0.85);
        p.drawPolygon(t);
    });
}

// ▶| コマ進め
inline QIcon stepForward(int sz = 20)
{
    return make(sz, [](QPainter &p, int s) {
        p.setBrush(col()); p.setPen(Qt::NoPen);
        QPolygonF t;
        t << QPointF(s*0.12, s*0.15) << QPointF(s*0.70, s*0.50) << QPointF(s*0.12, s*0.85);
        p.drawPolygon(t);
        p.drawRect(QRectF(s*0.76, s*0.15, s*0.14, s*0.70));
    });
}

// 🕐 現在時刻（時計）
inline QIcon clockNow(int sz = 20)
{
    return make(sz, [](QPainter &p, int s) {
        QPointF c(s*0.50, s*0.50);
        double r = s * 0.42;
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(col(), s*0.09, Qt::SolidLine, Qt::RoundCap));
        p.drawEllipse(c, r, r);
        double ha = -60.0 * PI / 180.0;
        p.setPen(QPen(col(), s*0.11, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(c, QPointF(c.x() + r*0.45*std::cos(ha), c.y() + r*0.45*std::sin(ha)));
        double ma = -90.0 * PI / 180.0;
        p.setPen(QPen(col(), s*0.09, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(c, QPointF(c.x() + r*0.65*std::cos(ma), c.y() + r*0.65*std::sin(ma)));
        p.setBrush(col()); p.setPen(Qt::NoPen);
        p.drawEllipse(c, s*0.07, s*0.07);
    });
}

// 📅 日時（カレンダー）
inline QIcon calendar(int sz = 20)
{
    return make(sz, [](QPainter &p, int s) {
        double m = s*0.08, bx = m, by = m + s*0.12;
        double bw = s - m*2, bh = s*0.80 - m, hdr = bh*0.32;
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(col(), s*0.07, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawRect(QRectF(bx, by, bw, bh));
        p.drawLine(QPointF(bx, by+hdr), QPointF(bx+bw, by+hdr));
        p.setPen(QPen(col(), s*0.09, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(bx+bw*0.28, by-s*0.10), QPointF(bx+bw*0.28, by+s*0.06));
        p.drawLine(QPointF(bx+bw*0.72, by-s*0.10), QPointF(bx+bw*0.72, by+s*0.06));
        p.setBrush(col()); p.setPen(Qt::NoPen);
        double dr = s*0.07, gx0 = bx+bw*0.18, gy0 = by+hdr+(bh-hdr)*0.28;
        double gsx = bw*0.32, gsy = (bh-hdr)*0.45;
        for (int c2 = 0; c2 < 3; ++c2)
            for (int r2 = 0; r2 < 2; ++r2)
                p.drawEllipse(QPointF(gx0+c2*gsx, gy0+r2*gsy), dr, dr);
    });
}

// ⚙ 歯車（設定）
inline QIcon gear(int sz = 20)
{
    return make(sz, [](QPainter &p, int s) {
        QPointF c(s*0.50, s*0.50);
        double Ro = s*0.46, Ri = s*0.34, Rh = s*0.17;
        const int N = 8;
        double dA = PI / N;
        QPainterPath outer;
        for (int i = 0; i < N; ++i) {
            double a0 = 2*PI*i/N - dA*0.45, a1 = 2*PI*i/N - dA*0.20;
            double a2 = 2*PI*i/N + dA*0.20, a3 = 2*PI*i/N + dA*0.45;
            auto pt = [&](double r, double a){ return QPointF(c.x()+r*std::cos(a), c.y()+r*std::sin(a)); };
            if (i == 0) outer.moveTo(pt(Ri, a0)); else outer.lineTo(pt(Ri, a0));
            outer.lineTo(pt(Ro, a1)); outer.lineTo(pt(Ro, a2)); outer.lineTo(pt(Ri, a3));
        }
        outer.closeSubpath();
        QPainterPath hole; hole.addEllipse(c, Rh, Rh);
        p.setBrush(col()); p.setPen(Qt::NoPen);
        p.drawPath(outer.subtracted(hole));
    });
}

// 合・衝（惑星直列：太陽＋惑星を水平線で結ぶ）
inline QIcon conjunction(int sz = 20)
{
    return make(sz, [](QPainter &p, int s) {
        p.setPen(QPen(col(), s*0.08, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(s*0.05, s*0.50), QPointF(s*0.95, s*0.50));
        p.setBrush(col()); p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(s*0.23, s*0.50), s*0.14, s*0.14);
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(col(), s*0.08));
        p.drawEllipse(QPointF(s*0.72, s*0.50), s*0.20, s*0.20);
    });
}

} // namespace ButtonIcons
