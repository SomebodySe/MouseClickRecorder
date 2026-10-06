#pragma once
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QWidget>
#include <QLineEdit>
#include <QKeyEvent>
#include <windows.h>
#include "clickrecord.h"

class EditWindow : public QDialog
{
    Q_OBJECT
public:
    explicit EditWindow(ClickRecord* record, QWidget* parent = nullptr);
    ~EditWindow();
signals:
    void deleteRequested();

protected:
    void closeEvent(QCloseEvent* event) override;
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    ClickRecord* record;
    QVBoxLayout* mainLayout;
    QVBoxLayout* pointLayout;
    QLabel* hotkeyLabel;
    QLabel* countLabel;
    QPushButton* changeHotkeyButton;
    QPushButton* undoButton;
    QPushButton* deleteButton;
    QPushButton* finishButton;
    QScrollArea* scrollArea;
    QWidget* pointContainer;
    QLineEdit* nameEdit;
    UINT hotkeyModifiers = MOD_CONTROL | MOD_SHIFT;
    UINT hotkeyKey = 'G';

    void refreshPoints();
    bool registerHotkey();
    void unregisterHotkey();
    bool parseHotkey(const QString& text, UINT& modifiers, UINT& key);

    bool hotkeyRegistered = false;
    bool waitingForHotkey = false;
    int hotkeyId = 1;
};