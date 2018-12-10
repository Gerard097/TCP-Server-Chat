#include "Client.h"

bool Client::ReceivedInvitationFromID(uint16_t id)
{
	for (auto &inv : m_invitations)
		if (inv == id)
			return true;

	return false;
}

void Client::ClearInvitationsList()
{
	m_invitations.clear();
}

void Client::AddIDToInvitationList(uint16_t id)
{
	m_invitations.push_back(id);
}
