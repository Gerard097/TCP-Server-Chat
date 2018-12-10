#include "ServerTCP.h"
#include <conio.h>
#include <iostream>

ServerTCP::ServerTCP() {

	m_commands[EXIT] = "/exit";
	m_commands[MSG] = "/msg";
	m_commands[ONLINE] = "/online";
	m_commands[PLAY] = "/play";
	m_commands[ACCEPT] = "/accept";
}

ServerTCP::~ServerTCP() {
}

bool ServerTCP::Start(const char* ip, const char* port, uint16_t maxConnections) {

	m_maxConnections = maxConnections;
	ADDRINFO hints, *pAddrResult;
	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (result)
	{
		std::cout << "Error al inicializar los Sockets de Windows en su version 2" << std::endl;
		return false;
	}

	hints.ai_addr = 0;
	hints.ai_addrlen = 0;
	hints.ai_canonname = 0;
	hints.ai_family = AF_INET; //Familia de direcciones 
	hints.ai_flags = AI_PASSIVE; //Solo sn diferentes de 0 en el servidor
	hints.ai_next = 0;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_socktype = SOCK_STREAM;

	result = GetAddrInfo((LPCTSTR)ip, port, &hints, &pAddrResult);

	if (result) {
		std::cout << "Error al buscar protocolo de comunicaciones en la maquina local" << std::endl;
		WSACleanup();
		return false;
	}

	m_listenSocket = socket(pAddrResult->ai_family, pAddrResult->ai_socktype, pAddrResult->ai_protocol);

	if (m_listenSocket == INVALID_SOCKET)
	{
		std::cout << "Imposible crear socket de escucha" << std::endl;
		FreeAddrInfo(pAddrResult);
		WSACleanup();
		return false;
	}

	result = bind(m_listenSocket, pAddrResult->ai_addr, pAddrResult->ai_addrlen);

	if (result) {
		std::cout << "Imposible asociar un puerto local" << std::endl;
		closesocket(m_listenSocket);
		FreeAddrInfo(pAddrResult);
		WSACleanup();
		return false;
	}

	result = listen(m_listenSocket, SOMAXCONN);

	if (result) {
		std::cout << "Imposible asociar un puerto local" << std::endl;
		closesocket(m_listenSocket);
		FreeAddrInfo(pAddrResult);
		WSACleanup();
		return false;
	}

	std::cout << "Presione ESC para cerrar el servidor\n";
	
	FreeAddrInfo(pAddrResult);
	m_serverOpen = true;
	m_noBlockSet.timevalue.tv_sec = 0;
	m_noBlockSet.timevalue.tv_usec = 0;

	return true;
}

void ServerTCP::Run() {

	while (m_serverOpen) {

		if(m_currentConnections < m_maxConnections)
			ReceiveNewClients();
		ProcessMessages();
		if(m_timer.getElapsedTime().asSeconds() >= 0.016f){
			ProcessGames();
			m_timer.restart();
		}
		ManageInputs();
	}	

	Close();

}

SOCKET ServerTCP::ListenToClients() {
	
	FD_ZERO(&m_noBlockSet.fdRead);
	FD_SET(m_listenSocket, &m_noBlockSet.fdRead);
	
	if (select(NULL, &m_noBlockSet.fdRead, NULL, NULL, &m_noBlockSet.timevalue)) {
		return accept(m_listenSocket, NULL, NULL);;
	}

	return INVALID_SOCKET;
}

void ServerTCP::ReceiveNewClients() {

	static char szNickBuffer[11] = "";
	Client newClient;
	newClient.sockt = ListenToClients();

	if (newClient.sockt != INVALID_SOCKET) {
		recv(newClient.sockt, szNickBuffer, sizeof(szNickBuffer), NULL);
		//TODO: Validar nickname para no tener nombres repetidos
		if (!ValidateNickname(szNickBuffer)) {
			SendMsgToClient("DENEGADO", newClient);
		}
		else {
			newClient.nickname = szNickBuffer;
			newClient.id = ids++;
			SendMsgToClient("ACEPTADO", newClient);
			std::cout << newClient.nickname << " ha ingresado al servidor." << std::endl;
			SendMsgToAllClients(newClient.nickname + " ha ingresado al servidor");
			m_clients.push_back(newClient);
			m_currentConnections++;
			ZeroMemory(szNickBuffer,sizeof(szNickBuffer));	
		}
	}

}

