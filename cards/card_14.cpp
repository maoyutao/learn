#include "card_14.h"
#include "ui_cardslot.h"
#include "w_battlefield.h"


Card_14::Card_14(BattleField *battleField, QObject *parent):
    Card(14, 6, "Crone: Weavess", "部署：从牌组打出“煮婆”和“呢喃婆”。",
         ":/new/cards/resource/cards/card_14.png",
         EQuality::Silver, EClassification::Unit, battleField, parent)
{
    position.append(EPosition::mBack);
    property.append(EProperty::Magic);
    property.append(EProperty::Waste);
}

void Card_14::exertAbility()
{
    battleField->setAllHandCardExertable(false);
    battleField->move(battleField->mBack, button);


    for (int it: battleField->mDeck)
    {
        if (it == 15 || it == 16)
        {
            battleField->mDeck.removeOne(it);
            CardButton * card = new CardButton(it, battleField, nullptr);
            card->card->first = false;
            card->card->exertAbility();
        }
    }
    if (first)
    {
        battleField->finishOneRound();
    }
}
