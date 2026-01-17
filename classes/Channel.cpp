#include "../headers/Channel.hpp"
#include <iostream>

// ---------------- Member Functions ----------------

std::string Channel::listNames() const
{
    std::string nicknames = "";

    for (std::set<Client *>::iterator it = _users.begin(); it != _users.end(); ++it)
    {
        if (isOperator(*it))
            nicknames += "@";
        nicknames += (*it)->getNickname() + " ";
    }
    return nicknames;
}

std::string Channel::listActiveModes() const
{
    std::string activeModes = "+";
    std::string args = "";

    if (_mode_i)
        activeModes += "i";
    if (_mode_t)
        activeModes += "t";
    if (_mode_k)
    {
        activeModes += "k";
        args += _key + " ";
    }
    if (_mode_l)
    {
        activeModes += "l";
        args += _userLimit;
    }
    return activeModes;
}

// ---------------- Operations with users ----------------

bool Channel::hasOperators() const
{
    return !_operators.empty();
}

bool Channel::hasUsers() const
{
    return !_users.empty();
}

bool Channel::hasUser(Client *client) const
{
    if (_users.find(client) != _users.end())
        return true;
    return false;
}

bool Channel::isOperator(Client *client) const
{
    if (_operators.find(client) == _operators.end())
        return false;
    return true;
}

void Channel::addUser(Client *client)
{
    std::cout << "Adding a user to the channel: " << _name << std::endl;
    _users.insert(client);
}
void Channel::removeUser(Client *client)
{
    std::cout << "Removing a user from the channel: " << _name << std::endl;
    _users.erase(client);
}
void Channel::addOperator(Client *client)
{
    std::cout << "Adding an operator to the channel: " << _name << std::endl;
    _operators.insert(client);
}
void Channel::removeOperator(Client *client)
{
    std::cout << "Removing an operator from the channel: " << _name << std::endl;
    _operators.erase(client);
}

// ---------------- Getters ----------------

const std::string &Channel::getName() const
{
    return _name;
}

const std::string &Channel::getKey(void) const
{
    return _key;
}

unsigned int Channel::getUserLimit(void) const
{
    return _userLimit;
}

unsigned int Channel::getNumOfUsers(void) const
{
    return _users.size();
}

std::string Channel::getTopic(void) const
{
    return _topic;
}

std::string Channel::getCreationTime() const
{
    return _created;
}

bool Channel::isInviteOnly(void) const
{
    return _mode_i;
}

bool Channel::isTopicRestricted() const
{
    return _mode_t;
}

bool Channel::hasKey(void) const
{
    return _mode_k;
}

std::set<Client *> Channel::getUsers() const
{
    return _users;
}

std::set<Client *> Channel::getOperators() const
{
    return _operators;
}

// ---------------- Setters + Modes ----------------

void Channel::setTopic(const std::string &topic)
{
    _topic = topic;
    _mode_t = true;
}

void Channel::setKey(const std::string &key)
{
    _key = key;
    _mode_k = true;
}

void Channel::setLimit(unsigned int n)
{
    _userLimit = n;
    _mode_l = true;
}

void Channel::setInviteMode(bool v)
{
    _mode_i = v;
}

void Channel::setTopicMode(bool v)
{
    _mode_t = v;
}

void Channel::removeKey(void)
{
    _key.clear();
    _mode_k = false;
}

void Channel::removeLimit(void)
{
    _userLimit = 0;
    _mode_l = false;
}

// ---------------- Constructors ----------------

Channel::Channel(void)
    : _name(""), _topic(""), _key(""), _userLimit(0), _created(""),
      _mode_i(false), _mode_t(false), _mode_k(false), _mode_l(false)
{
    std::cout << "(Channel) Default constructor\n";
}

Channel::Channel(const std::string &name, const std::string &creationTime)
    : _name(name), _topic(""), _key(""), _userLimit(0), _created(creationTime),
      _mode_i(false), _mode_t(false), _mode_k(false), _mode_l(false)
{
    std::cout << "(Channel) Overload constructor\n";
}

Channel::~Channel(void)
{
    std::cout << "(Channel) Destructor\n";
}
