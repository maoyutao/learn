#include "w_game.h"
#include "ui_mainwindow.h"
#include "ui_standarddialog.h"
#include "cardset.h"
#include <QStringList>
#include <QDialog>
#include <QDebug>
#include <QtNetwork>
#include <QStringBuilder>
#include <QByteArray>
#include <QDataStream>
#include <QtGlobal>

#define PGAMEREADY 0
#define PWAITING 1
#define PCHOOSE 2
#define PGAMING 3
#define PSCORE 4
#define PFINALSCORE 5

Game::Game(QWidget *parent) : QStackedWidget(parent)
{

}

void Game::init(Player *aPlayer, Ui::MainWindow *aui)
{
    ui = aui;
    player = aPlayer;
    battleField = ui->battleField;
    this->setCurrentIndex(PGAMEREADY);
    connect(ui->asClientButton, SIGNAL(clicked()), this, SLOT(asClient()), Qt::UniqueConnection);
    connect(ui->asServerButton, SIGNAL(clicked()), this, SLOT(asServer()), Qt::UniqueConnection);
}

void Game::asClient()
{
    Dialog dlg("invitation code:", true, this);
    if (QDialog::Accepted == dlg.exec())
    {
        this->setCurrentIndex(PWAITING);
        QStringList list = dlg.content.split('&');
        QHostAddress address = QHostAddress(list.at(0));
        int port = list.at(1).toInt();
        socket = new QTcpSocket(this);
        socket->connectToHost(address, port, QTcpSocket::ReadWrite);
        connect(socket, SIGNAL(connected()), this, SLOT(onConnect()));
    }
}

void Game::asServer()
{
    QHostAddress ip;
    int port = 56700;
    QString localHostName = QHostInfo::localHostName();
    QHostInfo info = QHostInfo::fromName(localHostName);
    foreach(QHostAddress address, info.addresses())
    {
         if(address.protocol() == QAbstractSocket::IPv4Protocol)
         {
             ip = address;
             break;
         }
    }
    server = new QTcpServer(this);
    while (!server->listen(ip, port))
    {
        port++;
    }
    connect(server, SIGNAL(newConnection()), this, SLOT(onNewConnection()));
    Dialog dlg("invitation code:", true, this);
    dlg.setText(ip.toString() % "&" % QString::number(port));
    if (QDialog::Accepted == dlg.exec())
    {
        this->setCurrentIndex(PWAITING);
    }
    else
    {
        server->close();
        server->deleteLater();
    }
}

void Game::onNewConnection()
{
    if (this->currentIndex() != PWAITING)
    {
        for (auto it: this->children())
        {
            if (it->inherits("Dialog"))
            {
                static_cast<Dialog*>(*&it)->accept();
            }
        }
        this->setCurrentIndex(PWAITING);
    }
    socket = server->nextPendingConnection();
    onConnect();
}

void Game::onConnect()
{
    connect(socket, SIGNAL(disconnected()), socket, SLOT(deleteLater()));
    connect(socket, SIGNAL(readyRead()), this, SLOT(receiveMsg()));
    ui->msg->setText(QApplication::translate("MainWindow", "<html><head/><body><p align=\"center\"><span style=\" font-size:24pt; color:#b7b7b7;\">Connect successfully!</span></p></body></html>", Q_NULLPTR));
    QTimer *timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, SIGNAL(timeout()), this, SLOT(chooseCardSet()), Qt::UniqueConnection);
    connect(timer, SIGNAL(timeout()), timer, SLOT(deleteLater()), Qt::UniqueConnection);
    timer->start(1200);
}

void Game::chooseCardSet()
{
    ui->gamingChooseSlot->setLimit(3); // 设置一页最多几张
    for (auto it: player->cardSets)
    {
        CardSetButton* set = new CardSetButton(&*it, ui->gamingChooseSlot); // 实例化按钮
        ui->gamingChooseSlot->addCard(set); // 显示
        if (it->isValid) {
            connect(set, SIGNAL(seletced(CardSet*)), this, SLOT(start(CardSet*)));
        } else {
            set->setEnabled(false);
        }
    }
    this->setCurrentIndex(PCHOOSE);
}

