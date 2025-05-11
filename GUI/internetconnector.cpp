#include "InternetConnector.h"
#include <QCoreApplication>




InternetConnector::InternetConnector(const QString &host, quint16 port) {
    socket = new QTcpSocket(this); // 设置父对象为 GameClient，管理 socket 内存

    // 尝试连接到服务器
    socket->connectToHost(host, port);

    // 检查连接是否成功
    if (!socket->waitForConnected(3000)) {
        qDebug() << "Failed to connect to server:" << socket->errorString();
        socket->deleteLater(); // 清理 socket
        socket = nullptr;
        return; // 提前退出
    }

    qDebug() << "Connected to server at" << host << ":" << port;

    // // 监听错误信号
    // connect(socket, &QTcpSocket::errorOccurred, this, [](QAbstractSocket::SocketError error) {
    //     qDebug() << "Socket error occurred:" << error;
    // });
}

InternetConnector::~InternetConnector() {
    if (socket) {
        socket->close();
        delete socket;
    }
}
#include<iostream>
bool InternetConnector::login(const QString &id, const QString &password) {
    QJsonObject loginRequest={
        {"Type", "Login"},
        {"Mass",QJsonObject{{"ID",id},{"Password",password}}}
    };
    sendMassage(loginRequest);
    QJsonObject response = receiveMassage();

    if (response["Status"].toString() == "Success") {
        std::cout<<response["Message"].toString().toStdString();
        qDebug() << "登录成功";
        return true;
    } else {
        qDebug() << "登录失败：" << response["Message"].toString();
        return false;
    }
}
void InternetConnector::logout() {
    QJsonObject logoutRequest={
        {"Type", "Logout"},
        {"Mass", ""}
    };
    sendMassage(logoutRequest);
}


QJsonArray convertToQJsonArray(const QVector<QVector<int>> &matrix) {
    QJsonArray jsonArray;
    for (const auto &row : matrix) {
        QJsonArray jsonRow;
        for (int value : row) {
            jsonRow.append(value);
        }
        jsonArray.append(jsonRow);
    }
    return jsonArray;
}

void InternetConnector::sendOrder(const QString &qstr) {
    printf("在此时发送命令\n");
    QJsonObject mas;
    mas["Type"] = "Order";
    mas["Mass"] = qstr;
    if(sendMassage(mas)){
        printf("Send success!!\n");
    }
    else{
        printf("发送失败！？");
    }
}

bool InternetConnector::sendMassage(const QJsonObject &json) {
    QJsonDocument doc(json);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    socket->write(data);
    return socket->flush();
}

QJsonObject InternetConnector::receiveMassage(int special) {
    QByteArray data;
    switch(special){
    case 0:
        socket->waitForReadyRead(30000);
        data = socket->readAll();
        break;
    case 1:
        socket->waitForReadyRead(-1);
        data = socket->readAll();
        break;
    case 2:
        data = socket->readAll();
        break;
    default:
        data = socket->readAll();
        break;
    }

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isObject()) {
        return doc.object();
    }

    return QJsonObject();
}



















