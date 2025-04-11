#include "myWindows.h"

LoginWindow::LoginWindow(InternetConnector *connector,QWidget *parent) : QWidget(parent) {

    this->connector = connector;

    setWindowTitle("登陆/注册");
    setFixedSize(500, 250);

    // 加载背景图片
    QPalette palette;
    QPixmap background(":/images/images/Register.jpg");
    palette.setBrush(QPalette::Window, background);
    this->setPalette(palette);

    QLabel* tip=new QLabel("(ID未注册将自动注册)",this);
    tip->setGeometry(180,50,150,20);
    usernameLabel = new QLabel("ID:", this);
    usernameLabel->setGeometry(20,50,30,20);
    usernameEdit = new QLineEdit(this);
    usernameEdit->setGeometry(50,50,130,20);
    passwordLabel = new QLabel("密码:", this);
    passwordLabel->setGeometry(20,100,30,20);
    passwordEdit = new QLineEdit(this);
    passwordEdit->setGeometry(50,100,130,20);
    passwordEdit->setEchoMode(QLineEdit::Password);
    loginButton = new QPushButton("登陆/注册", this);
    loginButton->setGeometry(60,150,100,30);

    connect(loginButton, &QPushButton::clicked, this, &LoginWindow::onLoginClicked);
}

// MyMainWindow::MyMainWindow(InternetConnector *connector,QWidget *parent) : QWidget(parent)  {

//     this->connector = connector;

//     setWindowTitle("菜单");
//     setFixedSize(350, 500); // 调整窗口大小

//     // 加载背景图片
//     QPalette palette;
//     QPixmap background(":/images/images/menu.jpg");
//     palette.setBrush(QPalette::Window, background);
//     this->setPalette(palette);

//     selectModeButton = new QPushButton("选择模式", this);
//     selectModeButton->setGeometry(100,100,150,50);
//     helpButton = new QPushButton("帮助", this);
//     helpButton->setGeometry(100,200,150,50);
//     viewRecordsButton = new QPushButton("查看纪录", this);
//     viewRecordsButton->setGeometry(100,300,150,50);
//     exitButton = new QPushButton("退出", this);
//     exitButton->setGeometry(100,400,150,50);


//     connect(selectModeButton, &QPushButton::clicked, this, &MyMainWindow::onSelectModeClicked);
//     connect(helpButton, &QPushButton::clicked, this, &MyMainWindow::onHelpClicked);
//     connect(viewRecordsButton, &QPushButton::clicked, this, &MyMainWindow::onViewRecordsClicked);
//     connect(exitButton, &QPushButton::clicked, qApp, &QApplication::quit);
// }

void MyMainWindow::createLayout() {
    // 主分割器
    QSplitter *mainSplitter = new QSplitter(Qt::Vertical);

    // 结果展示区域（标签页）
    resultTabs = new QTabWidget();
    resultTabs->setTabsClosable(true);
    resultTabs->setMovable(true);

    // 输入区域
    inputArea = new QWidget();
    QVBoxLayout *inputLayout = new QVBoxLayout;

    // 命令行模式组件
    cmdInput = new QLineEdit();
    cmdInput->setPlaceholderText("Enter command (use ; to separate multiple commands)");

    // 脚本模式组件
    scriptInput = new QTextEdit();
    scriptInput->setPlaceholderText("Enter multiple commands (one per line)");
    scriptInput->hide();

    submitBtn = new QPushButton("Execute Script");
    submitBtn->hide();

    inputLayout->addWidget(cmdInput);
    inputLayout->addWidget(scriptInput);
    inputLayout->addWidget(submitBtn);
    inputArea->setLayout(inputLayout);

    mainSplitter->addWidget(resultTabs);
    mainSplitter->addWidget(inputArea);
    setCentralWidget(mainSplitter);
}
void MyMainWindow::createMenu() {
    // 文件菜单
    fileMenu = menuBar()->addMenu(tr("&File"));
    newSessionAction = fileMenu->addAction(tr("New Session"));
    connect(newSessionAction, &QAction::triggered, this, &MyMainWindow::newSession);

    // 模式菜单
    modeMenu = menuBar()->addMenu(tr("&Mode"));
    modeGroup = new QActionGroup(this);
    QAction *cmdModeAction = modeMenu->addAction("Command Line Mode");
    QAction *scriptModeAction = modeMenu->addAction("Script Mode");
    cmdModeAction->setCheckable(true);
    scriptModeAction->setCheckable(true);
    modeGroup->addAction(cmdModeAction);
    modeGroup->addAction(scriptModeAction);
    cmdModeAction->setChecked(true);

    connect(cmdModeAction, &QAction::triggered, this, &MyMainWindow::switchCmdMode);
    connect(scriptModeAction, &QAction::triggered, this, &MyMainWindow::switchScriptMode);
}

