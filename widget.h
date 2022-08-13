#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <windows.h>
#include <QSystemTrayIcon>
QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();
    void initSysTray(void);

private:
    Ui::Widget* ui;

    HWND hWndChrome = NULL;
    const QString ChromeClass = "Chrome_WidgetWin_1";
    QSystemTrayIcon* sysTray = nullptr;

    // QWidget interface
protected:
    bool
    nativeEvent(const QByteArray& eventType, void* message, long* result) override;
};
#endif // WIDGET_H
