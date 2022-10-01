#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <windows.h>
#include <QSystemTrayIcon>
#include <QApplication>
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
    void writeIni(const QString &key, const QVariant &value);
    void readIni(void);

private:
    Ui::Widget* ui;

    HWND hWndTarget = NULL;
    const QString ChromeClass = "Chrome_WidgetWin_1";
    const QString iniPath = QApplication::applicationDirPath() + "/settings.ini";
    QSystemTrayIcon* sysTray = nullptr;
    qreal scaleRatio = 1.0;
    int TAB_H = 34; //px

    bool edge = true;
    bool chrome = true;

    // QWidget interface
protected:
    bool
    nativeEvent(const QByteArray& eventType, void* message, long* result) override;
};
#endif // WIDGET_H
