#pragma once
#ifndef CLIENT_H
#define CLIENT_H
#include <winsock.h>
#include <list>
#include <string>

class Client
{
public:
	std::string nickname;
	SOCKET sockt;
	uint16_t id;
	bool ReceivedInvitationFromID(uint16_t id);
	void ClearInvitationsList();
	void AddIDToInvitationList(uint16_t id);
private:
	std::list<uint16_t> m_invitations;
};

#endif // !CLIENT_H

