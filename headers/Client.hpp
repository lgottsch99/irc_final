#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <iostream>
#include <string>
#include <set>

// #include "Channel.hpp"

class Channel;

class Client {
    private:
        bool _authenticated; // PASS ok
        bool _registered;    // NICK+USER+PASS completed, WELCOME sent

        std::string _nickname;
        std::string _username;
        std::string _realname;

        std::set<Channel*> _channels; // Channels the client has joined
        std::set<std::string> _invitedChannelNames; // must store names because IRC allowed to invite an user to an non-existent channel

    public:
        Client(void);
        ~Client(void);

        std::string  getNickname(void) const;
        std::string  getUsername(void) const;
        std::string  getRealname(void) const;

        bool isAuthenticated(void) const;
        bool isRegistered(void) const;
        
	    bool hasChannel(Channel*) const;
        void addToChannel(Channel*);
        void leaveChannel(Channel*);
        std::set<Channel*> getChannels() const;

        void setNickname(const std::string &);
        void setUsername(const std::string &);
        void setRealname(const std::string &);
        void setAuthenticated(bool v);
        void setRegistered(bool v);

        void addInvited(const std::string& channelName);
        void removeInvited(const std::string& channelName);
        bool isInvited(std::string channel);

        // for Lilli
        int fd;                 // client socket fd. needed by server
        std::string ip_address; // client ip. mainly for identification and logging
        std::string recv_buf;   // buffer for incoming data. needed by server
        std::string send_buf;   // buffer to send msg back to client (numeric replies or chat msgs). needed by server

        std::string extract_line(void); // needed by server
};

#endif
