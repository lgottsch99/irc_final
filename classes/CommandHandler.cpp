#include "../headers/CommandHandler.hpp"
#include <iostream>

// ---------------- Member Functions  ----------------

void CommandHandler::handleCmd()
{
    std::cout << "Entered command handler." << std::endl;
    std::map<std::string, handlerFunc>::iterator it = _handlers.find(_cmd.command);

    if (it == _handlers.end())
        _server->sendNumeric(_client, ERR_UNKNOWNCOMMAND, _cmd.params, "Unknown command");
    else
    {
        std::cout << "Command recognised and going to the execution." << std::endl;
        (this->*it->second)();
    }
}

// ---------------- 4.1 CONNECTION REGISTRATION ----------------

bool CommandHandler::_tryRegister()
{
    if (_client->isAuthenticated())
    {
        if (!_client->getNickname().empty())
        {
            if (!_client->getUsername().empty())
                return true;
        }
    }
    return false;
}

/*
4.1.1 Password message
    Command: PASS <password>

    The password can and must be set before any attempt to register the connection is made.

    Example:
           PASS secretpasswordhere
*/
void CommandHandler::_handlePass()
{
    if (_cmd.params.empty())
        _server->sendNumeric(_client, ERR_NEEDMOREPARAMS, _cmd.params, "Not enough parameters");
    else if (_client->isRegistered())
        _server->sendNumeric(_client, ERR_ALREADYREGISTERED, _cmd.params, "You may not reregister");
    else if (_cmd.params[0] == _server->getPassword())
    {
        _client->setAuthenticated(true);
        std::cout << "Authorised successfully with the password from client: " << _cmd.params[0] << std::endl;
    }
    else
    {
        _client->setAuthenticated(false);
        _server->sendNumeric(_client, ERR_PASSWDMISMATCH, _cmd.params, "Password incorrect");
    }
}

void CommandHandler::_sendRegistrationNumerics()
{
    _server->sendNumeric(_client, RPL_WELCOME, _cmd.params, "Welcome to the server!");
    _server->sendNumeric(_client, RPL_YOURHOST, _cmd.params, "Your host is ft_irc, running version 1.0");
    _server->sendNumeric(_client, RPL_CREATED, _cmd.params, "This server was created " + _server->getCreationTime());
    _server->sendNumeric(_client, RPL_MYINFO, _cmd.params, "ft_irc 1.0 o itkol");
    _server->sendNumeric(_client, RPL_ISUPPORT, _cmd.params, "CHANTYPES=#& PREFIX=(o)@ CHANMODES=itkol NICKLEN=30 :are supported by this server");
}

/*
4.1.2 Nick message
    Command: NICK <nickname>

    NICK message is used to give user a nickname or change the previous one.

    Examples:
        NICK Wiz                        ; Introducing new nick "Wiz".
*/
void CommandHandler::_handleNick()
{
    if (_cmd.params.empty())
        _server->sendNumeric(_client, ERR_NONICKNAMEGIVEN, _cmd.params, "No nickname given");
    else if (_checkNickChars(_cmd.params[0]))
        _server->sendNumeric(_client, ERR_ERRONEUSNICKNAME, _cmd.params, "Erroneus nickname");
    else if (_isNameDublicate(_server, _cmd.params[0], &CommandHandler::_compareNick))
        _server->sendNumeric(_client, ERR_NICKNAMEINUSE, _cmd.params, "Nickname is already in use");
    else
    {
        if (!_client->getNickname().empty())
            _server->broadcastToAllChannels("User changed their nickname", _client);
        _client->setNickname(_cmd.params[0]);
        std::cout << "Set the nickname successfully to: " << _client->getNickname() << std::endl;
    }
    if (!_client->isRegistered() && _tryRegister())
    {
        _client->setRegistered(true);
        _sendRegistrationNumerics();
    }
}

