#include "../headers/Server.hpp"
#include "../headers/Client.hpp"
#include <iomanip>

void Server::sendNumeric(Client *c, Numeric code, const std::vector<std::string> &params, const std::string &trailing)
{
    std::ostringstream msg;
    msg << ":" << _serverName
        << " " << std::setw(3) << std::setfill('0') << code
        << " " << (!c->getNickname().empty() ? c->getNickname() : "*");

    for (size_t i = 0; i < params.size(); i++)
        msg << " " << params[i];

    if (!trailing.empty())
        msg << " :" << trailing;

    // check length and truncate
    if (msg.width() > MAX_MESSAGE_LEN - 2)
        msg.width(MAX_MESSAGE_LEN - 2);

    msg << "\r\n";

    replyToClient(c, msg.str());
}

void Server::sendReaction(Client *c, const std::string &param, const std::string &trailing)
{
    std::ostringstream msg;
    msg << ":" << _serverName
        // << " " << (!c->getNickname().empty() ? c->getNickname() : "*")
        << " " << param;

    if (!trailing.empty())
        msg << " :" << trailing;

    // check length and truncate
    if (msg.width() > MAX_MESSAGE_LEN - 2)
        msg.width(MAX_MESSAGE_LEN - 2);

    msg << "\r\n";

    replyToClient(c, msg.str());
}

void Server::sendServerNotice(Client *c, const std::string& text, const std::string& target = "")
{
    std::ostringstream msg;

    msg << ":" << _serverName << " NOTICE ";

    if (!target.empty())
        msg << target;
    else
        msg << (!c->getNickname().empty() ? c->getNickname() : "*");

    msg << " :" << text << "\r\n";

    replyToClient(c, msg.str());
}

void Server::sendChannelNotice(Channel *channel, const std::string& text)
{
    std::ostringstream msg;
    msg << ":" << _serverName
        << " NOTICE " << channel->getName()
        << " :" << text << "\r\n";

    std::set<Client *> users = channel->getUsers();

    for (std::set<Client *>::iterator it = users.begin(); it != users.end(); it++)
    {
        replyToClient(*it, msg.str());
    }
}

// void Server::pingClient(Client *c)
// {
//     std::ostringstream msg;
//     msg << "PING :" << serverName << "\r\n";
//     c.send(msg.str());
// }

// this is to reserved for fatal errors that should be followed by disconnecting the client
void Server::sendError(Client *c, const std::string& reason)
{
    replyToClient(c, "ERROR :" + reason + "\r\n");
}

void Server::broadcastFromUser(
    Client *from,
    const std::string &command,
    const std::vector<std::string> &params,
    const std::string &trailing,
    const Channel *channel) // can just pass the whole class IrcMessage instead of its parts separately to make the prototype simpler
{
    // (void)channel;
    std::ostringstream msg;

    msg << ":" << from->getNickname()
        << "!" << from->getUsername()
        // << "@" << from.hostname()
        << " " << command;

    for (size_t i = 0; i < params.size(); i++)
        msg << " " << params[i];

    if (!trailing.empty())
        msg << " :" << trailing;

    // check length and truncate
    if (msg.width() > MAX_MESSAGE_LEN - 2)
        msg.width(MAX_MESSAGE_LEN - 2);

    msg << "\r\n";

    broadcastToOneChannel(msg.str(), from, channel);
}

// operator nickname starts with '@'

void Server::broadcastToOneChannel(const std::string &msg, Client *client, const Channel *channel)
{
    std::set<Client *> users = channel->getUsers();

    for (std::set<Client *>::iterator it = users.begin(); it != users.end(); ++it)
    {
        if (*it == client)
            continue;
        replyToClient(*it, msg);
    }
}

//topic
void Server::broadcastEveryone(
    Client *from,
    const std::string &command,
    const std::vector<std::string> &params,
    const std::string &trailing,
    const Channel *channel) // can just pass the whole class IrcMessage instead of its parts separately to make the prototype simpler
{
    // (void)channel;
    std::ostringstream msg;

    msg << ":" << from->getNickname()
        << "!" << from->getUsername()
        // << "@" << from.hostname()
        << " " << command;

    for (size_t i = 0; i < params.size(); i++)
        msg << " " << params[i];

    if (!trailing.empty())
        msg << " :" << trailing;

    // check length and truncate
    if (msg.width() > MAX_MESSAGE_LEN - 2)
        msg.width(MAX_MESSAGE_LEN - 2);

    msg << "\r\n";

    broadcastInclSender(msg.str(), from, channel);
}

//topic
void Server::broadcastInclSender(const std::string &msg, Client *client, const Channel *channel)
{
	(void)client;
    std::set<Client *> users = channel->getUsers();

    for (std::set<Client *>::iterator it = users.begin(); it != users.end(); ++it)
    {
        replyToClient(*it, msg);
    }
}


void Server::broadcastToAllChannels(const std::string &trailing, Client *client)
{
    std::set<Channel *> channels = client->getChannels();

    for (std::set<Channel*>::iterator it = channels.begin(); it != channels.end(); ++it){
        broadcastToOneChannel(trailing, client, *it);
    }
}

void Server::sendPrivmsg(Client *from, const std::string& target, const std::string& text)
{
    std::ostringstream msg;
    msg << ":" << from->getNickname() 
    << "!" << from->getUsername()
    << "@" << from->ip_address
    << " PRIVMSG " << target
    << " :" << text << "\r\n";

    if (getChannel(target))
        broadcastToOneChannel(msg.str(), from, getChannel(target));
    else if (getClient(target))
        replyToClient(getClient(target), msg.str());

}


void Server::sendJoin(Client *client, Channel *channel) //for JOIN reply
{
    std::ostringstream msg;

    msg << ":" << client->getNickname()
        << "!" << client->getUsername()
        // << "@" << client->getHost()
        << " JOIN :" << channel->getName()
        << "\r\n";

    std::set<Client *> users = channel->getUsers();

    for (std::set<Client *>::iterator it = users.begin(); it != users.end(); ++it)
    {
        replyToClient(*it, msg.str()); // INCLUDING client
    }
}