void ServerTCP::ManageInputs() {

	if (_kbhit()) {
		char t = _getch();
		switch (t)
		{
		case '\x1b':
			m_serverOpen = false;
			break;
		default:
			break;
		}
	}

}

void ServerTCP::ProcessMessages() {
	
	static char szMessageBuffer[256] = "";

	if (MessageReceived()) {

		for (int i = (int)m_clients.size() - 1; i >= 0; i--) {
			if (!m_clients.size()) continue;
			if (FD_ISSET(m_clients[i].sockt, &m_noBlockSet.fdRead)) {

				bool clientDisconected = false;
				std::string sendMessage = m_clients[i].nickname;

				if (recv(m_clients[i].sockt, szMessageBuffer, sizeof(szMessageBuffer), NULL) < 0)
					clientDisconected = true;

				else if (szMessageBuffer[0] == '/')
					ProcessCommand(szMessageBuffer, m_clients[i], clientDisconected);

				if (clientDisconected) {
					std::cout << m_clients[i].nickname << " se ha desconectado." << std::endl;
					m_clients[i].ClearInvitationsList();
					m_clients.erase(m_clients.begin() + i);
					sendMessage += " se ha desconectado";
					SendMsgToAllClients(sendMessage);
					m_currentConnections--;
				}

				else if(szMessageBuffer[0] != '/'){
					std::cout << m_clients[i].nickname << " envio: " << szMessageBuffer << std::endl;
					sendMessage += ": ";
					sendMessage += szMessageBuffer;
					SendMsgToAllClients(sendMessage, &m_clients[i].id);
				}
			}
		}
	}

}

void ServerTCP::ProcessGames()
{
	char buffer[4] = "";
	std::string dataToSend = "MXX";
	for(int i = m_tronGames.size()-1; i >= 0; i--)  {
		if(m_tronGames[i].m_gameStarted){
			if(MessageReceived(*m_tronGames[i].GetPlayer(TronGame::P0))){
				if (recv(m_tronGames[i].GetPlayer(TronGame::P0)->sockt, buffer, sizeof(buffer), NULL) < 0) {
					dataToSend = "Error";
					std::cout << m_tronGames[i].GetPlayer(TronGame::P0)->nickname << " se ha desconectado" << std::endl;
					send(m_tronGames[i].GetPlayer(TronGame::P1)->sockt, dataToSend.data(), dataToSend.size() + 1, NULL);
					m_clients.push_back(*m_tronGames[i].GetPlayer(TronGame::P1));
					std::cout << m_clients.back().nickname << " regresado al lobby" << std::endl;
					m_tronGames.erase(m_tronGames.begin() + i);
					m_currentConnections++;
					continue;
				}
				dataToSend[1] = buffer[1];
			}
			if (MessageReceived(*m_tronGames[i].GetPlayer(TronGame::P1))) {
				if (recv(m_tronGames[i].GetPlayer(TronGame::P1)->sockt, buffer, sizeof(buffer), NULL) < 0) {
					dataToSend = "Error";
					std::cout << m_tronGames[i].GetPlayer(TronGame::P1)->nickname << " se ha desconectado" << std::endl;
					send(m_tronGames[i].GetPlayer(TronGame::P0)->sockt, dataToSend.data(), dataToSend.size() + 1, NULL);
					m_clients.push_back(*m_tronGames[i].GetPlayer(TronGame::P0));
					std::cout << m_clients.back().nickname << " regresado al lobby" << std::endl;
					m_tronGames.erase(m_tronGames.begin() + i);
					m_currentConnections++;
					continue;
				}
				dataToSend[2] = buffer[1];
			}
			if (buffer[0] == 'F') {
				std::cout << "Partida terminada entre " << m_tronGames[i].GetPlayer(TronGame::Player::P0)->nickname;
				std::cout << " y " << m_tronGames[i].GetPlayer(TronGame::Player::P1)->nickname << std::endl;
				m_clients.push_back(*m_tronGames[i].GetPlayer(TronGame::P0));
				std::cout << m_clients.back().nickname << " regresado al lobby" << std::endl;
				m_clients.push_back(*m_tronGames[i].GetPlayer(TronGame::P1));
				std::cout << m_clients.back().nickname << " regresado al lobby" << std::endl;
				m_tronGames.erase(m_tronGames.begin() + i);
				m_currentConnections += 2;
			}
			else {
				send(m_tronGames[i].GetPlayer(TronGame::P0)->sockt, dataToSend.data(), dataToSend.length() + 1, NULL);
				send(m_tronGames[i].GetPlayer(TronGame::P1)->sockt, dataToSend.data(), dataToSend.length() + 1, NULL);
			}
		}
		else
		{
			short time = 6 - (short)m_tronGames[i].m_start.getElapsedTime().asSeconds();
			send(m_tronGames[i].GetPlayer(TronGame::P0)->sockt, (char*)&time, sizeof(time), NULL);
			send(m_tronGames[i].GetPlayer(TronGame::P1)->sockt, (char*)&time, sizeof(time), NULL);
			if (time <= 0)
				m_tronGames[i].m_gameStarted = true;
		}
	}
}

