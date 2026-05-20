#pragma once
#include "icon_draw.h"
#include <QIcon>
#include <QPixmap>

// 太陽系アイコンを実行時に生成して返す
inline QIcon createAppIcon()
{
    QIcon icon;
    for (int size : {16, 32, 48, 256})
        icon.addPixmap(QPixmap::fromImage(drawPlanetIcon(size)));
    return icon;
}
