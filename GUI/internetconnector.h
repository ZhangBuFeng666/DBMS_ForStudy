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

public:
    InternetConnector(const QString &host, quint16 port);
    ~InternetConnector();

    // 登录验证
    bool login(const QString &id, const QString &password);

    // 单人游戏完成时发送结果
    // void sendSingleGameResult(int difficulty, qint64 timeUsed);

    // 请求联机对战
    // bool sendMatchRequest(int difficulty);
    // bool exchangeMatrices(QVector<QVector<int>> &myMatrix, QVector<QVector<int>> &opponentMatrix);

    // 发送用户请求的操作
    void sendOrder(const QString &qstr);

    // 发送对局完成信息
    // void sendGameOver(bool isWinner, int difficulty, qint64 timeUsed);

    // 请求个人记录
    // QVector<qint64> requestPersonalRecord();

    //接收并显示数据
    void recvMessage();

    void stopListening();
    void startListening();


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