void ServerTCP::CreateGame(Client &player0, Client &player1)
{

	player0.ClearInvitationsList();
	player1.ClearInvitationsList();

	TronGame newGame;
	newGame.Create(player0, player1);
	m_tronGames.push_back(newGame);
	std::cout << player0.nickname << " y " << player1.nickname << " han comenzado a jugar Tron." << std::endl;
	
	for (int i = static_cast<int>(m_clients.size()) - 1; i >= 0; i--)	
	{
		if (m_clients[i].id == player0.id || m_clients[i].id == player1.id)
			m_clients.erase(m_clients.begin() + i);
	}
	newGame.m_start.restart();
	m_currentConnections -= 2;

}

void ServerTCP::ProcessCommand(std::string command, Client& client, bool& isDisconected) {

	if (command.length() == 1)
		SendListOfCommands(client);
	else if (command == m_commands[EXIT])
		isDisconected = true;
	else if (command == m_commands[ONLINE])
		SendListOfClients(client);
	else if (command == m_commands[MSG])
		SendMsgToClient("Formato de mensaje: /msg <usuario> <mensaje>", client);
	else if (command == m_commands[PLAY])
		SendMsgToClient("Usa este comando para invitar a otro usuario a jugar: /play <usuario>", client);
	else if (command == m_commands[ACCEPT])
		SendMsgToClient("Usa este comando para aceptar la invitacion a un juego: /accept <usuario>", client);
	else {
		std::string dest = command.substr(0, 4);
		if(command.length() >= 8 && dest == "/msg"){			
			dest = command.substr(5);
			size_t pos = dest.find(' ');
			if (pos != std::string::npos && command[4] == ' ') {
				std::string message = dest.substr(pos+1);
				if (message.length()) {
					dest = dest.substr(0, pos);
					if (dest == client.nickname){
						SendMsgToClient("No puedes enviarte mensajes a ti mismo.", client);
						return;
					}
					for (auto& clients : m_clients) {
						if (clients.id != client.id) {
							if (clients.nickname == dest){
								SendMsgToClient("/Mensaje de "+ client.nickname + ": " + message, clients);
								SendMsgToClient("/Para " + clients.nickname + ": " + message, client);
								std::cout << client.nickname << 
								" envio '" << message << "' a " << 
								clients.nickname << 
								std::endl;
								return;
							}
						}
					}	
					SendMsgToClient("No se logro encontrar a " + dest, client); return;
				}
			}
		}
		else if (command.find(m_commands[PLAY]) != std::string::npos) {
			if (command[5] == ' ') {
				dest = command.substr(6);
				for (auto& clt : m_clients)
					if (dest == clt.nickname){
						if(clt.id != client.id){
							clt.AddIDToInvitationList(client.id);
							SendMsgToClient("/Se ha enviado la invitaciond de juego a " + dest,client);
							SendMsgToClient("/Recibiste una invitacion de juego de " + client.nickname, clt);
						}
						else
						{
							SendMsgToClient("No puedes invitarte a jugar a ti mismo.", client);
						}
						return;
					}
				SendMsgToClient("No se encontro al jugador " + dest, client);
				return;
			}
		}
		else if (command.find(m_commands[ACCEPT]) != std::string::npos) {
			if (command[7] == ' ') {
				dest = command.substr(8);
				for (auto& clt : m_clients) {
					if (clt.nickname == dest) {
						if (clt.id == client.id) {
							SendMsgToClient("What??",client);
						}
						else
						{
							if (client.ReceivedInvitationFromID(clt.id)) {
								SendMsgToClient("/Juego aceptado, empezando partida...",client);
								SendMsgToClient("/Juego aceptado, comenzando partida...",clt);
								CreateGame(clt, client);
							}
							else
								SendMsgToClient(clt.nickname + " no te invito a jugar.", client);
						}
						return;
					}
				}
				SendMsgToClient("No se logro encontrar al usuario especificado.", client);
				return;
			}
		}
		SendMsgToClient("Comando desconocido.", client);	
	}
	
}

