#pragma once

struct ServerIMPL;

class Server
{
	ServerIMPL* m_pImpl;
public:
	Server();
	void AddNewPlayer();
	void CheckDisconnect();
	void StartNewGames();
	void RecSendData();
	void ExitGame();
};
