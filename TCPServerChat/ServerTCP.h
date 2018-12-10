#pragma once
#ifndef SERVER_TCP_H
#define SERVER_TCP_H

#include <string>
#include <vector>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "TronGame.h"
#include "Client.h"
#include <SFML/System.hpp>

class ServerTCP {
public:
	ServerTCP();
	~ServerTCP();
	bool Start(const char* ip, const char* port, uint16_t maxConnections);
	void Run();
private:
	typedef std::string Command;
	enum {EXIT,MSG,ONLINE,PLAY,ACCEPT,COMMANDS};
	struct NoBlockSet {
		timeval timevalue;
		FD_SET fdRead;
	};	
private:
	SOCKET ListenToClients();
	bool MessageReceived();
	bool MessageReceived(Client&);
	bool ValidateNickname(const char*);
	void ManageInputs();
	void ReceiveNewClients();
	void ProcessMessages();
	void ProcessGames();
	void CreateGame(Client&, Client&);
	void ProcessCommand(std::string, Client&, bool&);
	void SendListOfClients(const Client&);
	void SendListOfCommands(const Client&);
	void SendMsgToClient(const std::string&, const Client&);
	void SendMsgToAllClients(const std::string&, uint16_t* exception = nullptr);
	void Close();
	
private:
	NoBlockSet m_noBlockSet;
	uint16_t ids{ 0 };
	sf::Clock m_timer;
	std::vector<Command> m_commands{ COMMANDS };
	std::vector<Client> m_clients;
	std::vector<TronGame> m_tronGames;
	bool m_serverOpen{ false };
	uint16_t m_maxConnections{0};
	uint16_t m_currentConnections{0};
	SOCKET m_listenSocket{ INVALID_SOCKET };

};

#endif // !SERVER_TCP_H