#ifndef MYWINDOWS_H
#define MYWINDOWS_H

#include <QMainWindow>
#include <QMenuBar>
#include <QStatusBar>
#include <QTabWidget>
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
#include "InternetConnector.h"
// 假设已经完成的游戏窗口类
// using GameWindow = gamePage;
// using GameOverWindow = GameOverPage;


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

// class MyMainWindow : public QWidget {
//     Q_OBJECT
// public:
//     MyMainWindow(InternetConnector *connector,QWidget *parent = nullptr);

// signals:
//     void selectMode();
//     void askHelp();
//     void viewRecords();

// private slots:
//     void onSelectModeClicked() {
//         emit selectMode();
//     }
//     void onHelpClicked() {
//         emit askHelp();
//     }
//     void onViewRecordsClicked() {
//         emit viewRecords();
//     }


// private:
//     InternetConnector *connector;
//     QVBoxLayout *layout;
//     QPushButton *selectModeButton;
//     QPushButton *helpButton;
//     QPushButton *viewRecordsButton;
//     QPushButton *exitButton;
// mainwindow.h


class QLineEdit;
class QTextEdit;
class QPushButton;

class MyMainWindow : public QMainWindow {
    Q_OBJECT
public:
    MyMainWindow(InternetConnector *connector,QWidget *parent = nullptr);

private slots:
    void handleCommand();
    void executeScript();
    void switchCmdMode();
    void switchScriptMode();
    void newSession();
    void closeTab(int index);

    // void saveSession(int sessionId);//保存会话快照
    // void loadSession(const QString& sessionFile);//载入会话快照

private:
    void createMenu();
    void createLayout();
    // void sendToServer(const QString& cmd);

    enum Mode { CMD_MODE, SCRIPT_MODE };
    Mode currentMode;

    // UI组件
    QTabWidget *resultTabs;
    QWidget *inputArea;
    QLineEdit *cmdInput;
    QTextEdit *scriptInput;
    QPushButton *submitBtn;

    // 菜单项
    QMenu *fileMenu;
    QMenu *modeMenu;
    QAction *newSessionAction;
    QActionGroup *modeGroup;

    //网络连接类
    InternetConnector *connector;
};
// };

// class SelectModeWindow : public QWidget {
//     Q_OBJECT
// public:
//     SelectModeWindow(QWidget *parent = nullptr);

// signals:
//     void startGame();

// private slots:
//     void onStartClicked() {
//         emit startGame();
//     }

// private:
//     QVBoxLayout *layout;
//     QLabel *difficultyLabel;
//     QComboBox *difficultyCombo;
//     QLabel *modeLabel;
//     QComboBox *modeCombo;
//     QLabel *statusLabel;
//     QPushButton *startButton;

//     friend class MainController;
// };
// class HelpWindow : public QWidget {
//     Q_OBJECT
// public:
//     HelpWindow(QWidget *parent = nullptr);
// private:
//     QVBoxLayout *layout;
//     QTextEdit *helpText;

// };

// class RecordWindow : public QWidget {
//     Q_OBJECT
// public:
//     RecordWindow(InternetConnector *connector, QWidget *parent = nullptr);
// private:
//     void loadRecords() ;
//     QVBoxLayout *layout;
//     QTextEdit *recordText;
//     InternetConnector *connector;
// };

// class GameOverWindow : public QWidget {
//     Q_OBJECT
// public:
//     GameOverWindow(QWidget *parent = nullptr);

// signals:
//     void returnToMenu();
//     void playAgain();

// private slots:
//     void onHomeClicked() {
//         emit returnToMenu();
//     }

//     void onRetryClicked() {
//         emit playAgain();
//     }
// };


class MainController : public QObject {
    Q_OBJECT
public:
    MainController() ;

    ~MainController() ;

private slots:
    void showMyMainWindow();
    //void showSelectModeWindow();
    //void showHelpWindow();
    //void showRecordWindow();
    //void startGameWindow();
    //void showOverWindow(int totalElapsedMs,int completed);

    //void showGameOverWindow() ;
    //void createAndShowGameWindow();
    //void onGameWindowClosed();


private:
    void hideAllWindows();

    LoginWindow *loginWindow;
    MyMainWindow *mainWindow;
    //HelpWindow *helpWindow;
    //SelectModeWindow *selectModeWindow;


    // GameWindow *gameWindow = nullptr;
    // GameOverWindow *gameOverWindow = nullptr;

    //RecordWindow *recordWindow;


    //新加部分
    InternetConnector *connector;
    friend class LoginWindow;
    friend class MyMainWindow;
};
#endif // MYWINDOWS_H