/*
4.1.3 User message
    Command: USER <username> <hostname> <servername> :<realname>

    Only after both USER and NICK have been received from a client does a user become registered.

    Examples:
        USER guest :Ronnie Reagan   ; User registering themselves with a
                                    username of "guest" and real name
                                    "Ronnie Reagan".
*/
void CommandHandler::_handleUser()
{
    if (_client->isRegistered())
        _server->sendNumeric(_client, ERR_ALREADYREGISTERED, _cmd.params, "You may not reregister");
    else if (_cmd.params.empty() || _cmd.trailing.empty())
        _server->sendNumeric(_client, ERR_NEEDMOREPARAMS, _cmd.params, "Not enough parameters");
    else
    {
        _client->setUsername(_cmd.params[0]);
        _client->setRealname(_cmd.trailing);
        std::cout << "Set the username: " << _client->getUsername() << " and realname: " << _client->getRealname() << std::endl;
    }
    if (!_client->isRegistered() && _tryRegister())
    {
        _client->setRegistered(true);
        _sendRegistrationNumerics();
    }
}

/*
4.1.6 Quit
    Command: QUIT [<Quit message>]

    A client session is ended with a quit message. If a "Quit Message" is given,
    this will be sent instead of the default message, the nickname.

    Examples:
        QUIT :Gone to have lunch        ; Preferred message format.
*/
void CommandHandler::_handleQuit()
{
    if (!_cmd.trailing.empty())
        _server->broadcastToAllChannels(_cmd.trailing, _client);
    else
        _server->broadcastToAllChannels(_client->getNickname() + " left the server.", _client);

    std::set<Channel *> channels = _client->getChannels();
    for (std::set<Channel *>::iterator it = channels.begin(); it != channels.end(); ++it)
        _removeClientFromChannel(_client, *it);

    _server->markClientToDisconnect(_client->fd);
}

// ---------------- 4.2 CHANNEL OPERATIONS ----------------

void CommandHandler::_addUserToChannel(Channel *channel)
{
    channel->addUser(_client);
    _client->addToChannel(channel);

    _server->sendJoin(_client, channel); // <-- JOIN reply

    _server->sendNumeric(_client, RPL_NAMREPLY, _cmd.params, channel->listNames());
    _server->sendNumeric(_client, RPL_ENDOFNAMES, _cmd.params, "End of /NAMES list");

    if (!channel->getTopic().empty())
        _server->sendNumeric(_client, RPL_TOPIC, _cmd.params, channel->getTopic());
    else
        _server->sendNumeric(_client, RPL_NOTOPIC, _cmd.params, "No topic is set");
}

/*
4.2.1 Join message
    Command: JOIN <channel>{,<channel>} [<key>{,<key>}]

    The JOIN command is used by a client to start listening to a specific channel.

    Examples:
        JOIN #foobar                    ; join channel #foobar.
        JOIN &foo fubar                 ; join channel &foo using key "fubar".
        JOIN #foo,&bar fubar            ; join channel #foo using key "fubar"
                                        and &bar using no key.
        JOIN #foo,#bar fubar,foobar     ; join channel #foo using key "fubar".
                                        and channel #bar using key "foobar".
        JOIN #foo,#bar                  ; join channels #foo and #bar.
*/
void CommandHandler::_handleJoin()
{
    if (_cmd.params.empty())
        _server->sendNumeric(_client, ERR_NEEDMOREPARAMS, _cmd.params, "Not enough parameters");
    else if (!_client->isRegistered())
        _server->sendNumeric(_client, ERR_NOTREGISTERED, _cmd.params, "You have not registered");
    else
    {
        std::vector<std::string> channels = Parser::splitByComma(_cmd.params[0], false);
        std::vector<std::string> keys;
        if (_cmd.params.size() >= 2)
            keys = Parser::splitByComma(_cmd.params[1], false);
        else
            keys.assign(channels.size(), "");

        // now use the channel and key vectors instead of just _cmd.params[0] and _cmd.params[1]
        for (unsigned int i = 0; i < channels.size(); i++)
        {
            if ((channels[i][0] != '#' && channels[i][0] != '&') || channels[i].length() > CHAN_MAX)
                _server->sendNumeric(_client, ERR_NOSUCHCHANNEL, _cmd.params, "No such channel");

            else
            {
                std::map<std::string, Channel *>::iterator it = _server->Channels.find(channels[i]);

                if (it == _server->Channels.end())
                {
                    std::cout << "Channel not found - creating a new one: " << channels[i] << std::endl;
                    _server->createChannel(channels[i]);
                    it = _server->Channels.find(channels[i]);
                    it->second->addOperator(_client);

                    _addUserToChannel(it->second);
                }
                else if (!_client->hasChannel(it->second))
                {
                    if (it->second->isInviteOnly() && !_client->isInvited(it->first))
                        _server->sendNumeric(_client, ERR_INVITEONLYCHAN, _cmd.params, "Cannot join channel (+i)");
                    else if (it->second->hasKey() && ((keys.size() >= channels.size() && keys[i] != it->second->getKey()) || keys.size() < i))
                        _server->sendNumeric(_client, ERR_BADCHANNELKEY, _cmd.params, "Cannot join channel (+k)");
                    else if (it->second->getUserLimit() > 0 && it->second->getUserLimit() <= it->second->getNumOfUsers())
                        _server->sendNumeric(_client, ERR_CHANNELISFULL, _cmd.params, "Cannot join channel (+l)");
                    else
                        _addUserToChannel(it->second);
                }
            }
        }
    }
}

