#pragma once
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QScrollArea>
#include <QWidget>
#include <QVector>
#include <QTimer>
#include "clickrecord.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

QT_BEGIN_NAMESPACE
namespace Ui { class MouseClickRecorderClass; }
QT_END_NAMESPACE

class MouseClickRecorder : public QMainWindow
{
    Q_OBJECT
public:
    explicit MouseClickRecorder(QWidget* parent = nullptr);
    ~MouseClickRecorder();

private:
    Ui::MouseClickRecorderClass* ui;
    QWidget* centralWidget;
    QVBoxLayout* mainLayout;
    QVBoxLayout* recordLayout;
    QWidget* recordContainer;
    QScrollArea* scrollArea;
    QPushButton* addButton;
    QSpinBox* pressTimeSpinBox;
    QSpinBox* intervalSpinBox;
    QVector<ClickRecord> records;
    int clickPressTime = 20;
    int clickInterval = 50;

    QTimer* clickTimer;
    int executingRecordIndex = -1;
    int executingPointIndex = 0;
    bool mouseDown = false;
    QPoint originalMousePos;

    void executeRecord(int index);
    void executeNextPoint();

    void saveConfig();
    void loadConfig();

    void addRecord();
    void refreshRecords();
    void editRecord(int index);
};