void MyMainWindow::switchCmdMode() {
    currentMode = CMD_MODE;
    scriptInput->hide();
    submitBtn->hide();
    cmdInput->show();
    cmdInput->setFocus();
}

void MyMainWindow::switchScriptMode() {
    currentMode = SCRIPT_MODE;
    cmdInput->hide();
    scriptInput->show();
    submitBtn->show();
    scriptInput->setFocus();
}

void MyMainWindow::newSession()
{
    // 清理所有结果标签页
    while (resultTabs->count() > 0) {
        QWidget* tab = resultTabs->widget(0);
        tab->deleteLater();
        resultTabs->removeTab(0);
    }

    // 创建初始标签页
    QTextEdit* defaultResult = new QTextEdit();
    defaultResult->setReadOnly(true);
    resultTabs->addTab(defaultResult, tr("Session 1"));
    resultTabs->setCurrentIndex(0);

    // 重置输入区域
    cmdInput->clear();
    scriptInput->clear();
    switchCmdMode();

    // 状态栏提示
    statusBar()->showMessage(tr("New session created"), 2000);
}

void MyMainWindow::handleCommand() {
    QString input = cmdInput->text().trimmed();
    if(input.isEmpty()) return;

    // 命令行模式处理
    if(currentMode == CMD_MODE && input.contains(';')) {
        QStringList commands = input.split(';', Qt::SkipEmptyParts);
        for(const QString &cmd : commands) {
            connector->sendOrder(cmd.trimmed());
        }
        cmdInput->clear();
        QJsonObject response = connector->receiveMassage();

    }
}

void MyMainWindow::executeScript() {
    QStringList commands = scriptInput->toPlainText().split('\n', Qt::SkipEmptyParts);
    for(const QString &cmd : commands) {
        connector->sendOrder(cmd.trimmed());
    }
}

// void MyMainWindow::sendToServer(const QString &cmd) {
//     // 创建新标签页
//     QTextEdit *resultView = new QTextEdit();
//     resultView->setReadOnly(true);
//     int tabIndex = resultTabs->addTab(resultView, cmd.left(15));
//     resultTabs->setCurrentIndex(tabIndex);

//     // 模拟后端响应（实际应使用网络模块）
//     // QTimer::singleShot(1000, [=](){
//     //     resultView->append("Result for: " + cmd + "\n" + QString::number(qrand()));
//     // });
// }

void MyMainWindow::closeTab(int index) {
    if(resultTabs->count() > 1) {
        QWidget *tab = resultTabs->widget(index);
        tab->deleteLater();
        resultTabs->removeTab(index);
    }
}
MyMainWindow::MyMainWindow(InternetConnector *connector,QWidget *parent)
    : QMainWindow(parent), currentMode(CMD_MODE)
{
    this->connector = connector;
    createMenu();
    createLayout();

    // 连接信号槽
    connect(cmdInput, &QLineEdit::returnPressed, this, &MyMainWindow::handleCommand);
    connect(submitBtn, &QPushButton::clicked, this, &MyMainWindow::executeScript);
    connect(resultTabs, &QTabWidget::tabCloseRequested, this, &MyMainWindow::closeTab);
}

// SelectModeWindow::SelectModeWindow(QWidget *parent) : QWidget(parent) {
//     setWindowTitle("模式选择");
//     setFixedSize(200,150);
//     difficultyLabel = new QLabel("难度:", this);
//     difficultyCombo = new QComboBox(this);
//     difficultyCombo->addItems({"小试牛刀", "渐入佳境", "炉火纯青"});

//     modeLabel = new QLabel("模式:", this);
//     modeCombo = new QComboBox(this);
//     modeCombo->addItems({"单人模式", "双人模式"});

//     statusLabel = new QLabel("正在匹配...", this);

//     startButton = new QPushButton("开始", this);


//     // 加载背景图片
//     QPalette palette;
//     QPixmap background(":/images/images/choice.jpg");
//     palette.setBrush(QPalette::Window, background);
//     this->setPalette(palette);

//     difficultyLabel->setGeometry(30,20,30,20);
//     modeLabel->setGeometry(30,60,30,20);
//     difficultyCombo->setGeometry(70,20,100,20);
//     modeCombo->setGeometry(70,60,100,20);
//     startButton->setGeometry(50,100,100,40);
//     statusLabel->setStyleSheet("QLabel { background-color: white; }");
//     statusLabel->setGeometry(40, 0, 120, 20);
//     statusLabel->setText("   匹配中，请稍后--");

//     connect(startButton, &QPushButton::clicked, this, &SelectModeWindow::onStartClicked);
// }

// HelpWindow::HelpWindow(QWidget *parent) : QWidget(parent) {
//     layout = new QVBoxLayout(this);
//     helpText = new QTextEdit(this);
//     helpText->setReadOnly(true);

//     layout->addWidget(helpText);
//     setLayout(layout);

