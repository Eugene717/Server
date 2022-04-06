#include "Server.h"

int main()
{
	Server server;

	while (true)
	{
		server.AddNewPlayer();
		server.StartNewGames();
		server.CheckDisconnect();
		server.RecSendData();
		server.ExitGame();
	}

	return 0;
}
