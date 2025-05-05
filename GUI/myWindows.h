#ifndef MYWINDOWS_H
#define MYWINDOWS_H

#include <QMainWindow>
#include <QMenuBar>
#include <QStatusBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QSplitter>
#include <QActionGroup>
#include <QApplication>
#include <QMessageBox>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QWidget>
#include <QFile>
#include <QFileDialog>
#include "InternetConnector.h"

class LoginWindow : public QWidget {
    Q_OBJECT
public:
    LoginWindow(InternetConnector *connector,QWidget *parent = nullptr);

signals:
    void loginSuccess();

private slots:
    void onLoginClicked() {

        if(connector->login(usernameEdit->text(),passwordEdit->text()))
            emit loginSuccess(); // 暂时直接触发登录成功信号
        else{
            passwordLabel->setText("密码:(密码错误)");
        }

    }

private:
    QVBoxLayout *layout;
    QLabel *usernameLabel ;
    QLineEdit *usernameEdit;
    QLabel *passwordLabel ;
    QLineEdit *passwordEdit;
    QPushButton *loginButton;
    InternetConnector *connector;
};


class QTextEdit;
class QPushButton;

class MyMainWindow : public QMainWindow {
    Q_OBJECT
public:
    MyMainWindow(InternetConnector *connector,QWidget *parent = nullptr);

private slots:
    //命令行模式下检测‘;’截断命令
    void checkForCmdEnd();
    //处理两种模式命令输入
    //文本用
    void executeScriptOrder();
    void saveFile();
    //命令行用
    void switchCmdMode();
    void switchScriptMode();
    void closeTab(int index);

private:

    bool fileChanged = false;
    QString curFileName;

    void createWindow();
    void createLayout();
    // void sendToServer(const QString& cmd);

    enum Mode { CMD_MODE, SCRIPT_MODE };
    Mode currentMode;

    // UI组件
    QTabWidget *resultTabs;
    QWidget *inputArea;
    QTextEdit  *cmdInput;
    QTextEdit *scriptInput;
    QPushButton *sendBtn;
    QPushButton *saveBtn;

    // 菜单项
    QMenu *fileMenu;
    QMenu *modeMenu;
    //QAction *newSessionAction;
    QActionGroup *fileGroup;
    QActionGroup *modeGroup;

    //文件操作
    void openFile();
    void saveFileAs();
    void newFile();

    //页面操作
    void executeCmdOrder(QString& command);

    //网络连接类
    InternetConnector *connector;
};



class MainController : public QObject {
    Q_OBJECT
public:
    MainController() ;

    ~MainController() ;

private slots:
    void showMyMainWindow();

private:
    void hideAllWindows();

    LoginWindow *loginWindow;
    MyMainWindow *mainWindow;

    //新加部分
    InternetConnector *connector;
    friend class LoginWindow;
    friend class MyMainWindow;
};
#endif // MYWINDOWS_H