/*
4.2.5 Names message
    Command: NAMES [<channel>{,<channel>}]

    By using the NAMES command, a user can list all nicknames that are
    visible to them on any channel that they can see.

    If no <channel> parameter is given, a list of all channels and their
    occupants is returned.

    There is no error reply for bad channel names.

    Examples:
    NAMES #twilight_zone,#42        ; list visible users on #twilight_zone
                                    and #42 if the channels are visible to
                                    you.
    NAMES                           ; list all visible channels and users
*/
void CommandHandler::_handleNames()
{
    if (_cmd.params.empty())
    {
        for (std::map<std::string, Channel *>::iterator it = _server->Channels.begin(); it != _server->Channels.end(); ++it)
            _server->sendNumeric(_client, RPL_NAMREPLY, _cmd.params, it->second->listNames());
        _server->sendNumeric(_client, RPL_ENDOFNAMES, _cmd.params, "End of /NAMES list");
    }
    else
    {
        std::vector<std::string> channels = Parser::splitByComma(_cmd.params[0]);
        for (size_t i = 0; i < channels.size(); i++)
        {
            Channel *channel = _server->getChannel(channels[i]);
            if (!channel)
                return;

            _server->sendNumeric(_client, RPL_NAMREPLY, _cmd.params, channel->listNames());
            _server->sendNumeric(_client, RPL_ENDOFNAMES, _cmd.params, "End of /NAMES list");
        }
    }
}

// util to remove clients, used in PART, KICK, QUIT
void CommandHandler::_removeClientFromChannel(Client *client, Channel *channel)
{
    if (channel->isOperator(client))
        channel->removeOperator(client);
    channel->removeUser(client);
    client->leaveChannel(channel);

    if (!channel->hasOperators())
    {
        Client *newOper = *channel->getUsers().begin();
        channel->addOperator(newOper);
    }
    if (!channel->hasUsers())
        _server->removeChannel(channel->getName());
}

/*
4.2.8 Kick command
    Command: KICK <channel> <user> [<comment>]

    The KICK command can be  used  to  forcibly  remove  a  user  from  a
    channel.   It  'kicks  them  out'  of the channel (forced PART).

    Examples:
        KICK &Melbourne Matthew                 ; Kick Matthew from &Melbourne
        KICK #Finnish John :Speaking English    ; Kick John from #Finnish using
                                                "Speaking English" as the reason (comment).
*/
void CommandHandler::_handleKick()
{
    if (_cmd.params.size() < 2)
        _server->sendNumeric(_client, ERR_NEEDMOREPARAMS, _cmd.params, "Not enough parameters");
    else
    {
        Channel *channel = _server->getChannel(_cmd.params[0]);

        if (!channel)
            _server->sendNumeric(_client, ERR_NOSUCHCHANNEL, _cmd.params, "No such channel");
        else if (!channel->hasUser(_client))
            _server->sendNumeric(_client, ERR_NOTONCHANNEL, _cmd.params, "You're not on that channel");
        else if (!channel->isOperator(_client))
            _server->sendNumeric(_client, ERR_CHANOPRIVSNEEDED, _cmd.params, "You're not channel operator");
        else
        {
            Client *kickedClient = _server->getClient(_cmd.params[1]);

            if (!kickedClient)
                _server->sendNumeric(_client, ERR_NOSUCHNICK, _cmd.params, "No such nick/channel");
            else if (!channel->hasUser(kickedClient))
                _server->sendNumeric(_client, ERR_USERNOTINCHANNEL, _cmd.params, "They aren't on that channel");
            else
            {
                // _server->broadcastFromUser(_client, _cmd.command, _cmd.params, _cmd.trailing, channel);
				_server->broadcastEveryone(_client, _cmd.command, _cmd.params, _cmd.trailing, channel);

                _removeClientFromChannel(kickedClient, channel);
            }
        }
    }
}

