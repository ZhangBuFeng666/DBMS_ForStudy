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

void InternetConnector::recvMessage() {
    QJsonObject message = receiveMassage(2);//直接读取

    if (message.isEmpty()) {
        qDebug() << "Received empty or invalid message from server.";
        return;
    }

    QString type = message["Type"].toString();
    if (type == "Move") {
        int x1 = message["X1"].toInt();
        int y1 = message["Y1"].toInt();
        int x2 = message["X2"].toInt();
        int y2 = message["Y2"].toInt();

        qDebug() << "Move received from server:" << x1 << y1 << x2 << y2;

        QPair<int ,int>first(x1,y1);
        QPair<int ,int>second(x2,y2);

        emit updateGraphicsSignal(first,second);


    } else if (type == "GameOver") {
        stopListening();
        //emit opWin();//connect到gamePage生成multi部分
        qDebug() << "Another person wins...";
        // 处理游戏结束逻辑
    } else {
        qDebug() << "Unhandled message type:" << type;
    }

}

bool InternetConnector::login(const QString &id, const QString &password) {
    QJsonObject loginRequest;
    loginRequest["Type"] = "Login";
    loginRequest["ID"] = id;
    loginRequest["Password"] = password;

    sendMassage(loginRequest);
    QJsonObject response = receiveMassage();

    if (response["Status"].toString() == "Success") {
        qDebug() << "登录成功";
        return true;
    } else {
        qDebug() << "登录失败：" << response["Message"].toString();
        return false;
    }
}

// void InternetConnector::sendSingleGameResult(int difficulty, qint64 timeUsed) {
//     QJsonObject result;
//     result["Type"] = "SingleGameResult";
//     result["Difficulty"] = difficulty;
//     result["TimeUsed"] = timeUsed;

//     sendMassage(result);
// }


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

// bool InternetConnector::sendMatchRequest(int difficulty) {
//     // 创建并发送匹配请求
//     QJsonObject matchRequest = {
//         {"Type", "MatchRequest"},
//         {"Difficulty", difficulty}
//     };
//     sendMassage(matchRequest);

//     // 接收并处理匹配响应
//     QJsonObject response = receiveMassage(1);
//     if(response["Status"].toString() == "Matched"){
//         qDebug() << "匹配成功，等待进行矩阵交换";
//         return true;
//     }else if (response["Status"].toString() == "Waiting") {
//         QJsonObject response = receiveMassage(1);
//         if(response["Status"].toString() == "Matched"){

//             qDebug() << "匹配成功，等待进行矩阵交换";
//             return true;
//         }
//     }
//     qDebug() << "匹配失败：" << response["Message"].toString();
//     return false;

// }

// bool internetConnector::exchangeMatrices(QVector<QVector<int>> &myMatrix, QVector<QVector<int>> &opponentMatrix) {
//     // 发送自己的矩阵到服务器
//     QJsonObject matrixRequest = {
//         {"Type", "Matrix"},
//         {"Matrix", convertToQJsonArray(myMatrix)}
//     };
//     sendJson(matrixRequest);

//     // 接收并处理对手的矩阵
//     QJsonObject matrixResponse = receiveJson(1);
//     if (matrixResponse["Type"].toString() != "OpponentMatrix") {
//         qDebug() << "未收到对手矩阵";
//         return false;
//     }

//     // 将对手的矩阵解析到 opponentMatrix
//     opponentMatrix.clear(); // 清空之前的数据
//     QJsonArray opponentArray = matrixResponse["Matrix"].toArray();
//     for (const auto &row : opponentArray) {
//         QVector<int> parsedRow;
//         for (const auto &val : row.toArray()) {
//             parsedRow.append(val.toInt());
//         }
//         opponentMatrix.append(parsedRow);
//     }

//     qDebug() << "对手矩阵接收成功";
//     return true;
// }



void InternetConnector::sendOrder(const QString &qstr) {
    printf("在此时发送命令\n");
    QJsonObject mas;
    mas["Type"] = "order";
    mas["Mas"] = qstr;
    if(sendMassage(mas)){
        printf("Send success!!\n");
    }
    else{
        printf("发送失败！？");
    }
}

// void InternetConnector::sendGameOver(bool isWinner, int difficulty, qint64 timeUsed) {
//     QJsonObject gameOver;
//     gameOver["Type"] = "GameOver";
//     gameOver["IsWinner"] = isWinner;
//     gameOver["Difficulty"] = difficulty;
//     gameOver["TimeUsed"] = timeUsed;
//     stopListening();
//     sendMassage(gameOver);
// }

// QVector<qint64> InternetConnector::requestPersonalRecord() {
//     QJsonObject request;
//     request["Type"] = "PersonalRecord";

//     sendMassage(request);

//     QVector<qint64> records(3);
//     QJsonObject rec = receiveMassage();
//     if(rec["Status"]=="Success"){
//         QJsonValueRef jsonValueRef0 = rec["Easy"];
//         QJsonValueRef jsonValueRef1 = rec["Mid"];
//         QJsonValueRef jsonValueRef2 = rec["Hard"];
//         records[0] = jsonValueRef0.toVariant().toLongLong();
//         records[1] = jsonValueRef1.toVariant().toLongLong();
//         records[2] = jsonValueRef2.toVariant().toLongLong();

//     }
//     return records;
// }

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
        socket->waitForReadyRead(3000);
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

void InternetConnector::startListening() {
    // 连接 readyRead 信号，确保监听服务器数据
    connect(socket, &QTcpSocket::readyRead, this, &InternetConnector::recvMessage);

    qDebug() << "Started listening for server messages.";
}

void InternetConnector::stopListening() {
    // 断开 readyRead 信号，停止监听
    disconnect(socket, &QTcpSocket::readyRead, this, &InternetConnector::recvMessage);

    qDebug() << "Stopped listening for server messages.";
}
