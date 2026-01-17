#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <set>

#include "Client.hpp"

class Channel
{
private:
    std::string _name;
    std::string _topic;
    std::string _key;
    unsigned int _userLimit;
    std::string _created;

    bool _mode_i;
    bool _mode_t;
    bool _mode_k;
    bool _mode_l;

    std::set<Client *> _users;
    std::set<Client *> _operators;

public:
    Channel();
    Channel(const std::string &name, const std::string &creationTime);
    ~Channel();

    const std::string &getName() const;
    std::string getTopic() const;
    const std::string &getKey() const;
    unsigned int getUserLimit() const;
    unsigned int getNumOfUsers() const;
    std::string getCreationTime() const;
    bool hasOperators() const;
    bool hasUsers() const;

    bool isInviteOnly() const;
    bool hasKey() const;
    bool isTopicRestricted() const;

    void setInviteMode(bool);
    void setKey(const std::string &);
    void setLimit(unsigned int);
    void setTopicMode(bool);
    void setTopic(const std::string &);
    void removeKey(void);
    void removeLimit(void);

    bool hasUser(Client *) const;
    bool isOperator(Client *) const;
    void addUser(Client *);
    void removeUser(Client *);
    void addOperator(Client *);
    void removeOperator(Client *);

    std::set<Client *> getUsers() const;
    std::set<Client *> getOperators() const;

    std::string listActiveModes() const;
	std::string listNames() const;
};

#endif