/* 4.2.2 Part message
    Command: PART <channel>{,<channel>} <reason>

    The PART message causes the client sending the message to be removed
    from the list of active users for all given channels listed in the
    parameter string.

    Numeric Replies:
            ERR_NEEDMOREPARAMS              ERR_NOSUCHCHANNEL
            ERR_NOTONCHANNEL
    Examples:
        PART #twilight_zone             ; leave channel "#twilight_zone"
        PART #oz-ops,&group5            ; leave both channels "&group5" and
                                        "#oz-ops".*/
void CommandHandler::_handlePart()
{
    if (_cmd.params.empty())
        _server->sendNumeric(_client, ERR_NEEDMOREPARAMS, _cmd.params, "Not enough parameters");
    else
    {
        std::vector<std::string> channels = Parser::splitByComma(_cmd.params[0]);
        for (size_t i = 0; i < channels.size(); i++)
        {
            Channel *channel = _server->getChannel(channels[i]);

            if (!channel)
                _server->sendNumeric(_client, ERR_NOSUCHCHANNEL, _cmd.params, "No such channel");
            else if (!channel->hasUser(_client))
                _server->sendNumeric(_client, ERR_NOTONCHANNEL, _cmd.params, "You're not on that channel");
            else
            {
                // _server->broadcastFromUser(_client, _cmd.command, _cmd.params, _cmd.trailing, channel);
                _server->broadcastEveryone(_client, _cmd.command, _cmd.params, _cmd.trailing, channel);

				_removeClientFromChannel(_client, channel);
            }
        }
    }
}

/*
4.2.7 Invite message
    Command: INVITE <nickname> <channel>

    The INVITE message is used to invite users to a channel.

    Examples:
        INVITE Wiz #Twilight_Zone       ; Command to invite WiZ to #Twilight_zone
*/
void CommandHandler::_handleInvite()
{
    if (_cmd.params.size() < 2)
        _server->sendNumeric(_client, ERR_NEEDMOREPARAMS, _cmd.params, "Not enough parameters");
    else
    {
        Client *invitedClient = _server->getClient(_cmd.params[0]);
        Channel *channel = _server->getChannel(_cmd.params[1]);

        if (!invitedClient)
            _server->sendNumeric(_client, ERR_NOSUCHNICK, _cmd.params, "No such nick/channel");
        else if (channel)
        {
            if (!channel->hasUser(_client))
                _server->sendNumeric(_client, ERR_NOTONCHANNEL, _cmd.params, "You're not on that channel");
            else if (channel->isInviteOnly() && !channel->isOperator(_client))
                _server->sendNumeric(_client, ERR_CHANOPRIVSNEEDED, _cmd.params, "You're not channel operator");
            else if (channel->hasUser(invitedClient))
                _server->sendNumeric(_client, ERR_USERONCHANNEL, _cmd.params, "is already on channel");
            else
            {
                _server->sendNumeric(_client, RPL_INVITING, _cmd.params, "");
                invitedClient->addInvited(_cmd.params[1]);
                _server->sendReaction(invitedClient, "INVITE " + invitedClient->getNickname(), channel->getName());
                // _server->replyToClient(invitedClient, );
            }
        }
        else
        {
            _server->sendNumeric(_client, RPL_INVITING, _cmd.params, "");
            invitedClient->addInvited(_cmd.params[1]);
            _server->sendReaction(invitedClient, "INVITE " + invitedClient->getNickname(), channel->getName());
        }
    }
}

