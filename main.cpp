#include "headers/Server.hpp"
#include <iostream>

int main(int argc, char *argv[])
{
	(void)argv;
	if (argc != 3)
	{
		std::cout << "Usage: ./ircserv <port> <password>" << std::endl;
		return 1;
	}
	
	Server irc_server;
	
	try
	{	
		std::cout << "init server\n";
		irc_server.init(argv);
		irc_server.pollLoop();

	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		irc_server.shutdown();
	}
	return 0;
}
