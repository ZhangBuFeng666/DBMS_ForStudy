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
    usernameEdit->setGeometry(20,70,130,20);
    passwordLabel = new QLabel("密码:", this);
    passwordLabel->setGeometry(20,100,30,20);
    passwordEdit = new QLineEdit(this);/////////////////////////////////////////////////////////////////
    passwordEdit->setGeometry(20,120,130,20);
    passwordEdit->setEchoMode(QLineEdit::Password);
    loginButton = new QPushButton("登录", this);
    loginButton->setGeometry(60,170,100,30);

    connect(loginButton, &QPushButton::clicked, this, &LoginWindow::onLoginClicked);
    connect(passwordEdit,&QLineEdit::returnPressed, this, &LoginWindow::onLoginClicked);

}


void MyMainWindow::createLayout() {
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    resize(800, 600);

    // 右侧：历史记录列表
    historyList = new QListWidget();
    historyList->setFixedWidth(180);
    // 创建右侧容器
    QWidget *historyPanel = new QWidget();
    QVBoxLayout *historyLayout = new QVBoxLayout(historyPanel);
    refreshBtn = new QPushButton("刷新历史");

    historyLayout->addWidget(historyList);
    historyLayout->addWidget(refreshBtn);
    historyLayout->setStretch(0, 1); // 列表可扩展
    historyLayout->setStretch(1, 0); // 按钮固定大小
    connect(historyList, &QListWidget::currentRowChanged, [this](int index) {
        resultStack->setCurrentIndex(index);
    });

    // 下方结果显示区域（堆栈页）
    resultStack = new QStackedWidget();

    // 输入区域
    inputWidget = new QWidget();
    QVBoxLayout *inputLayout = new QVBoxLayout(inputWidget);

    // 命令行模式组件
    cmdInput = new QTextEdit();
    cmdInput->setPlaceholderText("Enter command, use ';' to execute.");

    // 模拟命令行行为
    //connect(cmdInput, &QTextEdit::textChanged, this, &MyMainWindow::checkForCmdEnd);

    // 脚本模式组件
    scriptInput = new QTextEdit();
    cur_file = new QLabel();
    cur_file->hide();
    scriptInput->setPlaceholderText("Enter multiple commands (one per line)");
    scriptInput->hide();
    sendBtn = new QPushButton("运行");
    sendBtn->hide();
    saveBtn = new QPushButton("保存");
    saveBtn->hide();

    inputLayout->addWidget(cmdInput);
    inputLayout->addWidget(cur_file);
    inputLayout->addWidget(scriptInput);
    inputLayout->addWidget(sendBtn);
    inputLayout->addWidget(saveBtn);

    // 中部：输入区 + 结果堆栈
    QSplitter *middleSplitter = new QSplitter(Qt::Vertical);
    middleSplitter->addWidget(inputWidget);
    middleSplitter->addWidget(resultStack);
    middleSplitter->setSizes(QList<int>() << 250 << 350);

    // 左边数据库结构树
    dbTreeView = new QTreeWidget();
    dbTreeView->setHeaderLabel("数据库结构");

    // 横向主布局
    QSplitter *mainSplitter = new QSplitter(Qt::Horizontal);
    mainSplitter->addWidget(dbTreeView);
    mainSplitter->addWidget(middleSplitter);

    mainSplitter->addWidget(historyPanel);
    mainSplitter->setStretchFactor(0, 1);
    mainSplitter->setStretchFactor(1, 4);
    mainSplitter->setStretchFactor(2, 1);

    setCentralWidget(mainSplitter);
}

void MyMainWindow::updateDBTree() {
    QJsonObject mes;
    mes["Type"] = "Struct";
    connector->sendMassage(mes);

    QJsonObject dbStructure = connector->receiveMassage();
    dbTreeView->clear();

    // 获取表结构数据
    QJsonArray tables = dbStructure["Mass"].toArray();
    QString dbName = dbStructure["ID"].toString();
    QTreeWidgetItem *dbItem = new QTreeWidgetItem(dbTreeView);
    dbItem->setText(0, dbName);

    for (const auto &tbl : tables) {
        QJsonObject tblObj = tbl.toObject();

        QString tableName = tblObj["table"].toString();
        QTreeWidgetItem *tableItem = new QTreeWidgetItem(dbItem);
        tableItem->setText(0, tableName);

        QJsonArray columns = tblObj["columns"].toArray();
        for (int i = 0; i + 1 < columns.size(); i += 2) {
            QString columnName = columns[i].toString();
            QString columnType = columns[i + 1].toString();
            QString colText = columnName + " : " + columnType;

            QTreeWidgetItem *colItem = new QTreeWidgetItem(tableItem);
            colItem->setText(0, colText);
        }
    }
}