/*
4.2.4 Topic message
    Command: TOPIC <channel> [<topic>]

    The TOPIC message is used to change or view the topic of a channel.

    Examples:
        TOPIC #test :another topic      ;set the topic on #test to "another topic".
        TOPIC #test                     ;check the topic for #test.
*/
void CommandHandler::_handleTopic()
{
    if (_cmd.params.empty())
        _server->sendNumeric(_client, ERR_NEEDMOREPARAMS, _cmd.params, "Not enough parameters");
    else
    {
        Channel *channel = _server->getChannel(_cmd.params[0]);

        if (!channel)
            _server->sendNumeric(_client, ERR_NOSUCHCHANNEL, _cmd.params, "No such channel");
        else if (!_client->hasChannel(channel))
            _server->sendNumeric(_client, ERR_NOTONCHANNEL, _cmd.params, "You're not on that channel");
        else if (_cmd.trailing.empty())
        {
            if (channel->getTopic().empty())
                _server->sendNumeric(_client, RPL_NOTOPIC, _cmd.params, "No topic is set");
            else
                _server->sendNumeric(_client, RPL_TOPIC, _cmd.params, channel->getTopic());
        }
        else if (channel->isTopicRestricted() && !channel->isOperator(_client))
            _server->sendNumeric(_client, ERR_CHANOPRIVSNEEDED, _cmd.params, "You're not channel operator");
        else
        {
            // _server->broadcastFromUser(_client, _cmd.command, _cmd.params, _cmd.trailing, channel);
			_server->broadcastEveryone(_client, _cmd.command, _cmd.params, _cmd.trailing, channel);
			channel->setTopic(_cmd.trailing);

        }
    }
}

/*
4.4.1 Private messages
    Command: PRIVMSG <receiver>{,<receiver>} :<text to be sent>

    PRIVMSG is used to send private messages between users.  <receiver>
    is the nickname of the receiver of the message.  <receiver> can also
    be a list of names or channels separated with commas.

    Examples:
        PRIVMSG Angel :yes I'm receiving it !receiving it !'u>(768u+1n) .br
                                        ; Message to Angel.
*/
void CommandHandler::_handlePrivmsg()
{
    if (_cmd.params.empty())
        _server->sendNumeric(_client, ERR_NORECIPIENT, _cmd.params, "No recipient given " + _cmd.command);
    else if (_cmd.trailing.empty())
        _server->sendNumeric(_client, ERR_NOTEXTTOSEND, _cmd.params, "No text to send");
    else
    {
        std::vector<std::string> targets = Parser::splitByComma(_cmd.params[0]);
        for (std::vector<std::string>::iterator it = targets.begin(); it != targets.end(); it++)
        {
            if ((*it)[0] == '#' || (*it)[0] == '&')
            {
                Channel *channel = _server->getChannel(*it);

                if (!channel)
                    _server->sendNumeric(_client, ERR_NOSUCHCHANNEL, _cmd.params, "No such channel");
                else if (!channel->hasUser(_client))
                    _server->sendNumeric(_client, ERR_CANNOTSENDTOCHAN, _cmd.params, "Cannot send to channel");
                else
                    _server->sendPrivmsg(_client, channel->getName(), _cmd.trailing);
            }
            else
            {
                Client *recipient = _server->getClient(*it);

                if (!recipient)
                    _server->sendNumeric(_client, ERR_NOSUCHNICK, _cmd.params, "No such nick/channel");
                else
                    _server->sendPrivmsg(_client, recipient->getNickname(), _cmd.trailing);
            }
        }
    }
}

/*
4.4.2 Notice
    Command: NOTICE <nickname>{,<nickname>} :<text>

    The NOTICE message is used similarly to PRIVMSG.  The difference
    between NOTICE and PRIVMSG is that automatic replies must never be
    sent in response to a NOTICE message.
*/
void CommandHandler::_handleNotice()
{
    if (_cmd.params.empty())
        return;
    else
    {
        std::vector<std::string> targets = Parser::splitByComma(_cmd.params[0]);
        // now use targets instead of _cmd.params[0]
        for (std::vector<std::string>::iterator it = targets.begin(); it != targets.end(); it++)
        {
            if (_cmd.trailing.empty())
                return;
            else if ((*it)[0] == '#' || (*it)[0] == '&')
            {
                Channel *channel = _server->getChannel(*it);

                if (!channel)
                    return;
                else if (!channel->hasUser(_client))
                    return;
                else
                    _server->broadcastFromUser(_client, _cmd.command, _cmd.params, _cmd.trailing, channel);
            }
            else
            {
                Client *recipient = _server->getClient(*it);

                if (!recipient)
                    return;
                else
                    _server->sendPrivmsg(_client, recipient->getNickname(), _cmd.trailing);
            }
        }
    }
}

// ---------------- MODES ----------------

