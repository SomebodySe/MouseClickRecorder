#include "EditWindow.h"
#include <QMessageBox>
#include <QCloseEvent>
#include <windows.h>
#include <QLineEdit>
#include <QKeySequence>

EditWindow::EditWindow(ClickRecord* record, QWidget* parent)
    : QDialog(parent), record(record)
{
    setWindowTitle("编辑记录");
    setWindowFlag(Qt::WindowStaysOnTopHint, true);
    
    resize(100, 100);

    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(8);

    QHBoxLayout* nameLayout = new QHBoxLayout();
    QLabel* nameLabel = new QLabel("名称：", this);
    nameEdit = new QLineEdit(this);
    nameEdit->setText(this->record->name);
    nameEdit->setPlaceholderText("请输入记录名称");

    nameLayout->addWidget(nameLabel);
    nameLayout->addWidget(nameEdit);
    mainLayout->addLayout(nameLayout);

    QHBoxLayout* hotkeyLayout = new QHBoxLayout();
    hotkeyLabel = new QLabel("快捷键：" + this->record->hotkey, this);
    changeHotkeyButton = new QPushButton("修改快捷键", this);
    changeHotkeyButton->setFixedWidth(100);

    hotkeyLayout->addWidget(hotkeyLabel);
    hotkeyLayout->addStretch();
    hotkeyLayout->addWidget(changeHotkeyButton);
    mainLayout->addLayout(hotkeyLayout);

    countLabel = new QLabel(this);
    mainLayout->addWidget(countLabel);

    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    pointContainer = new QWidget();
    pointLayout = new QVBoxLayout(pointContainer);
    pointLayout->setContentsMargins(5, 5, 5, 5);
    pointLayout->setSpacing(4);

    scrollArea->setWidget(pointContainer);
    mainLayout->addWidget(scrollArea);

    QHBoxLayout* bottomLayout = new QHBoxLayout();
    undoButton = new QPushButton("撤销", this);
    deleteButton = new QPushButton("删除记录", this);
    finishButton = new QPushButton("完成", this);

    bottomLayout->addWidget(undoButton);
    bottomLayout->addWidget(deleteButton);
    bottomLayout->addStretch();
    bottomLayout->addWidget(finishButton);
    mainLayout->addLayout(bottomLayout);

    connect(finishButton, &QPushButton::clicked, this, [this]() {
        QString name = nameEdit->text().trimmed();

        if (name.isEmpty())
            name = "未命名记录";

        this->record->name = name;
        accept();
        });

    connect(undoButton, &QPushButton::clicked, this, [this]() {
        if (!this->record->points.isEmpty()) {
            this->record->points.removeLast();
            refreshPoints();
        }
        });

    connect(deleteButton, &QPushButton::clicked, this, [this]() {
        QMessageBox::StandardButton result = QMessageBox::question(
            this,
            "删除记录",
            "确定要删除这条记录吗？",
            QMessageBox::Yes | QMessageBox::No
        );

        if (result == QMessageBox::Yes) {
            unregisterHotkey();
            emit deleteRequested();
            reject();
        }
        });

    connect(changeHotkeyButton, &QPushButton::clicked, this, [this]() {
        waitingForHotkey = true;
        changeHotkeyButton->setText("请按快捷键");
        hotkeyLabel->setText("快捷键：等待输入...");
        setFocus();
        });


    refreshPoints();
    parseHotkey(this->record->hotkey, hotkeyModifiers, hotkeyKey);
    registerHotkey();

    adjustSize();
    setMinimumSize(minimumSizeHint());
}

EditWindow::~EditWindow()
{
    unregisterHotkey();
}

bool EditWindow::registerHotkey()
{
    if (hotkeyRegistered)
        return true;

    if (RegisterHotKey(
        reinterpret_cast<HWND>(winId()),
        hotkeyId,
        hotkeyModifiers,
        hotkeyKey)) {
        hotkeyRegistered = true;
        return true;
    }

    return false;
}

void EditWindow::unregisterHotkey()
{
    if (hotkeyRegistered) {
        UnregisterHotKey(reinterpret_cast<HWND>(winId()), 1);
        hotkeyRegistered = false;
    }
}

void EditWindow::closeEvent(QCloseEvent* event)
{
    unregisterHotkey();
    QDialog::closeEvent(event);
}

