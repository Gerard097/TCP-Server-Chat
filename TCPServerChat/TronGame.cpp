#include "TronGame.h"

TronGame::TronGame()
{
}

TronGame::~TronGame()
{
}

void TronGame::Create(const Client& player0, const Client& player1)
{
	m_player[P0] = player0;
	m_player[P1] = player1;
}

Client* TronGame::GetPlayer(Player player)
{
	return &m_player[player];
}