// i - invite-only channel flag
void CommandHandler::_modeInvite(Channel *channel, const t_mode &mode)
{
    if (mode.sign == '+')
    {
        channel->setInviteMode(true);
        // _server->broadcastFromUser(_client, _cmd.command, _cmd.params, _cmd.trailing, channel);
		_server->broadcastEveryone(_client, _cmd.command, _cmd.params, _cmd.trailing, channel);

    }
    else
    {
        channel->setInviteMode(false);
        // _server->broadcastFromUser(_client, _cmd.command, _cmd.params, _cmd.trailing, channel);
		_server->broadcastEveryone(_client, _cmd.command, _cmd.params, _cmd.trailing, channel);
    }
}

// t - set the restrictions on the topic modification
void CommandHandler::_modeTopic(Channel *channel, const t_mode &mode)
{
    if (mode.sign == '+')
    {
        channel->setTopicMode(true);
        // _server->broadcastFromUser(_client, _cmd.command, _cmd.params, _cmd.trailing, channel);
		_server->broadcastEveryone(_client, _cmd.command, _cmd.params, _cmd.trailing, channel);
    }
    else
    {
        channel->setTopicMode(false);
        // _server->broadcastFromUser(_client, _cmd.command, _cmd.params, _cmd.trailing, channel);
		_server->broadcastEveryone(_client, _cmd.command, _cmd.params, _cmd.trailing, channel);
    }
}

// k - set a channel key (password)
void CommandHandler::_modeKey(Channel *channel, const t_mode &mode)
{
    if (mode.sign == '+')
    {
        if (channel->hasKey())
            _server->sendNumeric(_client, ERR_KEYSET, _cmd.params, "Channel key already set");
        else if (mode.arg.empty())
            _server->sendNumeric(_client, ERR_NEEDMOREPARAMS, _cmd.params, "Not enough parameters");
        else
        {
            channel->setKey(mode.arg);
            // _server->broadcastFromUser(_client, _cmd.command, _cmd.params, _cmd.trailing, channel);
			_server->broadcastEveryone(_client, _cmd.command, _cmd.params, _cmd.trailing, channel);

        }
    }
    else
    {
        channel->removeKey();
        // _server->broadcastFromUser(_client, _cmd.command, _cmd.params, _cmd.trailing, channel);
		_server->broadcastEveryone(_client, _cmd.command, _cmd.params, _cmd.trailing, channel);

    }
}

// l - set the user limit to channel
void CommandHandler::_modeLimit(Channel *channel, const t_mode &mode)
{
    if (mode.sign == '+')
    {
        if (mode.arg.empty())
            _server->sendNumeric(_client, ERR_NEEDMOREPARAMS, _cmd.params, "Not enough parameters");
        else
        {
            unsigned long n = strtoul(mode.arg.c_str(), NULL, 10);

            if (n == 0 || n == ULONG_MAX || errno == ERANGE || n > UINT_MAX)
                _server->sendNumeric(_client, ERR_INVALIDMODEPARAM, _cmd.params, "Invalid parameter. Must be a valid positive number");
            else
            {
                channel->setLimit(static_cast<unsigned int>(n));
                // _server->broadcastFromUser(_client, _cmd.command, _cmd.params, _cmd.trailing, channel);
            	_server->broadcastEveryone(_client, _cmd.command, _cmd.params, _cmd.trailing, channel);
			}
        }
    }
    else
    {
        channel->removeLimit();
        // _server->broadcastFromUser(_client, _cmd.command, _cmd.params, _cmd.trailing, channel);
		_server->broadcastEveryone(_client, _cmd.command, _cmd.params, _cmd.trailing, channel);

    }
}

// o - set channel operator privilege
void CommandHandler::_modeOperator(Channel *channel, const t_mode &mode)
{
    if (mode.arg.empty())
        _server->sendNumeric(_client, ERR_NEEDMOREPARAMS, _cmd.params, "Not enough parameters");
    else
    {
        Client *newOper = _server->getClient(mode.arg);

        if (!newOper)
            _server->sendNumeric(_client, ERR_NOSUCHNICK, _cmd.params, "No such nick/channel");
        else if (!channel->hasUser(newOper))
            _server->sendNumeric(_client, ERR_USERNOTINCHANNEL, _cmd.params, "They aren't on that channel");
        else if (mode.sign == '+')
        {
            channel->addOperator(newOper);
            // _server->broadcastFromUser(_client, _cmd.command, _cmd.params, _cmd.trailing, channel);
			_server->broadcastEveryone(_client, _cmd.command, _cmd.params, _cmd.trailing, channel);

        }
        else
        {
            channel->removeOperator(newOper);
            // _server->broadcastFromUser(_client, _cmd.command, _cmd.params, _cmd.trailing, channel);
			_server->broadcastEveryone(_client, _cmd.command, _cmd.params, _cmd.trailing, channel);

        }
    }
}

