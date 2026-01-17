#ifndef COMMANDHANDLER_HPP
#define COMMANDHANDLER_HPP

#include "Server.hpp"
#include <climits>

#define NICK_MAX 30 
#define CHAN_MAX 200

class Server;

class CommandHandler	{
	private:
		Server *_server;
		Client *_client;
		IrcMessage _cmd;

		typedef struct s_mode
		{
			char sign;
			char mode;
			std::string arg;
		} t_mode;

		typedef std::vector<t_mode> t_mode_vect;

		typedef void (CommandHandler::*handlerFunc)(void);
		typedef void (CommandHandler::*modeFunc)(Channel *channel, const t_mode &mode);

		std::map<std::string, handlerFunc> _handlers;
		std::map<char, modeFunc> _modes;

		void _handlePass();
		void _handleNick();
		void _handleUser();
		void _handleQuit();
		void _handleJoin();
		void _handleMode();
		void _handleTopic();
		void _handleInvite();
		void _handleKick();
		void _handlePing();
		void _handlePong();
		void _handleNotice();
		void _handlePrivmsg();
		void _handleNames();
		void _handleCap();
		void _handlePart();

		void _modeInvite(Channel *channel, const t_mode &mode);
		void _modeKey(Channel *channel, const t_mode &mode);
		void _modeLimit(Channel *channel, const t_mode &mode);
		void _modeTopic(Channel *channel, const t_mode &mode);
		void _modeOperator(Channel *channel, const t_mode &mode);

		void _sendRegistrationNumerics();
		void _addUserToChannel(Channel *channel);
		void _init_modes();
		t_mode_vect _parseMode(const IrcMessage &_cmd);
		bool _modeNeedsArg(char mode, char sign);
		void _removeClientFromChannel(Client *client, Channel *channel);

		bool _isNameDublicate(Server *server, std::string name, bool (CommandHandler::*compareFunc)(Client *, const std::string &));
		bool _compareNick(Client *client, const std::string &name);
		bool _checkNickChars(const std::string &name);
		bool _compareUser(Client *client, const std::string &name);

		bool _tryRegister();

	public:
		CommandHandler();
		CommandHandler(Server *server, Client *client, const IrcMessage &cmd);
		~CommandHandler();

		void handleCmd();
};

#endif
