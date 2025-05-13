#ifndef MYWINDOWS_H
#define MYWINDOWS_H

#include <QMainWindow>
#include <QMenuBar>
#include <QToolBar>
#include <QStackedWidget>
#include <QListWidget>
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
#include <QTreeWidget>
#include <QFile>
#include <QFileDialog>
#include <QCheckBox>
#include "InternetConnector.h"

class HistoryItemWidget : public QWidget {
    Q_OBJECT
public:
    explicit HistoryItemWidget(const QString &text, QWidget *parent = nullptr);

    QLabel *label;
    QPushButton *closeBtn;

signals:
    void requestClose();  // 发出“请求关闭”的信号
};

class LoginWindow : public QWidget {
    Q_OBJECT
public:
    LoginWindow(InternetConnector *connector,QWidget *parent = nullptr);

signals:
    void loginSuccess();

private slots:
    void onLoginClicked() {

        if(connector->login(usernameEdit->text(),passwordEdit->text())){
            emit loginSuccess(); // 暂时直接触发登录成功信号
        }
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
    void updateDBTree();


signals:
    void clickExit();

private slots:
    //命令行模式下检测‘;’截断命令
    void checkForCmdEnd();
    //处理两种模式命令输入
    //文本用
    void executeScriptOrder();
    void saveFile();
    void textChanged(){
        fileChanged =1;
    }
    //命令行用
    void switchCmdMode();
    void switchScriptMode();
    // void closeTab(int index);

    //刷新历史记录（及其中条目内容）
    void refreshAllHistory();
    void refreshThisHistory();
    void clearHistory();

    //用户手动退出主界面（跳转登录）
    void ExitLogin();

    //双击树状图节点
        void onTreeItemDoubleClicked(QTreeWidgetItem *item);

private:

    bool fileChanged = 0;
    QString curFileName;

    void createWindow();
    void createLayout();
    // void sendToServer(const QString& cmd);

    enum Mode { CMD_MODE, SCRIPT_MODE };
    Mode currentMode;

    // UI组件
    QListWidget *historyList;          // 右侧历史记录
    QPushButton *refreshAllBtn;
    QPushButton *refreshThisBtn;
    QPushButton *clearHistoryBtn;
    QStackedWidget *resultStack;       // 显示各个查询结果
    QMap<int, QString> commandMap;     // 保存历史记录文本（可选）
    QWidget *inputWidget;
    QTextEdit  *cmdInput;
    QLabel *cur_file;
    QTextEdit *scriptInput;
    QPushButton *sendBtn;
    QPushButton *saveBtn;

    // 图形化相关组件
    QTreeWidget *dbTreeView;              // 左侧树状图
    QTableWidget *queryResultTable;      // 下部结果表格

    // 图形化操作建表按钮
    QPushButton *createTableBtn;
    QPushButton *alterTableBtn;
    QPushButton *dropTableBtn;

    // 图形化建表操作槽函数
    void showCreateTableDialog();
    void showAlterTableDialog();
    void showDropTableDialog();


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
    void executeCmdOrder(int status,QString& command,int index=0);

    //网络连接类
    InternetConnector *connector;

    //历史记录存储
    QVector<QString> historyCommands;  // 存所有命令

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
    void showLoginWindow();

    LoginWindow *loginWindow;
    MyMainWindow *mainWindow;

    //新加部分
    InternetConnector *connector;
    friend class LoginWindow;
    friend class MyMainWindow;
};
#endif // MYWINDOWS_H