void MyMainWindow::refreshHistory() {
    QVector<QString> newCommands;
    QVector<QWidget*> newResultWidgets;

    // 遍历历史列表
    for (int i = 0; i < historyList->count(); ++i) {
        QString itemText = historyList->item(i)->text();
        if (itemText.endsWith("(Failure)", Qt::CaseInsensitive)) {
            continue;  // 跳过失败记录
        }

        QString cmd = historyCommands[i]; // 取原始命令
        connector->sendOrder(cmd);
        QJsonObject response = connector->receiveMassage();

        // 构造新页面
        QWidget *resultWidget = new QWidget();
        QVBoxLayout *layout = new QVBoxLayout(resultWidget);
        layout->addWidget(new QLabel(cmd + "   (" + response["Status"].toString() + ")"));

        QTableWidget *table = new QTableWidget();
        layout->addWidget(table);

        if (response.contains("Mass") && response["Mass"].isObject()) {
            auto mass = response["Mass"].toObject();
            QJsonArray headers = mass["Header"].toArray();
            QJsonArray data = mass["Data"].toArray();

            table->setColumnCount(headers.size());
            QStringList headerLabels;
            for (const auto &h : headers)
                headerLabels << h.toString();
            table->setHorizontalHeaderLabels(headerLabels);

            table->setRowCount(data.size());
            for (int i = 0; i < data.size(); ++i) {
                QJsonArray row = data[i].toArray();
                for (int j = 0; j < row.size(); ++j) {
                    table->setItem(i, j, new QTableWidgetItem(row[j].toString()));
                }
            }
        }

        newCommands.append(cmd);
        newResultWidgets.append(resultWidget);
    }

    // 清空旧记录
    historyCommands = newCommands;
    historyList->clear();

    while (resultStack->count() > 0) {
        QWidget *w = resultStack->widget(0);
        resultStack->removeWidget(w);
        delete w;
    }

    // 重新插入新的
    for (int i = 0; i < newCommands.size(); ++i) {
        QString title = newCommands[i].section(' ', 0, 0).toUpper();
        connector->sendOrder(newCommands[i]);
        QJsonObject response = connector->receiveMassage();
        QString status = response["Status"].toString();
        historyList->addItem(title + " (" + status + ")");
        resultStack->addWidget(newResultWidgets[i]);
    }

    if (historyList->count()!=0)
        historyList->setCurrentRow(0);  // 默认选中第一项
}


