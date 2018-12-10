#pragma once
#ifndef TRON_GAME_H
#define TRON_GAME_H


#include <winsock.h>
#include "Client.h"
#include <SFML\System\Clock.hpp>


class TronGame
{
public:
	enum Player{P0,P1};
public:
	TronGame();
	~TronGame();
	void Create(const Client& player0, const Client& player1);
	Client* GetPlayer(Player);
public:
	sf::Clock m_start;
	bool m_gameStarted{ false };
private:
	Client m_player[2];
};



#endif // !TRON_GAME_H