//     // 从文件加载帮助文本
//     QFile file(":images/images/help.txt");
//     if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
//         QTextStream in(&file);
//         helpText->setText(in.readAll());
//         file.close();
//     } else {
//         QMessageBox::warning(this, "Error", "加载文件失败");
//     }
// }

// RecordWindow::RecordWindow(InternetConnector *con, QWidget *parent) : QWidget(parent), connector(con) {
//     setWindowTitle("查看记录");
//     setFixedSize(400, 200);

//     // 加载背景图片
//     QPalette palette;
//     QPixmap background(":/images/images/record.jpg");
//     palette.setBrush(QPalette::Window, background);
//     this->setPalette(palette);

//     recordText = new QTextEdit(this);
//     recordText->setReadOnly(true);
//     recordText->setGeometry(230,30,100,150);

//     // 调用函数加载记录
//     //loadRecords();
// }

// void RecordWindow::loadRecords() {

//     // 从 GameClient 获取记录
//     QVector<qint64> records = client->requestPersonalRecord();
//     if (!records.isEmpty()) {
//         QString displayText;

//         displayText += QString("小试牛刀: "+gamePage::formatTime(records[0])+ "\n");
//         displayText += QString("渐入佳境: "+gamePage::formatTime(records[1])+ "\n");
//         displayText += QString("炉火纯青: "+gamePage::formatTime(records[2])+ "\n");

//         recordText->setText(displayText);
//     } else {
//         recordText->setText("服务器异常");
//     }
// }

MainController::MainController() {

    connector = new InternetConnector("127.0.0.1",6666);
    loginWindow = new LoginWindow(connector);
    mainWindow = new MyMainWindow(connector);
    // selectModeWindow = new SelectModeWindow;
    // helpWindow = new HelpWindow;
    // recordWindow = new RecordWindow(connector);


    // 初始显示登录窗口
    loginWindow->show();

    // 界面切换信号槽连接
    connect(loginWindow, &LoginWindow::loginSuccess, this, &MainController::showMyMainWindow);
    // connect(menuWindow, &MyMainWindow::selectMode, this, &MainController::showSelectModeWindow);
    // connect(selectModeWindow, &SelectModeWindow::startGame, this, &MainController::startGameWindow);
    // connect(menuWindow, &MyMainWindow::askHelp, this, &MainController::showHelpWindow);
    // connect(menuWindow, &MyMainWindow::viewRecords, this, &MainController::showRecordWindow);
}

MainController::~MainController() {
    delete loginWindow;
    delete mainWindow;
    // delete selectModeWindow;
}

void MainController::hideAllWindows() {
    loginWindow->hide();
    mainWindow->hide();
    // selectModeWindow->hide();
}

void MainController::showMyMainWindow() {
    hideAllWindows();
    mainWindow->show();
}

// void MainController::showSelectModeWindow() {
//     hideAllWindows();

//     //隐藏匹配提示
//     selectModeWindow->statusLabel->hide();
//     // 创建一个事件循环，处理当前窗口的事件
//     QEventLoop loop;
//     loop.processEvents(QEventLoop::AllEvents, 1);  // 只处理当前窗口的事件

//     selectModeWindow->show();
// }
// void MainController::showHelpWindow(){
//     helpWindow->show();
// }
// void MainController::showRecordWindow(){
//     delete recordWindow;
//     recordWindow= new RecordWindow(connector);
//     recordWindow->show();
// }


// void MainController::startGameWindow(){
//     int diff = selectModeWindow->difficultyCombo->currentIndex();
//     int mode = selectModeWindow->modeCombo->currentIndex();
//     if(mode){//若多人模式发送请求并阻塞
//         selectModeWindow->statusLabel->show();
//         // 创建一个事件循环，只处理当前窗口的事件
//         QEventLoop loop;
//         loop.processEvents(QEventLoop::AllEvents, 1);  // 只处理当前窗口的事件

//         client->sendMatchRequest(diff);
//     }
//     //单人模式直接hide，多人模式匹配好后hide
//     hideAllWindows();

//     if(gameWindow)delete gameWindow;
//     gameWindow = new GameWindow(mode,diff,client);
//     gameWindow->show();

//     connect(gameWindow, &gamePage::gameOver, this, &MainController::showOverWindow);//connect的（1，3）指针参数必须指向对象

// }



// void MainController::showOverWindow(int totalElapsedMs,int completed){
//     hideAllWindows();
//     delete gameOverWindow;
//     if(completed)
//         gameOverWindow = new GameOverPage(totalElapsedMs,0);
//     else
//         gameOverWindow = new GameOverPage(totalElapsedMs,1);

//     gameOverWindow->show();
//     connect(gameOverWindow, &GameOverWindow::homeRequested, this, &MainController::showMyMainWindow);
//     //connect(gameOverWindow, &GameOverWindow::retryRequested, this, &MainController::startGameWindow);

// }
