#include "../headers/Client.hpp"

// ---------------- (Trend)Setters ----------------

void Client::setNickname(const std::string &name)
{
	_nickname = name;
}

void Client::setUsername(const std::string &name)
{
	_username = name;
}

void Client::setRealname(const std::string &name)
{
	_realname = name;
}

void Client::setAuthenticated(bool v)
{
	_authenticated = v;
}

void Client::setRegistered(bool v)
{
	_registered = v;
}

// ---------------- Getters ----------------

std::string Client::getNickname(void) const
{
	return _nickname;
}

std::string Client::getUsername(void) const
{
	return _username;
}

std::string Client::getRealname(void) const
{
	return _realname;
}

bool Client::isAuthenticated(void) const
{
	return _authenticated;
}

bool Client::isRegistered(void) const
{
	return _registered;
}

bool Client::hasChannel(Channel *channel) const
{
	if (_channels.find(channel) != _channels.end())
		return true;
	return false;
}

std::set<Channel*> Client::getChannels() const
{
	return _channels;
}

//

void Client::addToChannel(Channel *channel)
{
	_channels.insert(channel);
}

void Client::leaveChannel(Channel *channel)
{
	_channels.erase(channel);
}

void Client::addInvited(const std::string &channelName)
{
	_invitedChannelNames.insert(channelName);
}

void Client::removeInvited(const std::string &channelName)
{
	_invitedChannelNames.erase(channelName);
}

bool Client::isInvited(std::string channel)
{
	if (_invitedChannelNames.find(channel) != _invitedChannelNames.end())
		return true;
	return false;
}

// ---------------- Constructors ----------------

Client::Client(void)
	: _authenticated(false), _registered(false),
	  _nickname(""), _username(""), _realname("")
{
	std::cout << "(Client) Default constructor\n";
}

Client::~Client(void)
{
	std::cout << "(Client) Destructor\n";
}

// ---------------- Member FTs ----------------

/* checks if recv_buffer includes a full irc command line.
If yes: returns that single line (without \r\n) and removes it (incl \r\n) from recv buffer
Returns:
		single line string, (if full irc cmd present)
	OR 	"" (empty str), (if no full cmd presnt)
*/
std::string Client::extract_line(void)
{
	std::string del = "\r\n"; // irc protocol
	std::string line = "";

	size_t del_pos = recv_buf.find(del);
	// if (del_pos == std::string::npos)
	// {
	// 	std::cout << "Client: No full line yet..\n";
	// }
	// else
	if (del_pos != std::string::npos)
	{
		// get line up to del
		line = recv_buf.substr(0, del_pos);
		// std::cout << "Single line: " << line << std::endl;

		// update buf
		recv_buf.erase(0, del_pos + 2);
		// std::cout << "Remaining buf:\n" << recv_buf << std::endl;
	}
	return line;
}
