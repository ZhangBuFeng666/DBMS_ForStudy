#ifndef MYWINDOWS_H
#define MYWINDOWS_H

#include <QApplication>
#include <QMainWindow>
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

class MenuWindow : public QWidget {
    Q_OBJECT
public:
    MenuWindow(InternetConnector *connector,QWidget *parent = nullptr);

signals:
    void selectMode();
    void askHelp();
    void viewRecords();

private slots:
    void onSelectModeClicked() {
        emit selectMode();
    }
    void onHelpClicked() {
        emit askHelp();
    }
    void onViewRecordsClicked() {
        emit viewRecords();
    }


private:
    InternetConnector *connector;
    QVBoxLayout *layout;
    QPushButton *selectModeButton;
    QPushButton *helpButton;
    QPushButton *viewRecordsButton;
    QPushButton *exitButton;

};

class SelectModeWindow : public QWidget {
    Q_OBJECT
public:
    SelectModeWindow(QWidget *parent = nullptr);

signals:
    void startGame();

private slots:
    void onStartClicked() {
        emit startGame();
    }

private:
    QVBoxLayout *layout;
    QLabel *difficultyLabel;
    QComboBox *difficultyCombo;
    QLabel *modeLabel;
    QComboBox *modeCombo;
    QLabel *statusLabel;
    QPushButton *startButton;

    friend class MainController;
};
class HelpWindow : public QWidget {
    Q_OBJECT
public:
    HelpWindow(QWidget *parent = nullptr);
private:
    QVBoxLayout *layout;
    QTextEdit *helpText;

};

class RecordWindow : public QWidget {
    Q_OBJECT
public:
    RecordWindow(InternetConnector *connector, QWidget *parent = nullptr);
private:
    void loadRecords() ;
    QVBoxLayout *layout;
    QTextEdit *recordText;
    InternetConnector *connector;
};

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
    void showMenuWindow();
    void showSelectModeWindow();
    void showHelpWindow();
    void showRecordWindow();
    void startGameWindow();
    void showOverWindow(int totalElapsedMs,int completed);

    //void showGameOverWindow() ;
    //void createAndShowGameWindow();
    //void onGameWindowClosed();


private:
    void hideAllWindows();

    LoginWindow *loginWindow;
    MenuWindow *menuWindow;
    HelpWindow *helpWindow;
    SelectModeWindow *selectModeWindow;


    // GameWindow *gameWindow = nullptr;
    // GameOverWindow *gameOverWindow = nullptr;

    RecordWindow *recordWindow;


    //新加部分
    InternetConnector *connector;
    friend class LoginWindow;
};
#endif // MYWINDOWS_H
