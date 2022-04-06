#include "Server.h"
#include <SFML/Network.hpp>
#include <list>
#include <deque>
#include <random>
#include <iostream>

class Client
{
public:
	std::string m_name;
	sf::TcpSocket* m_socket;

	Client(sf::TcpSocket* socket): m_socket(socket) {}
	~Client() { delete m_socket; }
	void SetName(std::string&& name) { m_name = std::move(name); }
};

struct Game
{
	std::pair<Client*, Client*> m_clients;
	bool m_gameEnd;

	Game(std::pair<Client*, Client*> cli, bool b) :m_clients(cli), m_gameEnd(b) {}
	~Game() { delete m_clients.first, m_clients.second; }
};

struct ServerIMPL
{
	sf::TcpListener m_listener;
	sf::SocketSelector m_selector;
	std::default_random_engine m_gen;

	std::list<Client*> m_clients;
	std::deque<Game*> m_games;
};

Server::Server()
{
	m_pImpl = new ServerIMPL;
	m_pImpl->m_listener.listen(55055);  //just change port if want
	m_pImpl->m_selector.add(m_pImpl->m_listener);

	std::random_device rd;
	m_pImpl->m_gen.seed(rd());
}

void Server::AddNewPlayer()
{
	if (m_pImpl->m_selector.wait())
	{
		if (m_pImpl->m_selector.isReady(m_pImpl->m_listener))
		{
			sf::TcpSocket* client = new sf::TcpSocket;
			if (m_pImpl->m_listener.accept(*client) == sf::Socket::Done)
			{
				m_pImpl->m_clients.push_back(new Client(client));
				std::cout << client->getRemoteAddress() << "\t now in queue: " << m_pImpl->m_clients.size() << '\n';

				m_pImpl->m_selector.add(*client);
			}
			else
			{
				delete client;
			}
		}
	}
}

void Server::CheckDisconnect()
{
	if (m_pImpl->m_selector.wait())
	{
		for (auto i = m_pImpl->m_clients.begin(); i != m_pImpl->m_clients.end(); i++)
		{			
			if (m_pImpl->m_selector.isReady(*(*i)->m_socket))
			{
				sf::Packet packet;

				if ((*i)->m_socket->receive(packet) == sf::Socket::Status::Disconnected)
				{
					std::cout << (*i)->m_socket->getRemoteAddress() << " leave from queue\t" << "left in queue: " << m_pImpl->m_clients.size() - 1 << '\n';

					m_pImpl->m_selector.remove(*(*i)->m_socket);
					(*i)->m_socket->disconnect();
					m_pImpl->m_clients.erase(i);
					break;
				}
			}
		}
	}
}

void Server::StartNewGames()
{
	while (m_pImpl->m_clients.size() > 1)
	{
		Client* first = *m_pImpl->m_clients.begin();
		m_pImpl->m_clients.pop_front();
		Client* second = *m_pImpl->m_clients.begin();
		m_pImpl->m_clients.pop_front();

		

		std::string turn;
		m_pImpl->m_gen() % 2 == 0 ? turn = "f" : turn = "s";

		sf::Packet packet;
		if (turn == "f")
		{
			packet << "f";
			first->m_socket->send(packet);
			packet.clear();
			packet << "s";
			second->m_socket->send(packet);
			packet.clear();
		}
		else
		{
			packet << "s";
			first->m_socket->send(packet);
			packet.clear();
			packet << "f";
			second->m_socket->send(packet);
			packet.clear();
		}

		if (first->m_socket->receive(packet) == sf::Socket::Done)
		{
			packet >> first->m_name;
			second->m_socket->send(packet);
			packet.clear();
		}
		if (second->m_socket->receive(packet) == sf::Socket::Done)
		{
			packet >> second->m_name;
			first->m_socket->send(packet);
			packet.clear();
		}

		if (first->m_socket->isBlocking()) first->m_socket->setBlocking(false);
		if (second->m_socket->isBlocking()) second->m_socket->setBlocking(false);

		m_pImpl->m_games.push_back(new Game(std::make_pair(first, second), false));
		std::cout << "start game" << "\tin queue: " << m_pImpl->m_clients.size() << "\tgames: " << m_pImpl->m_games.size() << '\t';
		std::cout << first->m_name << "\t" << second->m_name << "\n";
	}
}

void Server::RecSendData()
{
	if (m_pImpl->m_selector.wait())
	{
		for (auto i = m_pImpl->m_games.begin(); i != m_pImpl->m_games.end(); i++)
		{
			if (m_pImpl->m_selector.isReady(*(*i)->m_clients.first->m_socket))
			{
				sf::Packet packet;

				if ((*i)->m_clients.first->m_socket->receive(packet) == sf::Socket::Done)
				{
					(*i)->m_clients.second->m_socket->send(packet);	//std::cout << "send to second player\n";
				}
				else
					(*i)->m_gameEnd = true;
			}
			if (m_pImpl->m_selector.isReady(*(*i)->m_clients.second->m_socket))
			{
				sf::Packet packet;

				if ((*i)->m_clients.second->m_socket->receive(packet) == sf::Socket::Done)
				{
					(*i)->m_clients.first->m_socket->send(packet);		//std::cout << "send to first player\n";
				}
				else
					(*i)->m_gameEnd = true;
			}
		}
	}
}

void Server::ExitGame()
{
	for (auto i = m_pImpl->m_games.begin(); i != m_pImpl->m_games.end(); i++)
	{
		if ((*i)->m_gameEnd)
		{
			std::cout << "game over\t" << (*i)->m_clients.first->m_name << "\t" << (*i)->m_clients.second->m_name << "\t\t" << "games left: " << m_pImpl->m_games.size() - 1 << '\n';

			m_pImpl->m_selector.remove(*(*i)->m_clients.first->m_socket);
			m_pImpl->m_selector.remove(*(*i)->m_clients.second->m_socket);
			(*i)->m_clients.first->m_socket->disconnect();
			(*i)->m_clients.second->m_socket->disconnect();

			m_pImpl->m_games.erase(i);
			break;
		}
	}
}
