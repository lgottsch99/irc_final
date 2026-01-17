#ifndef SERVER_HPP
#define SERVER_HPP

#include <map>
#include <string>
#include <string.h>
#include <vector>
#include <poll.h>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <cerrno>
#include <ctime>

#include "Client.hpp"
#include "Parser.hpp"
#include "CommandHandler.hpp"
#include "Channel.hpp"
#include "NumericReplies.hpp"
//todo incl handling headers

#define MAX_RECV_BUF 10000
#define MAX_MESSAGE_LEN 512

class Server {
private:
    int         _port;
    std::string _password;
    int         _serverSocketFd;
	static bool _signal; //-> static boolean for signal, to shutdown in clean way
	std::string	_serverName; //server name for irc replies (always "ft_irc")
	std::string _creationTime;

    std::vector<struct pollfd> _pollfds;

	bool _str_is_digit(std::string str);
	void _validate_args(char *argv[]);
	void _setup_signal_handling(void);
	void _accept_new_client(void);
	void _receive_data(int client_fd);
	void _sendMsgBuf(pollfd* pfd);
	
	std::vector<int> _clients_to_disconnect; //tracker of fd of client that has some error
	
	void _disconnect_clients(void);
	void _remove_single_client(int client_fd);

public:
    Server();
    ~Server();

    // int FD → Client object
    std::map<int, Client*> Clients;

    // Name → Channel object
    std::map<std::string, Channel*> Channels;
	/* The channel is created implicitly when
	the first client joins it, and the channel ceases to
   	exist when the last client leaves it. */
	void createChannel(const std::string&); // i allocate, needs freeing somewhere OK
    void removeChannel(const std::string&);
	Channel *getChannel(const std::string&);
	Client *getClient(const std::string&);
	std::string getCurrentTime();
	std::string getCreationTime() const;

	void sendNumeric(Client* c, Numeric code, const std::vector<std::string>&params, const std::string& trailing);
	void sendReaction(Client *c, const std::string &param, const std::string &trailing);
	void sendError(Client *c, const std::string& reason);
	void sendServerNotice(Client *c, const std::string& text, const std::string& target);
	void sendChannelNotice(Channel *ch, const std::string& text);
	void sendPrivmsg(Client *from, const std::string& target, const std::string& text);
	void broadcastFromUser(Client *from, const std::string &command, const std::vector<std::string> &params, const std::string &trailing, const Channel *channel);
	void broadcastToOneChannel(const std::string &msg, Client *client, const Channel* channel);
	void broadcastToAllChannels(const std::string &trailing, Client *client); // QUIT, NICK
	
	void broadcastEveryone(Client *from, const std::string &command, const std::vector<std::string> &params, const std::string &trailing, const Channel *channel);
	void broadcastInclSender(const std::string &msg, Client *client, const Channel* channel);


	void sendJoin(Client *client, Channel *channel); //added lilli join test
	static void SignalHandler(int signum);

    void init(char *argv[]);
    void pollLoop();
	void shutdown(void); // add delete channels

	//INTERFACE handler -> server for disconnecting a client 
	void markClientToDisconnect(int client_fd);

    //INTERFACE  handler -> server for sending msg
    void replyToClient(Client* client, const std::string& msg); //msg needs to be correclty formatted for irc
	
	std::string getPassword(void) const;
	void setPass(std::string pass); // for testing, remove later
};

#endif
