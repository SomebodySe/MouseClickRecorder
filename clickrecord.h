#pragma once
#include <QPoint>
#include <QString>
#include <QVector>

struct ClickRecord
{
    QString name;
    QString hotkey;
    QVector<QPoint> points;
};