void EditWindow::refreshPoints()
{
    while (QLayoutItem* item = pointLayout->takeAt(0)) {
        if (QWidget* widget = item->widget())
            widget->deleteLater();
        delete item;
    }

    countLabel->setText(QString("已记录 %1 个位置").arg(this->record->points.size()));

    for (int i = 0; i < this->record->points.size(); ++i) {
        const QPoint& point = this->record->points[i];

        QLabel* label = new QLabel(
            QString("%1. X: %2    Y: %3")
            .arg(i + 1)
            .arg(point.x())
            .arg(point.y()),
            pointContainer
        );

        label->setMinimumHeight(28);
        pointLayout->addWidget(label);
    }

    pointLayout->addStretch();
}

bool EditWindow::nativeEvent(const QByteArray& eventType, void* message, qintptr* result)
{
    MSG* msg = static_cast<MSG*>(message);

    if (msg->message == WM_HOTKEY && msg->wParam == 1) {
        POINT point;

        if (GetCursorPos(&point)) {
            record->points.append(QPoint(point.x, point.y));
            refreshPoints();
        }

        *result = 0;
        return true;
    }

    return QDialog::nativeEvent(eventType, message, result);
}

void EditWindow::keyPressEvent(QKeyEvent* event)
{
    if (!waitingForHotkey) {
        QDialog::keyPressEvent(event);
        return;
    }

    UINT newModifiers = 0;

    if (event->modifiers() & Qt::ControlModifier)
        newModifiers |= MOD_CONTROL;
    if (event->modifiers() & Qt::AltModifier)
        newModifiers |= MOD_ALT;
    if (event->modifiers() & Qt::ShiftModifier)
        newModifiers |= MOD_SHIFT;
    if (event->modifiers() & Qt::MetaModifier)
        newModifiers |= MOD_WIN;

    UINT newKey = event->nativeVirtualKey();

    if (newKey == VK_CONTROL || newKey == VK_SHIFT || newKey == VK_MENU ||
        newKey == VK_LWIN || newKey == VK_RWIN)
        return;

    if (newModifiers == 0) {
        QMessageBox::warning(this, "快捷键无效", "请至少使用 Ctrl、Alt、Shift 或 Win 中的一个修饰键。");
        return;
    }

    UINT oldModifiers = hotkeyModifiers;
    UINT oldKey = hotkeyKey;

    if (hotkeyRegistered)
        UnregisterHotKey(reinterpret_cast<HWND>(winId()), hotkeyId);

    hotkeyRegistered = false;

    if (RegisterHotKey(reinterpret_cast<HWND>(winId()), hotkeyId, newModifiers, newKey)) {
        hotkeyModifiers = newModifiers;
        hotkeyKey = newKey;
        hotkeyRegistered = true;
        record->hotkey = QKeySequence(event->modifiers() | event->key()).toString();
        hotkeyLabel->setText("快捷键：" + record->hotkey);
        waitingForHotkey = false;
        changeHotkeyButton->setText("修改快捷键");
    }
    else {
        RegisterHotKey(reinterpret_cast<HWND>(winId()), hotkeyId, oldModifiers, oldKey);
        hotkeyRegistered = true;
        QMessageBox::warning(this, "快捷键注册失败", "这个快捷键可能已经被其他程序占用，原快捷键保持不变。");
        hotkeyLabel->setText("快捷键：" + record->hotkey);
        waitingForHotkey = false;
        changeHotkeyButton->setText("修改快捷键");
    }

    event->accept();
}

bool EditWindow::parseHotkey(const QString& text, UINT& modifiers, UINT& key)
{
    QStringList parts = text.split('+', Qt::SkipEmptyParts);
    if (parts.isEmpty())
        return false;

    modifiers = 0;
    key = 0;

    for (QString part : parts) {
        part = part.trimmed();

        if (part.compare("Ctrl", Qt::CaseInsensitive) == 0)
            modifiers |= MOD_CONTROL;
        else if (part.compare("Alt", Qt::CaseInsensitive) == 0)
            modifiers |= MOD_ALT;
        else if (part.compare("Shift", Qt::CaseInsensitive) == 0)
            modifiers |= MOD_SHIFT;
        else if (part.compare("Win", Qt::CaseInsensitive) == 0)
            modifiers |= MOD_WIN;
        else if (part.size() == 1)
            key = part.at(0).toUpper().unicode();
        else if (part.startsWith("F", Qt::CaseInsensitive)) {
            bool ok = false;
            int number = part.mid(1).toInt(&ok);
            if (ok && number >= 1 && number <= 24)
                key = VK_F1 + number - 1;
        }
    }

    return modifiers != 0 && key != 0;
}