void CommandHandler::_init_modes()
{
    _modes['i'] = &CommandHandler::_modeInvite;
    _modes['k'] = &CommandHandler::_modeKey;
    _modes['l'] = &CommandHandler::_modeLimit;
    _modes['t'] = &CommandHandler::_modeTopic;
    _modes['o'] = &CommandHandler::_modeOperator;
}

/*	typedef struct s_mode
    {
        char sign;
        char mode;
        std::string arg;
    } t_mode;

    typedef std::vector<t_mode> t_mode_vect;
*/

CommandHandler::t_mode_vect CommandHandler::_parseMode(const IrcMessage &_cmd)
{
    t_mode_vect modes;

    if (_cmd.params.size() < 2)
        return modes;

    const std::string &modeStr = _cmd.params[1];
    char sign = 0;
    size_t argIndex = 2; // 0 is for #channel, 1 is for modestring, args start at 2
    for (size_t i = 0; i < modeStr.size(); i++)
    {
        char c = modeStr[i];
        if (c == '+' || c == '-')
        {
            sign = c;
            continue;
        }

        t_mode m;
        m.sign = sign;
        m.mode = c;

        if (_modeNeedsArg(c, sign))
        {
            if (argIndex < _cmd.params.size())
                m.arg = _cmd.params[argIndex++];
            else
            {
                m.arg = ""; // throw ERR_NEEDMOREPARAMS here or later
            }
        }
        modes.push_back(m);
    }
    return modes;
}

bool CommandHandler::_modeNeedsArg(char mode, char sign)
{
    if (mode == 'i' || mode == 't')
        return false;
    else if ((mode == 'k' || mode == 'l') && sign == '-')
        return false;
    else
        return true;
}

/*
4.2.3 Mode message
    Command: MODE <channel> {[+|-]|o|i|t|l|k} [<limit>] [<user>]

    The MODE command is provided so that channel operators may change the
    characteristics of `their' channel.

    | Mode | Needs arg when `+` | Needs arg when `-` |
    | ---- | ------------------ | ------------------ |
    | i    | no                 | no                 |
    | t    | no                 | no                 |
    | k    | yes                | no                 |
    | l    | yes                | no                 |
    | o    | yes                | yes                |
*/
void CommandHandler::_handleMode()
{
    if (_cmd.params.empty())
        _server->sendNumeric(_client, ERR_NEEDMOREPARAMS, _cmd.params, "Not enough parameters");
    else
    {
        Channel *channel = _server->getChannel(_cmd.params[0]);

        if (!channel)
            _server->sendNumeric(_client, ERR_NOSUCHCHANNEL, _cmd.params, "No such channel");
        else if (!_client->hasChannel(channel))
            _server->sendNumeric(_client, ERR_NOTONCHANNEL, _cmd.params, "You're not on that channel");
        else if (_cmd.params.size() == 1)
        {
            _server->sendNumeric(_client, RPL_CHANNELMODEIS, _cmd.params, channel->listActiveModes());
            _server->sendNumeric(_client, RPL_CREATIONTIME, _cmd.params, channel->getCreationTime());
        }
        else if (!channel->isOperator(_client))
            _server->sendNumeric(_client, ERR_CHANOPRIVSNEEDED, _cmd.params, "You're not channel operator");
        else
        {
            t_mode_vect modes = _parseMode(_cmd);

            _init_modes();
            for (size_t i = 0; i < modes.size(); i++) // iterates twice for one invalid mode
            {
                std::map<char, modeFunc>::iterator it = _modes.find(modes[i].mode);

                if (it == _modes.end() || !(modes[i].sign == '+' || modes[i].sign == '-'))
                    _server->sendNumeric(_client, ERR_UNKNOWNMODE, _cmd.params, "is unknown mode char to me");
                else
                    (this->*it->second)(channel, modes[i]);
            }
        }
    }
}

