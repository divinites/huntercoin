#include "gamemapview.h"

#include "../gamestate.h"
#include "../util.h"

#include <QImage>
#include <QGraphicsPixmapItem>
#include <QGraphicsSimpleTextItem>

#include <boost/foreach.hpp>

using namespace Game;

static const int TILE_SIZE = 48;

GameMapView::GameMapView(QWidget *parent)
    : QGraphicsView(parent)
{
    scene = new QGraphicsScene(this);
    setScene(scene);

    QPixmap bg_tile(TILE_SIZE, TILE_SIZE);
    bg_tile.fill(QColor(0, 180, 0));

    QPainter painter(&bg_tile);
    painter.drawRect(0, 0, TILE_SIZE, TILE_SIZE);
    painter.end();

    setBackgroundBrush(bg_tile);

    setDragMode(QGraphicsView::ScrollHandDrag);
}

void GameMapView::updateGameMap(const GameState &gameState)
{
    scene->clear();

    QGraphicsTextItem *text = new QGraphicsTextItem;
    text->setHtml("<span style='color:white'>&nbsp;&nbsp; &rarr; x<br />&darr;<br />y</span>");
    text->setPos(-6, -12);
    scene->addItem(text);

    BOOST_FOREACH(const PAIRTYPE(Coord, GameState::LootInfo) &loot, gameState.loot)
    {
        int x = loot.first.x * TILE_SIZE;
        int y = loot.first.y * TILE_SIZE;
        scene->addEllipse(x, y, TILE_SIZE, TILE_SIZE, QPen(Qt::black), QBrush(Qt::yellow));
        text = new QGraphicsTextItem;
        text->setHtml(
                "<center>"
                + QString::fromStdString(FormatMoney(loot.second.nAmount))
                + "</center>"
            );
        text->setPos(x, y + 13);
        text->setTextWidth(TILE_SIZE);
        scene->addItem(text);
    }

    playerLocations.clear();
    playerLocations.reserve(gameState.players.size());

    // Sort by coordinate bottom-up, so the stacking (multiple players on tile) looks correct
    typedef const std::pair<const PlayerID, PlayerState> *PlayerEntryPtr;
    std::multimap<Coord, PlayerEntryPtr> sortedPlayers;

    for (std::map<PlayerID, PlayerState>::const_iterator mi = gameState.players.begin(); mi != gameState.players.end(); mi++)
    {
        const Coord &coord = mi->second.coord;
        sortedPlayers.insert(std::make_pair(Coord(-coord.x, -coord.y), &(*mi)));
        playerLocations[QString::fromStdString(mi->first)] = QPoint(coord.x, coord.y);
    }

    Coord prev_coord;
    int offs = -1;
    BOOST_FOREACH(const PAIRTYPE(Coord, PlayerEntryPtr) &data, sortedPlayers)
    {
        const PlayerID &playerName = data.second->first;
        const PlayerState &playerState = data.second->second;
        const Coord &coord = playerState.coord;
        int color = playerState.color;

        if (offs >= 0 && coord == prev_coord)
            offs++;
        else
        {
            prev_coord = coord;
            offs = 0;
        }

        int x = coord.x * TILE_SIZE + offs * 2;
        int y = coord.y * TILE_SIZE + offs * 13;
        scene->addRect(x, y, TILE_SIZE, TILE_SIZE, QPen(Qt::black), QBrush(color == 0 ? Qt::red : Qt::blue));
        QGraphicsSimpleTextItem *simpleText = scene->addSimpleText(QString::fromStdString(playerName));
        simpleText->setPos(x + 2, y);
        if (color == 1)
            simpleText->setBrush(QBrush(Qt::white));
    }
}

const static QPoint NO_POINT(INT_MAX, INT_MAX);

void GameMapView::CenterMapOnPlayer(const QString &name)
{
    QPoint p = playerLocations.value(name, NO_POINT);
    if (p == NO_POINT)
        return;
    centerOn((p.x() + 0.5) * TILE_SIZE, (p.y() + 0.5) * TILE_SIZE);
}