void MyMainWindow::createWindow() {
    // 文件菜单
    fileMenu = menuBar()->addMenu(tr("&文件"));
    QAction *newScriptFile=fileMenu->addAction(tr("新建"));
    QAction *openScriptFile=fileMenu->addAction("打开");
    QAction *saveScriptFile=fileMenu->addAction("另存为");
    QAction *exitLogin=fileMenu->addAction("退出登录");

    connect(openScriptFile, &QAction::triggered, this, &MyMainWindow::openFile);
    connect(saveScriptFile, &QAction::triggered, this, &MyMainWindow::saveFileAs);
    connect(newScriptFile, &QAction::triggered, this, &MyMainWindow::newFile);
    connect(exitLogin, &QAction::triggered, this, &MyMainWindow::ExitLogin);

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

    //添加图形化建表等按钮的工具栏（放在菜单栏下面）
    QToolBar *toolBar = addToolBar("操作工具栏");
    createTableBtn = new QPushButton("建表");
    alterTableBtn = new QPushButton("改表");
    dropTableBtn = new QPushButton("删表");

    toolBar->addWidget(createTableBtn);
    toolBar->addWidget(alterTableBtn);
    toolBar->addWidget(dropTableBtn);

    // 后续可以绑定图形化建表对话框
    connect(createTableBtn, &QPushButton::clicked, this, &MyMainWindow::showCreateTableDialog);
    connect(alterTableBtn, &QPushButton::clicked, this, &MyMainWindow::showAlterTableDialog);
    connect(dropTableBtn, &QPushButton::clicked, this, &MyMainWindow::showDropTableDialog);

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
            fileChanged=0;
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

    // 创建新的标签页 Widget
    QWidget *resultWidget = new QWidget();
    QVBoxLayout* resultLayout = new QVBoxLayout(resultWidget);
    resultLayout->addWidget(new QLabel(command + "   (" + response["Status"].toString() + ")"));
    resultLayout->setSpacing(5);
    resultLayout->setContentsMargins(5, 5, 5, 5);

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

    // 添加到结果堆栈
    resultStack->addWidget(resultWidget);

    // 添加到历史列表
    //QString title = command.section(' ', 0, 0).toUpper();
    QString title = command;
    QListWidgetItem *item = new QListWidgetItem();
    HistoryItemWidget *historyWidget = new HistoryItemWidget(title);

    // 设置到列表中
    historyList->addItem(item);
    historyList->setItemWidget(item, historyWidget);
    historyList->setCurrentItem(item);

    //保存到内部历史记录表
    historyCommands.append(command);

    // 3. 删除历史记录时，同时删除对应结果页
    connect(historyWidget, &HistoryItemWidget::requestClose, this, [=]() {
    int row = historyList->row(item);
    QWidget *toDelete = resultStack->widget(row);
    resultStack->removeWidget(toDelete);
    delete toDelete;
    delete historyList->takeItem(row);
    });

    // 清空输入框，准备下一个命令
    cmdInput->clear();

}

void MyMainWindow::switchCmdMode() {
    currentMode = CMD_MODE;
    cur_file->hide();
    scriptInput->hide();
    sendBtn->hide();
    saveBtn->hide();
    cmdInput->show();
    cmdInput->setFocus();
}

void MyMainWindow::switchScriptMode() {
    currentMode = SCRIPT_MODE;
    cmdInput->hide();
    cur_file->show();
    scriptInput->show();
    sendBtn->show();
    saveBtn->show();
    scriptInput->setFocus();
}

// void MyMainWindow::closeTab(int index) {
//     if(resultTabs->count() > 0) {
//         QWidget *tab = resultTabs->widget(index);
//         tab->deleteLater();
//         resultTabs->removeTab(index);
//     }
// }
MyMainWindow::MyMainWindow(InternetConnector *connector,QWidget *parent)
    : QMainWindow(parent), currentMode(CMD_MODE)
{
    this->connector = connector;
    createWindow();
    createLayout();

    // 连接信号槽
    connect(cmdInput,&QTextEdit::textChanged,this,&MyMainWindow::checkForCmdEnd);
    connect(scriptInput,&QTextEdit::textChanged,this,&MyMainWindow::textChanged);
    connect(sendBtn, &QPushButton::clicked, this, &MyMainWindow::executeScriptOrder);
    connect(saveBtn, &QPushButton::clicked, this, &MyMainWindow::saveFile);
    connect(dbTreeView, &QTreeWidget::itemDoubleClicked, this, &MyMainWindow::onTreeItemDoubleClicked);


}
void MyMainWindow::onTreeItemDoubleClicked(QTreeWidgetItem *item) {
    if (!item) return;

    QTreeWidgetItem *parent = item->parent();

    if (!parent) {
        // 没有父节点 → 是数据库名，忽略或扩展未来支持
        return;
    }

    QString parentText = parent->text(0);
    QString itemText = item->text(0);
    QString sql;
    if (parent->parent() == nullptr) {
        // 说明 item 是 “表”，parent 是 “数据库”
        sql = QString("SELECT * FROM %1;").arg(itemText);
    } else {
        // item 是 “列”，parent 是 “表”
        QString tableName = parent->text(0);
        QString columnName = itemText.section(':',0,0);

        sql = QString("SELECT %1 FROM %2;").arg(columnName, tableName);

    }
    executeCmdOrder(sql);

}