/*
Capability negotiation is started by the client issuing a CAP LS 302 command
(indicating to the server support for IRCv3.2 capability negotiation).
Negotiation is then performed with
the CAP REQ, CAP ACK, and CAP NAK commands, and is ended with the CAP END command.
    Command: CAP <subcommand> [<parameters>...]
*/
void CommandHandler::_handleCap()
{
    if (_cmd.params.empty())
        return;
    else
    {
        const std::string subcommand = _cmd.params[0];

        if (subcommand == "LS")
            _server->sendReaction(_client, "CAP * LS :", "");
        else if (subcommand == "REG")
            _server->sendReaction(_client, "NAK", _cmd.trailing); // we dont support any capabilities, always reject
        else if (subcommand == "END")
            return;
        else
            _server->sendNumeric(_client, ERR_INVALIDCAPCMD, _cmd.params, "Invalid subcommand");
    }
}

// ---------------- Connecton check ----------------

void CommandHandler::_handlePing()
{
    if (_cmd.params.empty())
        _server->sendNumeric(_client, ERR_NEEDMOREPARAMS, _cmd.params, "Not enough parameters");
    else if (_cmd.params[0].empty())
        _server->sendNumeric(_client, ERR_NOORIGIN, _cmd.params, "No origin specified");

    _server->sendReaction(_client, "PONG", _cmd.trailing);
    // _server->replyToClient(_client, "PONG :ft_irc " + _cmd.trailing + "\r\n");
}

void CommandHandler::_handlePong()
{
    return;
}


// ---------------- Utils ----------------

bool CommandHandler::_compareNick(Client *client, const std::string &name)
{
    return client->getNickname() == name;
}

bool CommandHandler::_compareUser(Client *client, const std::string &name)
{
    return client->getUsername() == name;
}

// Util to check whether a client with the same name already exists
bool CommandHandler::_isNameDublicate(Server *server, std::string name, bool (CommandHandler::*compareFunc)(Client *, const std::string &))
{
    for (std::map<int, Client *>::iterator it = server->Clients.begin(); it != server->Clients.end(); ++it)
    {
        if (it->second && (this->*compareFunc)(it->second, name))
            return true;
    }
    return false;
}

bool CommandHandler::_checkNickChars(const std::string &name)
{
    // the nick should not start with digits or hyphens or a slash
    if (isdigit(static_cast<unsigned char>(name[0])) ||
        name[0] == '-' || name[0] == '/')
        return true;
    // allowed chars: A-Z, a-z, 0-9, [ ] \ ^ _ { | } -
    std::string specialChars = "[]\\^_{|}-";
    for (int i = 0; name[i]; i++)
    {
        if (!(isalnum(name[i]) || specialChars.find(name[i]) != name.npos) || isspace(name[i]))
            return true;
    }
    // the nick should not be longer than NICK_MAX
    if (name.length() > NICK_MAX)
        return true;
    return false;
}

// ---------------- Constructors ----------------

CommandHandler::CommandHandler(void) : _server(NULL), _client(NULL), _cmd(IrcMessage())
{
    // std::cout << "(CommandHandler) Default constructor\n";
}

CommandHandler::CommandHandler(Server *server, Client *client, const IrcMessage &cmd) : _server(server), _client(client), _cmd(cmd)
{
    // std::cout << "(CommandHandler) Overload constructor\n";

    _handlers["PASS"] = &CommandHandler::_handlePass;
    _handlers["NICK"] = &CommandHandler::_handleNick;
    _handlers["USER"] = &CommandHandler::_handleUser;
    _handlers["QUIT"] = &CommandHandler::_handleQuit;
    _handlers["JOIN"] = &CommandHandler::_handleJoin;
    _handlers["MODE"] = &CommandHandler::_handleMode;
    _handlers["TOPIC"] = &CommandHandler::_handleTopic;
    _handlers["INVITE"] = &CommandHandler::_handleInvite;
    _handlers["KICK"] = &CommandHandler::_handleKick;
    _handlers["PING"] = &CommandHandler::_handlePing;
    _handlers["PONG"] = &CommandHandler::_handlePong;
    _handlers["NOTICE"] = &CommandHandler::_handleNotice;
    _handlers["PRIVMSG"] = &CommandHandler::_handlePrivmsg;
    _handlers["NAMES"] = &CommandHandler::_handleNames;
    _handlers["CAP"] = &CommandHandler::_handleCap;
    _handlers["PART"] = &CommandHandler::_handlePart;
}

CommandHandler::~CommandHandler(void)
{
    // std::cout << "(CommandHandler) Destructor\n";
}
