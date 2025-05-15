#ifndef INTERNETCONNECTOR_H
#define INTERNETCONNECTOR_H


#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QVector>
#include <QObject>
#include <QDebug>

class InternetConnector : public QObject{

    Q_OBJECT
    //friend class MultiGame;
    friend class MyMainWindow;

public:
    InternetConnector(const QString &host, quint16 port);
    ~InternetConnector();

    // 登录验证
    bool login(const QString &id, const QString &password);
    void logout();

signals:
    void logged();

    // 发送用户请求的操作
    void sendOrder(const QString &qstr);

    //外部用接收并显示数据
    //void recvMessage();

    // void stopListening();
    // void startListening();


signals:
    void findOp();
    void getMatrix();
    void updateGraphicsSignal(QPair<int ,int> first,QPair<int ,int> second); // 更新图案的信号
    //void opWin();

private:
    QTcpSocket *socket;

    // 私有的 JSON 数据发送和接收
    bool sendMassage(const QJsonObject &json);
    QJsonObject receiveMassage(int special = 0);
};


#endif // INTERNETCONNECTOR_H
