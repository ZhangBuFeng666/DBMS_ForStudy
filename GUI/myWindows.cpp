#include "myWindows.h"

LoginWindow::LoginWindow(InternetConnector *connector,QWidget *parent) : QWidget(parent) {

    this->connector = connector;

    setWindowTitle("登录");
    setFixedSize(500, 250);

    // 加载背景图片
    // QPalette palette;
    // QPixmap background(":/images/images/Register.jpg");
    // palette.setBrush(QPalette::Window, background);
    // this->setPalette(palette);

    QLabel* tip=new QLabel("",this);
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
    loginButton = new QPushButton("登录", this);
    loginButton->setGeometry(60,150,100,30);

    connect(loginButton, &QPushButton::clicked, this, &LoginWindow::onLoginClicked);
}



void MyMainWindow::createLayout() {
    // 设置初始窗口大小为 800x600
    resize(800, 600);

    // 主分割器
    QSplitter *mainSplitter = new QSplitter(Qt::Vertical);

    // 结果展示区域（标签页(含输入语句展示+状态展示+结果（比如表）展示)）
    resultTabs = new QTabWidget();
    resultTabs->setTabsClosable(true);
    resultTabs->setMovable(true);

    // 输入区域
    inputArea = new QWidget();
    QVBoxLayout *inputLayout = new QVBoxLayout;

    // 命令行模式组件
    cmdInput = new QTextEdit();
    cmdInput->setPlaceholderText("Enter command, use ';' to execute.");

    // 模拟命令行行为
    //connect(cmdInput, &QTextEdit::textChanged, this, &MyMainWindow::checkForCmdEnd);

    // 脚本模式组件
    scriptInput = new QTextEdit();
    scriptInput->setPlaceholderText("Enter multiple commands (one per line)");
    scriptInput->hide();

    sendBtn = new QPushButton("运行");
    sendBtn->hide();
    saveBtn = new QPushButton("保存");
    saveBtn->hide();

    inputLayout->addWidget(cmdInput);
    inputLayout->addWidget(scriptInput);
    inputLayout->addWidget(sendBtn);
    inputLayout->addWidget(saveBtn);

    inputArea->setLayout(inputLayout);

    // 添加到主分割器
    mainSplitter->addWidget(inputArea);
    mainSplitter->addWidget(resultTabs);

    // 设置结果区域占下半部分，输入区域占上半部分
    mainSplitter->setSizes(QList<int>() << 300 << 300);  // 让两部分等分，400:400比例

    setCentralWidget(mainSplitter);
}

void MyMainWindow::createWindow() {
    // 文件菜单
    fileMenu = menuBar()->addMenu(tr("&文件"));
    QAction *newScriptFile=fileMenu->addAction(tr("新建"));
    QAction *openScriptFile=fileMenu->addAction("打开");
    QAction *saveScriptFile=fileMenu->addAction("另存为");

    connect(openScriptFile, &QAction::triggered, this, &MyMainWindow::openFile);
    connect(saveScriptFile, &QAction::triggered, this, &MyMainWindow::saveFileAs);
    connect(newScriptFile, &QAction::triggered, this, &MyMainWindow::newFile);
    // 模式菜单
    modeMenu = menuBar()->addMenu(tr("&模式"));
    modeGroup = new QActionGroup(this);
    QAction *cmdModeAction = modeMenu->addAction("命令行模式");
    QAction *scriptModeAction = modeMenu->addAction("文本编辑模式");
    cmdModeAction->setCheckable(true);
    scriptModeAction->setCheckable(true);
    modeGroup->addAction(cmdModeAction);
    modeGroup->addAction(scriptModeAction);
    cmdModeAction->setChecked(true);

    connect(cmdModeAction, &QAction::triggered, this, &MyMainWindow::switchCmdMode);
    connect(scriptModeAction, &QAction::triggered, this, &MyMainWindow::switchScriptMode);
}

