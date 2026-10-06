#include "MouseClickRecorder.h"
#include "ui_MouseClickRecorder.h"
#include "EditWindow.h"

MouseClickRecorder::MouseClickRecorder(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MouseClickRecorderClass)
{
    ui->setupUi(this);
    if (menuBar())
        menuBar()->hide();
    if (statusBar())
        statusBar()->hide();

    setWindowFlag(Qt::WindowStaysOnTopHint, true);
    setWindowTitle("Clicker");
    resize(100, 100);

    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(10, 4, 10, 4);
    mainLayout->setSpacing(8);

    QHBoxLayout* topLayout = new QHBoxLayout();

    QLabel* pressLabel = new QLabel("按下", centralWidget);
    pressTimeSpinBox = new QSpinBox(centralWidget);
    pressTimeSpinBox->setRange(1, 10000);
    pressTimeSpinBox->setValue(clickPressTime);
    pressTimeSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    pressTimeSpinBox->setFixedWidth(40);

    QLabel* intervalLabel = new QLabel("间隔", centralWidget);
    intervalSpinBox = new QSpinBox(centralWidget);
    intervalSpinBox->setRange(1, 10000);
    intervalSpinBox->setValue(clickInterval);
    intervalSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    intervalSpinBox->setFixedWidth(40);

    addButton = new QPushButton("+", centralWidget);
    addButton->setFixedSize(30, 30);

    topLayout->addWidget(pressLabel);
    topLayout->addWidget(pressTimeSpinBox);
    topLayout->addSpacing(6);
    topLayout->addWidget(intervalLabel);
    topLayout->addWidget(intervalSpinBox);
    topLayout->addStretch();
    topLayout->addWidget(addButton);

    mainLayout->addLayout(topLayout);

    scrollArea = new QScrollArea(centralWidget);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    scrollArea->setMinimumHeight(0);

    recordContainer = new QWidget();
    recordContainer->setMinimumSize(0, 0);

    recordLayout = new QVBoxLayout(recordContainer);
    recordLayout->setContentsMargins(0, 0, 0, 0);
    recordLayout->setSpacing(6);

    scrollArea->setWidget(recordContainer);
    mainLayout->addWidget(scrollArea);

    connect(addButton, &QPushButton::clicked, this, &MouseClickRecorder::addRecord);

    connect(pressTimeSpinBox, &QSpinBox::valueChanged, this, [this](int value) {
        clickPressTime = value;
        saveConfig();
        });
    connect(intervalSpinBox, &QSpinBox::valueChanged, this, [this](int value) {
        clickInterval = value;
        saveConfig();
        });

    clickTimer = new QTimer(this);
    clickTimer->setSingleShot(true);

    connect(clickTimer, &QTimer::timeout, this, &MouseClickRecorder::executeNextPoint);

    setMinimumWidth(minimumSizeHint().width());
    setMinimumHeight(120);
    resize(minimumSizeHint().width(), 120);  

    loadConfig();
}

MouseClickRecorder::~MouseClickRecorder()
{
    delete ui;
}

void MouseClickRecorder::addRecord()
{
    ClickRecord record;
    record.name = QString("记录 %1").arg(records.size() + 1);
    record.hotkey = "Ctrl+Shift+G";
    records.append(record);
    refreshRecords();
    saveConfig();
}

void MouseClickRecorder::refreshRecords()
{
    while (QLayoutItem* item = recordLayout->takeAt(0)) {
        if (QWidget* widget = item->widget())
            widget->deleteLater();
        delete item;
    }

    for (int i = 0; i < records.size(); ++i) {
        QWidget* row = new QWidget(recordContainer);
        row->setMinimumHeight(24);

        QHBoxLayout* layout = new QHBoxLayout(row);
        layout->setContentsMargins(5, 0, 5, 0);

        QLabel* nameLabel = new QLabel(records[i].name, row);
        QPushButton* executeButton = new QPushButton("执行", row);
        QPushButton* editButton = new QPushButton("编辑", row);

        executeButton->setFixedWidth(60);
        editButton->setFixedWidth(60);

        layout->addWidget(nameLabel);
        layout->addStretch();
        layout->addWidget(executeButton);
        layout->addWidget(editButton);

        connect(editButton, &QPushButton::clicked, this, [this, i]() {
            editRecord(i);
            });
        connect(executeButton, &QPushButton::clicked, this, [this, i]() {
            executeRecord(i);
            });

        recordLayout->addWidget(row);
    }

}