bool ServerTCP::MessageReceived() {

	if (!m_clients.size()) return false;

	FD_ZERO(&m_noBlockSet.fdRead);
	for (auto& clients : m_clients)
		FD_SET(clients.sockt, &m_noBlockSet.fdRead);

	if (select(NULL, &m_noBlockSet.fdRead, NULL, NULL, &m_noBlockSet.timevalue))
		return true;

	return false;

}

bool ServerTCP::MessageReceived(Client& client)
{
	FD_ZERO(&m_noBlockSet.fdRead);
	FD_SET(client.sockt,&m_noBlockSet.fdRead);
	
	if (select(NULL, &m_noBlockSet.fdRead, NULL, NULL, &m_noBlockSet.timevalue))
		return true;

	return false;
}

bool ServerTCP::ValidateNickname(const char* nickname)
{
	std::string nick = nickname;
	for (auto& client : m_clients)
		if (client.nickname == nick) return false;
	return true;
}

void ServerTCP::SendMsgToClient(const std::string& message, const Client& client) {

	send(client.sockt, message.c_str(), message.length() + 1, NULL);

}

void ServerTCP::SendMsgToAllClients(const std::string& message, uint16_t* exception) {

	for (auto&clients : m_clients) {
		if (exception)
			if (clients.id == *exception)
				continue;
		send(clients.sockt, message.c_str(), message.length() + 1, NULL);
	}

}

void ServerTCP::SendListOfCommands(const Client& client) {

	int it = 0;
	SendMsgToClient("---Lista de Comandos---", client);
	for (auto& commands : m_commands) {
		Sleep(30);
		SendMsgToClient(std::to_string(it + 1) + ".-" + m_commands[it++], client);
	}

}

void ServerTCP::SendListOfClients(const Client& client) {

	int it = 0;
	SendMsgToClient("---Personas Online---", client);
	for (auto& clients : m_clients){
		Sleep(30);
		SendMsgToClient(std::to_string(++it) + ".-" + clients.nickname,	client);
	}

}

void ServerTCP::Close() {

	std::cout << "Cerrando servidor...";
	for (auto& clients : m_clients)
		closesocket(clients.sockt);
	closesocket(m_listenSocket);
	WSACleanup();
	Sleep(1000);

}