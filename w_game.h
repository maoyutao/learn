﻿#ifndef W_GAME_H
#define W_GAME_H

#include "w_game.h"
#include "player.h"
#include "w_battlefield.h"
#include <QWidget>
#include <QStackedWidget>
#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>

namespace Ui {
class MainWindow;
}

class Game;

typedef void (Game::*standardSlot)(CardButton*);

class Game : public QStackedWidget
{
    Q_OBJECT
public:
    explicit Game(QWidget *parent = nullptr);
    void init(Player* player, Ui::MainWindow *ui);
signals:

public slots:
private slots:
    void asClient();
    void asServer();
    void onConnect();
    void onNewConnection();
    void chooseCardSet();
    void start(CardSet* cardSet);
    void dispatchCard(CardButton* card);
    void changePageToGaming();
    void showToBechosen(QList<CardButton*> list, standardSlot slot); // 在choose槽里展示list里的button  并为这些button的selected绑定这个类中的标准槽
    void receiveMsg();
private:
    Ui::MainWindow *ui{nullptr};
    BattleField* battleField;
    Player* player{nullptr};
    QTcpSocket* socket{nullptr};
    QTcpServer* server{nullptr};
    int mScore[3];
    int oScore[3];

    int signalTimes{0};
    int signalTimesLimit{0};

    bool myRound{false};
    bool ready[2]{false, false};

private:
    void sendMsg(QMap<QString, QString> msgMap);
    QMap<QString, QString> parse(QString msg);
    QString stringify(QMap<QString, QString> msgMap);
    void msgHandler(QMap<QString, QString> msgMap);

    void hReady(QMap<QString, QString> msgMap);
};

#endif // W_GAME_H
