#pragma once
#include <SFML/Network.hpp>
#include <list>
#include <deque>
#include <random>

class Client;
using Game = std::pair<std::pair<Client*, Client*>, bool>;

class Server
{
	sf::TcpListener m_listener;
	sf::SocketSelector m_selector;
	std::default_random_engine m_gen;

	std::list<Client*> m_clients;
	std::deque<Game> m_games;
public:

	Server();
	void AddNewPlayer();
	void CheckDisconnect();
	void StartNewGames();
	void RecSendData();
	void ExitGame();
};
