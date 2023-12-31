#include "widget.h"
#include "WinUtility.h"
#include "ui_widget.h"
#include <QDebug>
#include <QLibrary>
#include <QMessageBox>
#include <QTimer>
#include <QMenu>
#include <QAction>
#include <QScreen>
#include <QSettings>
/*
 * Hooks and DLLs: http://www.flounder.com/hooks.htm
 * HOOK API （一）——HOOK基础+一个鼠标钩子实例: https://www.cnblogs.com/fanling999/p/4592740.html
 * Visual Studio关于hook项目的简单使用_吨吨不打野的博客: https://blog.csdn.net/Castlehe/article/details/108275984
*/
Widget::Widget(QWidget* parent)
    : QWidget(parent), ui(new Ui::Widget)
{
    ui->setupUi(this);

    scaleRatio = qApp->primaryScreen()->logicalDotsPerInch() / 96; //high DPI scale
    TAB_H *= scaleRatio;

    readIni();
    initSysTray();

    typedef bool (*HookFunc)(HWND, HWND, DWORD*);
    typedef bool (*UnHookFunc)();
    QLibrary lib(R"(mouseHook.dll)"); //析构不会unload
    if (!lib.load()) //↑打包后 ↓Qt中
        lib.setFileName(R"(D:\Qt\projects\DBClickForChrome\mouseHook\x64\Debug\mouseHook.dll)");
    HookFunc setMouseHook = (HookFunc)lib.resolve("setMouseHook");
    UnHookFunc clearHook = (UnHookFunc)lib.resolve("clearHook");
    clearHook(); //手动清除hWndServer防止Chrome dll未卸载导致共享变量未删除
    //hWndChrome = FindWindow(ChromeClass.toStdWString().c_str(), NULL); //不同窗口都是同一线程 所以都可以Hook接收
    //&很合我意 选项卡以外的标题栏部分(不属于客户区 会触发leave消息) 双击会在MAX & Normal中切换 不会接收鼠标消息 不会关闭选项卡
    //！如果Chrome被结束，则Hook失效 需要重新Hook !!!!!!!!!!!!!!!!!!!!!
    //程序结束后，操作系统隐式释放所有句柄（包括HHOOK），由于是本进程加载dll调用SetHook 所以持有Hook句柄
    //释放后，Chrome的dll也被卸载，猜测：当所有dll卸载后，dll从内存中被清除共享字段清空
    //猜测：Chrome的dll释放有延迟，导致共享字段hWndServer!=NULL导致return false; //可以手动在setHook之前UnHook
    //Error 20报错：原因是Nahimic的声音追踪器 不兼容

    QTimer* timer = new QTimer(this);
    timer->callOnTimeout(this, [=]() {
        static QSet<DWORD> hookedThreads; //不能用HWND 否则会多次注入 导致多次触发（一个Thread可以有多个windows）
        HWND foreWin = GetForegroundWindow();
        QString className = Win::getWindowClass(foreWin);
        QString title = Win::getWindowText(foreWin);
        DWORD threadID = GetWindowThreadProcessId(foreWin, NULL);
        title.replace("\u200B", "");

        qDebug() << foreWin << className << title;
        if (hookedThreads.contains(threadID)) return;
        if (className != ChromeClass) return;
        //if (!(title.contains("Google Chrome") || title.contains("Microsoft Edge"))) return;

        if ((chrome && title.contains("Google Chrome")) || (edge && title.contains("Microsoft Edge"))) {
            DWORD errorCode = 114514;
            if (setMouseHook((HWND)this->winId(), foreWin, &errorCode)) {
                qDebug() << "Hook Successful!" << errorCode << foreWin << title << className;
                hookedThreads << threadID;
                sysTray->showMessage("Tip", "Hacked Chrome | Edge");
            } else {
                qDebug() << "Hook Failed; Code:" << errorCode;
                //QMessageBox::warning(this, "Warning", QString("Hook Failed; Code: %1 ; %2").arg(errorCode).arg(GetLastError()));
            }
        }
    });
    timer->start(1000);

    /*
    do {
        hWndTarget = FindWindow(ChromeClass.toStdWString().c_str(), NULL); //Edge也是这个类名 醉了
        qDebug() << hWndTarget << Win::getWindowClass(hWndTarget) << Win::getWindowText(hWndTarget);
        Sleep(1000);
    } while (!Win::getWindowText(hWndTarget).contains("Google Chrome")); //验明正身 同时防止NULL 否则会给所有线程注入
    //Chrome地址栏上下是两个不同窗口 焦点转移到上方窗口时 WindowText才会包含Google Chrome

    DWORD errorCode = 114514;
    if (setMouseHook((HWND)this->winId(), hWndTarget, &errorCode)) {
        qDebug() << "Hook Successful!" << sizeof(HWND) << errorCode;
        sysTray->showMessage("Tip", "Hacked Chrome");
    } else {
        qDebug() << "Hook Failed; Code:" << errorCode;
        QMessageBox::warning(this, "Warning", QString("Hook Failed; Code: %1 ; %2").arg(errorCode).arg(GetLastError()));
        QTimer::singleShot(0, this, [=]() { qApp->quit(); });
    }*/
}