void MouseClickRecorder::editRecord(int index)
{
    if (index < 0 || index >= records.size())
        return;

    EditWindow window(&records[index], this);
    window.setWindowFlag(Qt::WindowStaysOnTopHint, true);

    connect(&window, &EditWindow::deleteRequested, this, [this, index]() {
        if (index >= 0 && index < records.size()) {
            records.removeAt(index);
            saveConfig();
        }
        });

    window.exec();
    refreshRecords();
    saveConfig();
}

void MouseClickRecorder::saveConfig()
{
    QJsonObject root;
    root["clickPressTime"] = clickPressTime;
    root["clickInterval"] = clickInterval;

    QJsonArray recordArray;

    for (const ClickRecord& record : records) {
        QJsonObject recordObject;
        recordObject["name"] = record.name;
        recordObject["hotkey"] = record.hotkey;

        QJsonArray pointArray;

        for (const QPoint& point : record.points) {
            QJsonObject pointObject;
            pointObject["x"] = point.x();
            pointObject["y"] = point.y();
            pointArray.append(pointObject);
        }

        recordObject["points"] = pointArray;
        recordArray.append(recordObject);
    }

    root["records"] = recordArray;

    QFile file("record_conf.txt");

    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();
}

void MouseClickRecorder::loadConfig()
{
    QFile file("record_conf.txt");

    if (!file.open(QIODevice::ReadOnly))
        return;

    QJsonParseError error;
    QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();

    if (error.error != QJsonParseError::NoError || !document.isObject())
        return;

    QJsonObject root = document.object();

    clickPressTime = root["clickPressTime"].toInt(20);
    clickInterval = root["clickInterval"].toInt(50);

    pressTimeSpinBox->setValue(clickPressTime);
    intervalSpinBox->setValue(clickInterval);

    records.clear();

    QJsonArray recordArray = root["records"].toArray();

    for (const QJsonValue& recordValue : recordArray) {
        QJsonObject recordObject = recordValue.toObject();

        ClickRecord record;
        record.name = recordObject["name"].toString();
        record.hotkey = recordObject["hotkey"].toString();

        if (record.name.isEmpty())
            record.name = QString("记录 %1").arg(records.size() + 1);

        if (record.hotkey.isEmpty())
            record.hotkey = "Ctrl+Shift+G";

        QJsonArray pointArray = recordObject["points"].toArray();

        for (const QJsonValue& pointValue : pointArray) {
            QJsonObject pointObject = pointValue.toObject();

            int x = pointObject["x"].toInt();
            int y = pointObject["y"].toInt();

            record.points.append(QPoint(x, y));
        }

        records.append(record);
    }

    refreshRecords();
}

void MouseClickRecorder::executeRecord(int index)
{
    if (index < 0 || index >= records.size())
        return;

    if (records[index].points.isEmpty())
        return;

    if (clickTimer->isActive() || mouseDown)
        return;

    executingRecordIndex = index;
    executingPointIndex = 0;
    mouseDown = false;

    executeNextPoint();
}

void MouseClickRecorder::executeNextPoint()
{
    if (executingRecordIndex < 0 || executingRecordIndex >= records.size()) {
        mouseDown = false;
        return;
    }

    const QVector<QPoint>& points = records[executingRecordIndex].points;

    if (executingPointIndex >= points.size()) {
        mouseDown = false;
        executingRecordIndex = -1;
        return;
    }

    const QPoint& point = points[executingPointIndex];

    SetCursorPos(point.x(), point.y());

    INPUT input = {};
    input.type = INPUT_MOUSE;

    if (!mouseDown) {
        input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        SendInput(1, &input, sizeof(INPUT));

        mouseDown = true;
        clickTimer->start(clickPressTime);
    }
    else {
        input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
        SendInput(1, &input, sizeof(INPUT));

        mouseDown = false;
        ++executingPointIndex;
        clickTimer->start(clickInterval);
    }
}