void MyMainWindow::checkForCmdEnd() {
    QString command = cmdInput->toPlainText().trimmed();
    if (command.endsWith(";")) {
        executeCmdOrder(command);
    }
}
void MyMainWindow::saveFile(){
    if(curFileName==""){
        saveFileAs();
    }else{
        QFile file(curFileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            // 将 QTextEdit 中的内容写入文件
            QTextStream out(&file);
            out << scriptInput->toPlainText();
            file.close();
            fileChanged=false;
        } else {
            QMessageBox::warning(this, "错误", "内定文件不存在");
        }
    }
}
void MyMainWindow::executeScriptOrder() {
    QString oriCommand = scriptInput->toPlainText().trimmed();
    // 获取输入的命令并去掉首尾空格
    oriCommand.replace("\n", " ");  // 替换换行符为空格
    oriCommand = oriCommand.trimmed();  // （文本编辑模式）去除转化出来的尾部空格

    if (oriCommand.endsWith(";")) {
        oriCommand.chop(1);
        QStringList commands = oriCommand.split(';');
        for(QString& command:commands){
            executeCmdOrder(command);//复用命令行命令执行函数
        }
        // 清空输入框，准备下一个命令
        cmdInput->clear();
    }else if(oriCommand==""){
        QMessageBox::warning(this, "空白", "请输入命令");
    }else{
        QMessageBox::warning(this, "非法结尾", "命令必须以\';\'结尾");
    }
}
void MyMainWindow::executeCmdOrder(QString& command) {
    // 获取输入的命令并去掉首尾空格
    command.replace("\n", " ");  // 替换换行符为空格

    // 发送命令、接收响应
    connector->sendOrder(command);
    QJsonObject response = connector->receiveMassage();
    // QString responseText = QString(QJsonDocument(response).toJson(QJsonDocument::Indented));

    // 获取命令第一个单词作为标签页标题
    QStringList commandParts = command.split(' ', Qt::SkipEmptyParts);
    QString commandTitle = commandParts.isEmpty() ? "Unknown Command" : commandParts.first();

    // 创建新的标签页 Widget
    QWidget *resultWidget = new QWidget();
    QVBoxLayout* resultLayout = new QVBoxLayout(resultWidget);
    resultLayout->setSpacing(5);
    resultLayout->setContentsMargins(5, 5, 5, 5);

    // 顶部显示命令原文
    resultLayout->addWidget(new QLabel(command+"   ("+response["Status"].toString()+")"));

    // 创建表格
    QTableWidget* table = new QTableWidget();
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->verticalHeader()->setVisible(false);
    table->setAlternatingRowColors(true);

    // 尝试从 response["Mass"] 中解析表格数据
    if (response.contains("Mass") && response["Mass"].isObject()) {
        QJsonObject massObj = response["Mass"].toObject();
        if (massObj.contains("Header") && massObj.contains("Data")) {
            QJsonArray headers = massObj["Header"].toArray();
            QJsonArray data = massObj["Data"].toArray();

            table->setColumnCount(headers.size());
            QStringList headerLabels;
            for (const auto& h : headers) {
                headerLabels << h.toString();
            }
            table->setHorizontalHeaderLabels(headerLabels);

            table->setRowCount(data.size());
            for (int i = 0; i < data.size(); ++i) {
                QJsonArray row = data[i].toArray();
                for (int j = 0; j < row.size(); ++j) {
                    table->setItem(i, j, new QTableWidgetItem(row[j].toString()));
                }
            }
        }
    }

    // 表格占据剩余空间
    resultLayout->addWidget(table);

    // 添加到标签页
    resultTabs->addTab(resultWidget, commandTitle);

    // 清空输入框，准备下一个命令
    cmdInput->clear();

}

void MyMainWindow::switchCmdMode() {
    currentMode = CMD_MODE;
    scriptInput->hide();
    sendBtn->hide();
    saveBtn->hide();
    cmdInput->show();
    cmdInput->setFocus();
}

void MyMainWindow::switchScriptMode() {
    currentMode = SCRIPT_MODE;
    cmdInput->hide();
    scriptInput->show();
    sendBtn->show();
    saveBtn->show();
    scriptInput->setFocus();
}

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
    createWindow();
    createLayout();

    // 连接信号槽
    connect(cmdInput,&QTextEdit::textChanged,this,&MyMainWindow::checkForCmdEnd);
    connect(sendBtn, &QPushButton::clicked, this, &MyMainWindow::executeScriptOrder);
    connect(saveBtn, &QPushButton::clicked, this, &MyMainWindow::saveFile);
    connect(resultTabs, &QTabWidget::tabCloseRequested, this, &MyMainWindow::closeTab);
}



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

void MyMainWindow::openFile() {
    // 弹出文件选择对话框
    QString fileName = QFileDialog::getOpenFileName(this, "打开文件", "", "SQL Files (*.sql);;Text Files (*.txt);;All Files (*)");

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            //保存临时文件路径
            curFileName=fileName;
            // 读取文件内容到 QTextEdit
            QTextStream in(&file);
            scriptInput->setPlainText(in.readAll());
            file.close();
        } else {
            QMessageBox::warning(this, "错误", "无法打开文件");
        }
    }
}
void MyMainWindow::saveFileAs() {
    // 弹出保存文件对话框，默认保存为 .sql 文件
    QString fileName = QFileDialog::getSaveFileName(this, "另存为", "", "SQL Files (*.sql);;All Files (*)");

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        //保存临时文件路径
        curFileName=fileName;
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            // 将 QTextEdit 中的内容写入文件
            QTextStream out(&file);
            out << scriptInput->toPlainText();
            file.close();
        } else {
            QMessageBox::warning(this, "错误", "无法保存文件");
        }
    }
}
void MyMainWindow::newFile() {
    // 如果当前有文件内容，询问用户是否保存
    if (fileChanged) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "保存当前文件", "是否保存当前文件？",
                                      QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

        if (reply == QMessageBox::Yes) {
            saveFileAs(); // 调用另存为功能保存文件
        } else if (reply == QMessageBox::Cancel) {
            return; // 如果选择取消，则不进行新建操作
        }
    }

    // 清空文本框内容，准备新建文件
    scriptInput->clear();

    // 可选：为新文件设置一个默认的文本（例如 "新建文件.sql"）
    scriptInput->setPlainText("-- 请输入SQL语句");
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
// void MyMainWindow::newSession()
// {
//     // 清理所有结果标签页
//     while (resultTabs->count() > 0) {
//         QWidget* tab = resultTabs->widget(0);
//         tab->deleteLater();
//         resultTabs->removeTab(0);
//     }

//     // 创建初始标签页
//     QTextEdit* defaultResult = new QTextEdit();
//     defaultResult->setReadOnly(true);
//     resultTabs->addTab(defaultResult, tr("Session 1"));
//     resultTabs->setCurrentIndex(0);

//     // 重置输入区域
//     cmdInput->clear();
//     scriptInput->clear();
//     switchCmdMode();

//     // 状态栏提示
//     statusBar()->showMessage(tr("New session created"), 2000);
// }


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