void MyMainWindow::showCreateTableDialog() {
    QDialog dialog(this);
    dialog.setWindowTitle("建表");

    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    QLineEdit* tableNameEdit = new QLineEdit();
    tableNameEdit->setPlaceholderText("表名");

    QLabel* fieldLabel = new QLabel("字段定义：");
    QTableWidget* fieldTable = new QTableWidget(0, 6);
    fieldTable->setHorizontalHeaderLabels({"列名", "类型", "长度", "主键", "非空", "外键"});
    fieldTable->horizontalHeader()->setStretchLastSection(true);
    fieldTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    QPushButton* addRowBtn = new QPushButton("添加字段");
    QPushButton* deleteRowBtn = new QPushButton("删除选中字段");

    QHBoxLayout* fieldBtnLayout = new QHBoxLayout();
    fieldBtnLayout->addWidget(addRowBtn);
    fieldBtnLayout->addWidget(deleteRowBtn);

    QLineEdit* tableConstraintEdit = new QLineEdit();
    tableConstraintEdit->setPlaceholderText("表级约束，如 PRIMARY KEY (id)");

    QPushButton* okBtn = new QPushButton("确认");
    QPushButton* cancelBtn = new QPushButton("取消");
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);

    layout->addWidget(new QLabel("表名："));
    layout->addWidget(tableNameEdit);
    layout->addWidget(fieldLabel);
    layout->addWidget(fieldTable);
    layout->addLayout(fieldBtnLayout);
    layout->addWidget(new QLabel("表级约束（可选）："));
    layout->addWidget(tableConstraintEdit);
    layout->addLayout(btnLayout);

    auto addRow = [&]() {
        int row = fieldTable->rowCount();
        fieldTable->insertRow(row);

        fieldTable->setItem(row, 0, new QTableWidgetItem());  // 列名

        QComboBox* typeBox = new QComboBox();
        typeBox->addItems({"INT", "CHAR", "FLOAT", "DATE"});
        fieldTable->setCellWidget(row, 1, typeBox);

        QLineEdit* lengthEdit = new QLineEdit();
        lengthEdit->setPlaceholderText("仅对 CHAR 有效");
        fieldTable->setCellWidget(row, 2, lengthEdit);

        fieldTable->setCellWidget(row, 3, new QCheckBox());  // 主键
        fieldTable->setCellWidget(row, 4, new QCheckBox());  // 非空
        fieldTable->setCellWidget(row, 5, new QCheckBox());  // 外键
    };
    connect(addRowBtn, &QPushButton::clicked, addRow);
    addRow(); // 初始一行

    connect(deleteRowBtn, &QPushButton::clicked, [&]() {
        QList<QTableWidgetSelectionRange> ranges = fieldTable->selectedRanges();
        if (!ranges.isEmpty()) {
            int row = ranges.first().topRow();
            fieldTable->removeRow(row);
        }
    });

    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);

    if (dialog.exec() == QDialog::Accepted) {
        QString tableName = tableNameEdit->text().trimmed();
        QStringList columnDefs;
        QStringList primaryKeys;
        QStringList constraints;

        for (int i = 0; i < fieldTable->rowCount(); ++i) {
            QString name = fieldTable->item(i, 0)->text().trimmed();
            QComboBox* typeBox = qobject_cast<QComboBox*>(fieldTable->cellWidget(i, 1));
            QString type = typeBox->currentText();

            QString typeLength;
            if (type == "CHAR") {
                QLineEdit* lenEdit = qobject_cast<QLineEdit*>(fieldTable->cellWidget(i, 2));
                QString len = lenEdit->text().trimmed();
                if (len.isEmpty()) len = "20"; // 默认长度
                type += "(" + len + ")";
            }

            bool isPK = qobject_cast<QCheckBox*>(fieldTable->cellWidget(i, 3))->isChecked();
            bool notNull = qobject_cast<QCheckBox*>(fieldTable->cellWidget(i, 4))->isChecked();
            bool isFK = qobject_cast<QCheckBox*>(fieldTable->cellWidget(i, 5))->isChecked();

            if (!name.isEmpty()) {
                QString colDef = name + " " + type;
                if (notNull) colDef += " NOT NULL";
                columnDefs << colDef;
                if (isPK) primaryKeys << name;
                if (isFK) constraints << QString("FOREIGN KEY(%1) REFERENCES ref_table(ref_column)").arg(name);
            }
        }

        if (!primaryKeys.isEmpty())
            constraints << QString("PRIMARY KEY(%1)").arg(primaryKeys.join(", "));

        QString tableConstraint = tableConstraintEdit->text().trimmed();
        if (!tableConstraint.isEmpty())
            constraints << tableConstraint;

        QString sql = QString("CREATE TABLE %1 (\n  %2").arg(tableName, columnDefs.join(",\n  "));
        if (!constraints.isEmpty())
            sql += ",\n  " + constraints.join(",\n  ");
        sql += "\n);";

        executeCmdOrder(sql);
    }
}