Widget::~Widget()
{
    delete ui;
}

void Widget::initSysTray()
{
    if (sysTray) return;
    sysTray = new QSystemTrayIcon(this);
    sysTray->setIcon(QIcon(":/Images/ICON_WB.ico"));
    sysTray->setToolTip("DBClick for Chrome & Edge");

    QMenu* menu = new QMenu(this);
    menu->setStyleSheet("QMenu{background-color:rgb(45,45,45);color:rgb(220,220,220);}"
                        "QMenu:selected{background-color:rgb(60,60,60);}");
    QAction* act_edge = new QAction("Edge", menu);
    QAction* act_chrome = new QAction("Chrome", menu);
    QAction* act_quit = new QAction("Quit>>", menu);

    act_edge->setCheckable(true);
    act_edge->setChecked(edge);
    act_chrome->setCheckable(true);
    act_chrome->setChecked(chrome);
    connect(act_quit, &QAction::triggered, qApp, &QApplication::quit);
    connect(act_edge, &QAction::toggled, this, [=](bool checked) {
        writeIni("edge", checked);
        sysTray->showMessage("Tip", "已取消Edge Hook 请重启生效\n2秒后自动退出...");
        QTimer::singleShot(2000, [=]() { qApp->quit(); });
    });
    connect(act_chrome, &QAction::toggled, this, [=](bool checked) {
        writeIni("chrome", checked);
        sysTray->showMessage("Tip", "已取消Chrome Hook 请重启生效\n2秒后自动退出...");
        QTimer::singleShot(2000, [=]() { qApp->quit(); });
    });

    menu->addAction(act_edge);
    menu->addAction(act_chrome);
    menu->addAction(act_quit);

    sysTray->setContextMenu(menu);
    sysTray->show();
}

void Widget::writeIni(const QString& key, const QVariant& value)
{
    QSettings iniSet(iniPath, QSettings::IniFormat);
    iniSet.setValue(key, value);
}

void Widget::readIni()
{
    QSettings iniSet(iniPath, QSettings::IniFormat);
    edge = iniSet.value("edge", true).toBool();
    chrome = iniSet.value("chrome", true).toBool();
}

bool Widget::nativeEvent(const QByteArray& eventType, void* message, long* result) //还需要区分Chrome标题窗口
{
    Q_UNUSED(eventType);
    Q_UNUSED(result);
    MSG* msg = (MSG*)message;
    static const UINT UWM_MOUSEHOOK = RegisterWindowMessageW(L"Chrome_WH_MOUSE");
    if (msg->message == UWM_MOUSEHOOK && msg->wParam == WM_LBUTTONDBLCLK && QCursor::pos().y() <= TAB_H) { //检测标签页高度 防止误触
        HWND hwnd = (HWND)msg->lParam;
        qDebug() << "WM_LBUTTONDBLCLK" << hwnd;
        if (Win::getWindowClass(hwnd) == ChromeClass) {
            Win::simulateKeyEvent(QList<BYTE>({VK_CONTROL, 'W'}));
            return true;
        }
    }
    return false; //此处返回false，留给其他事件处理器处理
}
