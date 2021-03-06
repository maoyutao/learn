#ifndef PLAYER_H
#define PLAYER_H

#include "cardset.h"
#include <QObject>
#include <QString>
#include <QList>

class Player : public QObject
{
    Q_OBJECT
private:
    int point{0};
    QString name{"Warrior"};
public:
    explicit Player(QObject *parent = nullptr);
    QList<int> allCards;
    QList<CardSet*> cardSets;
signals:

public slots:
};

#endif // PLAYER_H