void MyMainWindow::showAlterTableDialog() {
    QDialog dialog(this);
    dialog.setWindowTitle("修改表结构");

    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    // 表选择
    QComboBox* tableSelector = new QComboBox();
    QMap<QString, QTreeWidgetItem*> tableMap;  // 记录表名与节点映射

    for (int i = 0; i < dbTreeView->topLevelItemCount(); ++i) {
        QTreeWidgetItem* dbItem = dbTreeView->topLevelItem(i);
        for (int j = 0; j < dbItem->childCount(); ++j) {
            QTreeWidgetItem* tableItem = dbItem->child(j);
            QString tableName = tableItem->text(0);
            tableSelector->addItem(tableName);
            tableMap[tableName] = tableItem;
        }
    }

    QTableWidget* fieldTable = new QTableWidget(0, 3);
    fieldTable->setHorizontalHeaderLabels({"列名", "类型（如 CHAR(20)）", "操作（ADD/MODIFY/DROP）"});
    fieldTable->horizontalHeader()->setStretchLastSection(true);

    QPushButton* addRowBtn = new QPushButton("添加列");
    QPushButton* okBtn = new QPushButton("确认");
    QPushButton* cancelBtn = new QPushButton("取消");

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);

    layout->addWidget(new QLabel("选择表："));
    layout->addWidget(tableSelector);
    layout->addWidget(fieldTable);
    layout->addWidget(addRowBtn);
    layout->addLayout(btnLayout);

    // 表选择后加载已有列信息
    auto loadTableColumns = [&](const QString& tableName) {
        fieldTable->setRowCount(0);
        if (!tableMap.contains(tableName)) return;

        QTreeWidgetItem* tableItem = tableMap[tableName];
        for (int i = 0; i < tableItem->childCount(); ++i) {
            QString fullText = tableItem->child(i)->text(0);
            QStringList parts = fullText.split(":", Qt::SkipEmptyParts);

            if (parts.size() < 2) continue;

            QString colName = parts[0].trimmed();
            QString colType = parts[1].trimmed();

            if (colName.isEmpty()) continue;

            int row = fieldTable->rowCount();
            fieldTable->insertRow(row);
            fieldTable->setItem(row, 0, new QTableWidgetItem(colName));
            fieldTable->setItem(row, 1, new QTableWidgetItem(colType));

            QComboBox* actionBox = new QComboBox();
            actionBox->addItems({"MODIFY", "DROP"});
            fieldTable->setCellWidget(row, 2, actionBox);
        }
    };


    connect(tableSelector, &QComboBox::currentTextChanged, loadTableColumns);
    if (tableSelector->count() > 0)
        loadTableColumns(tableSelector->currentText());

    // 添加新字段操作（空行，默认ADD）
    auto addRow = [&]() {
        int row = fieldTable->rowCount();
        fieldTable->insertRow(row);
        fieldTable->setItem(row, 0, new QTableWidgetItem());
        fieldTable->setItem(row, 1, new QTableWidgetItem("CHAR(20)"));

        QComboBox* actionBox = new QComboBox();
        actionBox->addItems({"ADD"});
        fieldTable->setCellWidget(row, 2, actionBox);
    };
    connect(addRowBtn, &QPushButton::clicked, addRow);

    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);

    // 执行SQL
    if (dialog.exec() == QDialog::Accepted) {
        QString tableName = tableSelector->currentText();
        for (int i = 0; i < fieldTable->rowCount(); ++i) {
            QString colName = fieldTable->item(i, 0) ? fieldTable->item(i, 0)->text().trimmed() : "";
            QString colType = fieldTable->item(i, 1) ? fieldTable->item(i, 1)->text().trimmed() : "";
            QComboBox* actionBox = qobject_cast<QComboBox*>(fieldTable->cellWidget(i, 2));
            QString action = actionBox ? actionBox->currentText() : "";

            if (colName.trimmed().isEmpty() || action.isEmpty())
                continue;

            QString sql;
            if (action == "ADD")
                sql = QString("ALTER TABLE %1 ADD COLUMN %2 %3;").arg(tableName, colName, colType);
            else if (action == "MODIFY")
                sql = QString("ALTER TABLE %1 MODIFY COLUMN %2 %3;").arg(tableName, colName, colType);
            else if (action == "DROP") {
                QMessageBox::StandardButton confirm = QMessageBox::question(
                    this,
                    "确认删除",
                    QString("确认要删除表 [%1] 中的列 [%2] 吗？").arg(tableName, colName),
                    QMessageBox::Yes | QMessageBox::No
                    );
                if (confirm == QMessageBox::Yes) {
                    sql = QString("ALTER TABLE %1 DROP COLUMN %2;").arg(tableName, colName);
                } else {
                    continue; // 用户取消删除，跳过此行
                }
            }

            executeCmdOrder(sql);
        }
    }
}


