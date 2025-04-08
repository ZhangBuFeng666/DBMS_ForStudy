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

MenuWindow::MenuWindow(GameClient *client,QWidget *parent) : QWidget(parent)  {

    this->client = client;

    setWindowTitle("菜单");
    setFixedSize(350, 500); // 调整窗口大小

    // 加载背景图片
    QPalette palette;
    QPixmap background(":/images/images/menu.jpg");
    palette.setBrush(QPalette::Window, background);
    this->setPalette(palette);

    selectModeButton = new QPushButton("选择模式", this);
    selectModeButton->setGeometry(100,100,150,50);
    helpButton = new QPushButton("帮助", this);
    helpButton->setGeometry(100,200,150,50);
    viewRecordsButton = new QPushButton("查看纪录", this);
    viewRecordsButton->setGeometry(100,300,150,50);
    exitButton = new QPushButton("退出", this);
    exitButton->setGeometry(100,400,150,50);


    connect(selectModeButton, &QPushButton::clicked, this, &MenuWindow::onSelectModeClicked);
    connect(helpButton, &QPushButton::clicked, this, &MenuWindow::onHelpClicked);
    connect(viewRecordsButton, &QPushButton::clicked, this, &MenuWindow::onViewRecordsClicked);
    connect(exitButton, &QPushButton::clicked, qApp, &QApplication::quit);
}

SelectModeWindow::SelectModeWindow(QWidget *parent) : QWidget(parent) {
    setWindowTitle("模式选择");
    setFixedSize(200,150);
    difficultyLabel = new QLabel("难度:", this);
    difficultyCombo = new QComboBox(this);
    difficultyCombo->addItems({"小试牛刀", "渐入佳境", "炉火纯青"});

    modeLabel = new QLabel("模式:", this);
    modeCombo = new QComboBox(this);
    modeCombo->addItems({"单人模式", "双人模式"});

    statusLabel = new QLabel("正在匹配...", this);

    startButton = new QPushButton("开始", this);


    // 加载背景图片
    QPalette palette;
    QPixmap background(":/images/images/choice.jpg");
    palette.setBrush(QPalette::Window, background);
    this->setPalette(palette);

    difficultyLabel->setGeometry(30,20,30,20);
    modeLabel->setGeometry(30,60,30,20);
    difficultyCombo->setGeometry(70,20,100,20);
    modeCombo->setGeometry(70,60,100,20);
    startButton->setGeometry(50,100,100,40);
    statusLabel->setStyleSheet("QLabel { background-color: white; }");
    statusLabel->setGeometry(40, 0, 120, 20);
    statusLabel->setText("   匹配中，请稍后--");

    connect(startButton, &QPushButton::clicked, this, &SelectModeWindow::onStartClicked);
}

HelpWindow::HelpWindow(QWidget *parent) : QWidget(parent) {
    layout = new QVBoxLayout(this);
    helpText = new QTextEdit(this);
    helpText->setReadOnly(true);

    layout->addWidget(helpText);
    setLayout(layout);

    // 从文件加载帮助文本
    QFile file(":images/images/help.txt");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        helpText->setText(in.readAll());
        file.close();
    } else {
        QMessageBox::warning(this, "Error", "加载文件失败");
    }
}

RecordWindow::RecordWindow(GameClient *client, QWidget *parent) : QWidget(parent), client(client) {
    setWindowTitle("查看记录");
    setFixedSize(400, 200);

    // 加载背景图片
    QPalette palette;
    QPixmap background(":/images/images/record.jpg");
    palette.setBrush(QPalette::Window, background);
    this->setPalette(palette);

    recordText = new QTextEdit(this);
    recordText->setReadOnly(true);
    recordText->setGeometry(230,30,100,150);

    // 调用函数加载记录
    loadRecords();
}

void RecordWindow::loadRecords() {

    // 从 GameClient 获取记录
    QVector<qint64> records = client->requestPersonalRecord();
    if (!records.isEmpty()) {
        QString displayText;

        displayText += QString("小试牛刀: "+gamePage::formatTime(records[0])+ "\n");
        displayText += QString("渐入佳境: "+gamePage::formatTime(records[1])+ "\n");
        displayText += QString("炉火纯青: "+gamePage::formatTime(records[2])+ "\n");

        recordText->setText(displayText);
    } else {
        recordText->setText("服务器异常");
    }
}

MainController::MainController() {

    client = new GameClient("127.0.0.1",6666);
    loginWindow = new LoginWindow(client);
    menuWindow = new MenuWindow(client);
    selectModeWindow = new SelectModeWindow;
    helpWindow = new HelpWindow;
    recordWindow = new RecordWindow(client);


    // 初始显示登录窗口
    loginWindow->show();

    // 界面切换信号槽连接
    connect(loginWindow, &LoginWindow::loginSuccess, this, &MainController::showMenuWindow);
    connect(menuWindow, &MenuWindow::selectMode, this, &MainController::showSelectModeWindow);
    connect(selectModeWindow, &SelectModeWindow::startGame, this, &MainController::startGameWindow);
    connect(menuWindow, &MenuWindow::askHelp, this, &MainController::showHelpWindow);
    connect(menuWindow, &MenuWindow::viewRecords, this, &MainController::showRecordWindow);
}

MainController::~MainController() {
    delete loginWindow;
    delete menuWindow;
    delete selectModeWindow;
}

void MainController::hideAllWindows() {
    loginWindow->hide();
    menuWindow->hide();
    selectModeWindow->hide();
}

void MainController::showMenuWindow() {
    hideAllWindows();
    menuWindow->show();
}

void MainController::showSelectModeWindow() {
    hideAllWindows();

    //隐藏匹配提示
    selectModeWindow->statusLabel->hide();
    // 创建一个事件循环，处理当前窗口的事件
    QEventLoop loop;
    loop.processEvents(QEventLoop::AllEvents, 1);  // 只处理当前窗口的事件

    selectModeWindow->show();
}
void MainController::showHelpWindow(){
    helpWindow->show();
}
void MainController::showRecordWindow(){
    delete recordWindow;
    recordWindow= new RecordWindow(client);
    recordWindow->show();
}


void MainController::startGameWindow(){
    int diff = selectModeWindow->difficultyCombo->currentIndex();
    int mode = selectModeWindow->modeCombo->currentIndex();
    if(mode){//若多人模式发送请求并阻塞
        selectModeWindow->statusLabel->show();
        // 创建一个事件循环，只处理当前窗口的事件
        QEventLoop loop;
        loop.processEvents(QEventLoop::AllEvents, 1);  // 只处理当前窗口的事件

        client->sendMatchRequest(diff);
    }
    //单人模式直接hide，多人模式匹配好后hide
    hideAllWindows();

    if(gameWindow)delete gameWindow;
    gameWindow = new GameWindow(mode,diff,client);
    gameWindow->show();

    connect(gameWindow, &gamePage::gameOver, this, &MainController::showOverWindow);//connect的（1，3）指针参数必须指向对象

}



void MainController::showOverWindow(int totalElapsedMs,int completed){
    hideAllWindows();
    delete gameOverWindow;
    if(completed)
        gameOverWindow = new GameOverPage(totalElapsedMs,0);
    else
        gameOverWindow = new GameOverPage(totalElapsedMs,1);

    gameOverWindow->show();
    connect(gameOverWindow, &GameOverWindow::homeRequested, this, &MainController::showMenuWindow);
    //connect(gameOverWindow, &GameOverWindow::retryRequested, this, &MainController::startGameWindow);

}