void Game::start(CardSet *cardSet)
{
    ui->msg->setText(QApplication::translate("MainWindow", "<html><head/><body><p align=\"center\"><span style=\" font-size:24pt; color:#b7b7b7;\">Please wait patiently.</span></p></body></html>", Q_NULLPTR));
    this->setCurrentIndex(PWAITING);
    ui->gamingChooseSlot->clear();
    battleField->initForFirst(cardSet, ui);
    signalTimesLimit = 3;
    showToBechosen(battleField->drawCards(10), &Game::dispatchCard);
}

void Game::showToBechosen(QList<CardButton *> list, standardSlot slot)
{
    for (auto it: list) {
        ui->gamingChooseSlot->addCard(&*it);
        connect(&*it, &CardButton::seletced, this, slot);
    }
    this->setCurrentIndex(PCHOOSE);
    ui->gamingChooseSlot->setCurrentIndex(0);
}

void Game::dispatchCard(CardButton * card)
{
    ui->gamingChooseSlot->setAllEnabled(false);
    auto newCard = battleField->drawCards().first();
    ui->gamingChooseSlot->replaceCard(card, newCard);


    battleField->addCardToMDeck(card->card->id);
    connect(newCard, SIGNAL(seletced(CardButton*)), this, SLOT(dispatchCard(CardButton*)));
    signalTimes++;
    if (signalTimes < signalTimesLimit)
    {
        ui->gamingChooseSlot->setAllEnabled(true);
        return;
    }
    QTimer *timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, SIGNAL(timeout()), this, SLOT(changePageToGaming()), Qt::UniqueConnection);
    connect(timer, SIGNAL(timeout()), timer, SLOT(deleteLater()), Qt::UniqueConnection);
    timer->start(800);
}

void Game::changePageToGaming()
{
    battleField->init();
    QMap<QString, QString> msg;
    msg["type"] = "ready";
    msg["who"] = "opponent";
    sendMsg(msg);
    msg["who"] = "me";
    hReady(msg);
    ui->gamingChooseSlot->clear();
    ui->gameStackWidget->setCurrentIndex(PGAMING);
}

void Game::sendMsg(QMap<QString, QString> msg)
{
    QString str = stringify(msg);
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_9);
    out << str;
    socket->write(block);
//    qDebug() << "in sendMsg";
}

void Game::receiveMsg()
{
//    qDebug() << "in receiveMsg";
    QDataStream in;
    in.setDevice(socket);
    in.setVersion(QDataStream::Qt_5_9);
    QString str;
    in.startTransaction();
    in >> str;
    msgHandler(parse(str));
}

QMap<QString, QString> Game::parse(QString msg)
{
    QList<QString> temp = msg.split('&');
    QList<QString> kv;
    QMap<QString, QString> map;
    for (auto it: temp)
    {
        kv = it.split('=');
        map[kv.at(0)] = kv.at(1);
    }
    return map;
}

QString Game::stringify(QMap<QString, QString> msgMap)
{
    QList<QString> temp;
    for (QMap<QString, QString>::const_iterator it: msgMap)
    {
        temp.append(it.key() % "=" % it.value());
    }
    return temp.join('&');
}

void Game::msgHandler(QMap<QString, QString> msgMap)
{
    switch (msgMap["type"]) {
    case "ready":
        hReady(msgMap);
        break;
    default:
        break;
    }
}

void Game::hReady(QMap<QString, QString> msgMap)
{
    (msgMap["who"] == "me") ? ready[0] : ready[1] = true;
    if (ready[0] && ready[1])
    {
        // 开始游戏
        if (server)
        {
            qsrand(QTime(0,0,0).secsTo(QTime::currentTime()));
            QMap<QString, QString> msg;
            msg["type"] = "startGame";
            int first = qrand() % 2;
            if (first)
            {
                myRound = true;
                msg["first"] = "opponent";
            } else {
                myRound = false;
                msg["first"] = "me";
            }
            sendMsg(msg);

        }
    }
}