#include <QInputDialog>

void MyMainWindow::showDropTableDialog() {
    bool ok;
    QString tableName = QInputDialog::getText(this, "删除表", "请输入要删除的表名：", QLineEdit::Normal, "", &ok);

    if (ok && !tableName.trimmed().isEmpty()) {
        QString dropSQL = QString("DROP TABLE %1;").arg(tableName.trimmed());
        executeCmdOrder(dropSQL);  //  直接执行SQL
    }
}

void MyMainWindow::ExitLogin(){
    emit clickExit();
    connector->logout();

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
    connect(mainWindow, &MyMainWindow::clickExit, this, &MainController::showLoginWindow);
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
void MainController::showLoginWindow() {
    hideAllWindows();
    loginWindow->show();
}
void MainController::showMyMainWindow() {
    hideAllWindows();
    mainWindow->updateDBTree();
    mainWindow->show();
}

void MyMainWindow::openFile() {
    if (fileChanged) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "保存当前文件", "是否保存当前文件？",
                                      QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

        if (reply == QMessageBox::Yes) {
            saveFileAs(); // 调用另存为功能保存文件
        } else if (reply == QMessageBox::Cancel) {
            return; // 如果选择取消，则不进行打开操作
        }
    }
    // 弹出文件选择对话框
    QString fileName = QFileDialog::getOpenFileName(this, "打开文件", "", "SQL Files (*.sql);;Text Files (*.txt);;All Files (*)");

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            //保存临时文件路径
            curFileName=fileName;
            cur_file->setText(curFileName);
            // 读取文件内容到 QTextEdit
            QTextStream in(&file);
            scriptInput->setPlainText(in.readAll());
            file.close();
            //最后标记更改
            fileChanged = 0;
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
    curFileName = "";
    cur_file->setText("");
    // 可选：为新文件设置一个默认的文本（例如 "新建文件.sql"）
    scriptInput->setPlainText("");
    //最后重置更改
    fileChanged = 0;
}

HistoryItemWidget::HistoryItemWidget(const QString &text, QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(5, 0, 5, 0);
    layout->setSpacing(5);

    label = new QLabel(text, this);
    closeBtn = new QPushButton("✕", this);
    closeBtn->setFixedSize(16, 16);
    closeBtn->setStyleSheet("QPushButton { border: none; font-weight: bold; }"
                            "QPushButton:hover { color: red; }");

    layout->addWidget(label);
    layout->addStretch();
    layout->addWidget(closeBtn);

    connect(closeBtn, &QPushButton::clicked, this, &HistoryItemWidget::requestClose);
}
