#include "../headers/Server.hpp"

#include <unistd.h> // needed to include on Mac

// ------------ Constructors -----------------------------

bool Server::_signal = false; //-> initialize the static boolean

// default
Server::Server() : _serverSocketFd(-1), _serverName("ft_irc")
{
    std::cout << "(Server) Default constructor\n";
}

// destructor
Server::~Server()
{
    std::cout << "(Server) Destructor\n";

    // close all open sockets / fds
    shutdown();
}

// ------------ Member FTs -------------------------------------------------

// PRIVATE ----------------

/* (SERVER INTERNAL LOGIC)
removing all clients&sockets that had errors or dsconnected
*/
void Server::_disconnect_clients(void)
{
    if (_clients_to_disconnect.size() == 0) // no clients to disconnect
        return;

    std::vector<int>::iterator it = _clients_to_disconnect.end(); // end() i spast last elem
    it--;
    while (!_clients_to_disconnect.empty())
    {
        int client_fd = _clients_to_disconnect.back();
        _remove_single_client(client_fd);
        _clients_to_disconnect.pop_back();
    }
}

/* removes single client
!!! Client *irc connection* needs to be CLOSED at this point
*/
void Server::_remove_single_client(int client_fd)
{
    std::cout << "Removing a client..\n";

    // delete & erase Client Instance from map
    std::map<int, Client *>::iterator it = Clients.find(client_fd);
    delete (it->second);
    Clients.erase(it);

    // remove pollfd struct
    std::vector<struct pollfd>::iterator pit = _pollfds.begin();
    while (pit != _pollfds.end())
    {
        if (pit->fd == client_fd)
        {
            _pollfds.erase(pit);
            break;
        }
        pit++;
    }

    // close client socket
    close(client_fd);
}

/* checking if each char is numeric
 */
bool Server::_str_is_digit(std::string str)
{
    for (size_t i = 0; i < str.length(); i++)
    {
        if (std::isdigit(str[i]) == 0)
            return false;
    }
    return true;
}

/*validating port & pw
    -> throws EXCEPTIONS if invalid
*/
void Server::_validate_args(char *argv[])
{
    std::string port = argv[1];
    std::string pw = argv[2];

    // check port not empty
    if (!port.empty())
    {
        // check port numbers only and within valid range between 0 and 65535
        if (_str_is_digit(port))
        {
            std::stringstream ss;
            int num;

            ss << port;
            ss >> num;
            if (num < 0 || num > 65535)
                throw std::runtime_error("ERROR: PORT outside valid range.");
            else
                _port = num;
        }
        else
            throw std::runtime_error("ERROR: PORT invalid.");
    }
    else
        throw std::runtime_error("ERROR: PORT cannot be empty.");

    // check pw not empty
    if (!pw.empty())
        _password = pw;
    else
        throw std::runtime_error("ERROR: PASSWORD cannot be empty.");
}

void Server::_setup_signal_handling(void)
{
    struct sigaction sa;
    sa.sa_handler = &Server::SignalHandler;

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);  // ctrl c
    sigaction(SIGQUIT, &sa, NULL); // ctrl backslash
    sigaction(SIGTERM, &sa, NULL); // kill

    // SIGPIPE ignore
	sa.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sa, NULL); // kill

}

/* creates CLIENT class & corresponding socket
 */
void Server::_accept_new_client(void)
{
    std::cout << "accepting new client....\n";

    Client *NewClient = new Client;
    struct pollfd newpollfd;
    struct sockaddr_in clientaddr;

    socklen_t len = sizeof(clientaddr);
    // accept (create new socket)
    int clientfd = accept(_serverSocketFd, (sockaddr *)&clientaddr, &len);
    if (clientfd == -1)
        throw std::runtime_error("ERROR: accepting new connection.");

    // set new socket opt NONBLOCK
    if (fcntl(clientfd, F_SETFL, O_NONBLOCK) == -1)
        throw std::runtime_error("ERROR: NEW CLIENT setting O_NONBLOCK failed.");

    // create new pollfd for poll()
    newpollfd.fd = clientfd;
    newpollfd.events = POLLIN;
    newpollfd.revents = 0;
    _pollfds.push_back(newpollfd);

    // create new Client instance
    NewClient->fd = clientfd;
    NewClient->ip_address = inet_ntoa(clientaddr.sin_addr); // conv net addr to string
    Clients.insert(std::make_pair(clientfd, NewClient));    // add to client map

    std::cout << "NEW CLIENT ACCEPTED!" << std::endl;

    // std::string sendTest = "lololool\r\n";
    // replyToClient(NewClient, sendTest);
}

/* INTERFACE SERVER -> PARSER & HANDLER

reads incoming data, passes it to PARSER & HANDLER
*/
void Server::_receive_data(int fd)
{
    char buf[1024];
    memset(buf, 0, sizeof(buf)); // avoid garbage

    ssize_t bytes = recv(fd, &buf, sizeof(buf) - 1, 0);

    // check if client disconnected (bytes -1 or 0)
    if (bytes <= 0 && (errno != EAGAIN && errno != EWOULDBLOCK)) // EAGAIN & EWOULDBLOCK signal try again later -> not real error
        _clients_to_disconnect.push_back(fd);

    else // process data
    {
        std::cout << "received: " << buf << std::endl;

        // append to client recv buffer (but check against overflow)
        std::map<int, Client *>::iterator it = Clients.find(fd);
        if (it->second->recv_buf.size() + bytes > MAX_RECV_BUF)
        {
            // TODO correct response
            // replyToClient(it->second, "417\r\n"); // ERR_INPUTTOOLONG, faulty client osftware
            markClientToDisconnect(it->first);
            return;
        }
        it->second->recv_buf.append(buf);

        // extract full lines if any, send each individually to PARSE + HANDLE
        bool lines_available = true;
        while (lines_available)
        {
            std::string line = it->second->extract_line();
            if (line.empty())
            {
                lines_available = false;
                break;
            }

            if (line.length() < 510) // IRC Protocol: msg MUST NOT be longer than 512 bytes total
            {
                //    INTERFACE SERVER -> PARSER & HANDLER

                // PASS to PARSER
                std::cout << "-----> SENDING LINE TO PARSE: " << line << std::endl;

                IrcMessage message;
                message = Parser::parseLine(line);
                printIrcMessage(message);

                // PASS to HANDLER
                CommandHandler handler(this, it->second, message);
                handler.handleCmd();
            }
            else // TODO test
            {
                // TODO send correct response
                // replyToClient(it->second, "417\r\n"); // ERR_INPUTTOOLONG, faulty client osftware
                markClientToDisconnect(it->first);
                break;
            }
        }
    }
}

/* (SERVER INTERNAL LOGIC)
actual sending of clients' send buf by Server poll().
Splits msgs longer than 512 bytes into multiple msgs. <-TODO

param:
        pollfd* pfd : pointer to pollfd struct for certain client

*/
void Server::_sendMsgBuf(pollfd *pfd)
{
    // get clients send buf
    std::map<int, Client *>::iterator it = Clients.find(pfd->fd);
    if (it == Clients.end())
        return; // client disconnected

    Client *c = it->second;

    // TODO check max msg size/ buf size (split if too long) 		TODO

    ssize_t bytes = send(pfd->fd, c->send_buf.data(), c->send_buf.length(), 0);

    if (bytes > 0) // some bytes were sent
    {
        // remove from sending buf
        c->send_buf.erase(0, bytes);

        // remove POLLOUT if buf now empty
        if (c->send_buf.empty())
            pfd->events &= ~POLLOUT; // remove POLLOUT but keep POLLIN (bitmask: ~ flipp all bits, A &= B keeps only the bits that are 1 in BOTH A AND B.)
    }
    else if (bytes == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
    {
        return;
    }
    else if (bytes < 0 && !(errno == EAGAIN || errno == EWOULDBLOCK)) // fatal error OR client disconnected
    {
        // mark client to disconnect
        _clients_to_disconnect.push_back(pfd->fd);
    }
}

// PUBLIC  ----------------------------------------------------------------------------------------------

/* clean exit of server in case of error or stop signal
    -> closes all existing socket connections
    ->clears ALL Clients & data
*/
void Server::shutdown(void) // + delete channels
{
	//delete channels
    for (std::map<std::string, Channel *>::iterator it = Channels.begin(); it != Channels.end(); it++)
		delete it->second;
	Channels.clear();

    // delete all client instances
    for (std::map<int, Client *>::iterator it = Clients.begin(); it != Clients.end(); it++)
        delete it->second;

    // remove map container
    Clients.clear();

    // go thru pollfd struct and close all open fds (includes Servsocket!)
    for (int i = 0; i < (int)_pollfds.size(); i++)
    {
        if (_pollfds[i].fd >= 0)
            close(_pollfds[i].fd);
    }
}

/* marking a client to be disconnected (eg after QUIT or error)
 */
void Server::markClientToDisconnect(int client_fd)
{
    _clients_to_disconnect.push_back(client_fd);
}

/* handling ctrl c, ctrl / for server shutdown
 */
void Server::SignalHandler(int signum)
{
    (void)signum;
    Server::_signal = true; //-> set the static boolean to true to stop the server
}

/* initializing server, creating server socket
    -> throws EXCEPTIONS if invalid
*/
void Server::init(char *argv[])
{
    _validate_args(argv);
    std::cout << "Port is: " << _port << std::endl
              << "Pw is: " << _password << std::endl
              << "CREATING SERVER SOCKET...\n"
              << std::endl;

    _setup_signal_handling();

    // server socket (socket - setoptions - bind - listen)
    _serverSocketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverSocketFd == -1)
    {
        std::cerr << "errno = " << errno << std::endl;
        throw std::runtime_error("ERROR: Server socket creation failed.");
    }

    // set socket conf
    //(Reuseaddr for dev)
    int enable = 1;
    if (setsockopt(_serverSocketFd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1) // SOL_SOCKET: option on socket level,
    {
        close(_serverSocketFd);
        throw std::runtime_error("ERROR: setting SO_REUSEADDR failed.");
    }
    // NONBLOCKING
    if (fcntl(_serverSocketFd, F_SETFL, O_NONBLOCK) == -1)
    {
        close(_serverSocketFd);
        throw std::runtime_error("ERROR: setting O_NONBLOCK failed.");
    }

    /*
    struct sockaddr_in {
    sa_family_t    sin_family; // AF_INET
    in_port_t      sin_port;   // port
    struct in_addr sin_addr;   // IP address
    };
    */
    struct sockaddr_in address;           // holding all network conf that our sockets need (doc https://man7.org/linux/man-pages/man3/sockaddr.3type.html)
    memset(&address, 0, sizeof(address)); // preventing garbage
    address.sin_family = AF_INET;         //-> set address family to ipv4
    address.sin_port = htons(_port);      // convert port to network byte order (big endian), network always uses big e, most modern machines little e
    address.sin_addr.s_addr = INADDR_ANY; // INADDR_ANY is a constant (0.0.0.0) defined in <netinet/in.h> (alternative: address.sin_addr.s_addr = inet_addr("192.168.1.100"))

    // bind to addr
    if (bind(_serverSocketFd, (const struct sockaddr *)&address, sizeof(address)) == -1)
    {
        close(_serverSocketFd);
        if (errno == 13)
            throw std::runtime_error("ERROR: Server socket bind failed. Use port > 1024");
        throw std::runtime_error("ERROR: Server socket bind failed.");
    }

    // make it a server side socket -> listen
    if (listen(_serverSocketFd, SOMAXCONN) == -1) // SOMAXCONN is system dependend max value of backlog(man listen) -> makes it portable
    {
        close(_serverSocketFd);
        throw std::runtime_error("ERROR: Server socket listen failed.");
    }

    // add server socket to poll struct
    struct pollfd NewPoll;
    NewPoll.fd = _serverSocketFd;
    NewPoll.events = POLLIN; // serv socket is only listening at start
    NewPoll.revents = 0;
    _pollfds.push_back(NewPoll);

    // std::time_t now = std::time(NULL);
    // _creationTime = std::ctime(&now);
    // _creationTime.erase(_creationTime.find_last_of("\n"));

    _creationTime = getCurrentTime();
    std::cout << "Server was created on " << _creationTime << std::endl;

    std::cout << "SERVER SOCKET READY!" << std::endl;
}

/* loop checks if any data is incoming on any socket,
    then decides how to process the data
    ->runs forever until signal (ctrl c, ctrl \) or crash
*/
void Server::pollLoop(void)
{
    // std::cout << "Starting to poll...\n";

    while (Server::_signal == false) // run until sig received
    {
        // poll
        if (poll(&_pollfds[0], _pollfds.size(), -1) == -1 && Server::_signal == false)
            throw std::runtime_error("ERROR: poll() failed.");

        for (size_t i = 0; i < _pollfds.size(); i++)
        {
            if (_pollfds[i].revents & POLLIN) // data to READ
            {
                if (_pollfds[i].fd == _serverSocketFd) // new client wants to connect
                    _accept_new_client();
                else
                    _receive_data(_pollfds[i].fd); // get data from registered client
            }
            if (_pollfds[i].revents & POLLOUT) // data to SEND
            {
                _sendMsgBuf(&_pollfds[i]);
            }
        }
        _disconnect_clients();
    }
}

/* (INTERFACE HANDLING -> SERVER) USE THIS TO SEND ANY MSG TO A CLIENT

!!!!  param msg needs to be correclty formatted for IRC

adds message (msg) to send to client (indicated by Client*) to outgoing send buf
-> next poll iteration is gonna pick it up and send when ready
*/
void Server::replyToClient(Client *client, const std::string &msg)
{
    std::cout << "SENDING TO CLIENT " << client->getNickname() << " : [" << msg << "]" << std::endl;

    // add msg to send_buf of client, (msg needs to be irc protocol conform)
    client->send_buf += msg;

    // enable POLLOUT on client's pollfd (so that server understands it needs to send sth)
    // get pollfd of client
    std::vector<struct pollfd>::iterator it = _pollfds.begin(); // might need to improve in case of client disconnected TODO
    while (it->fd != client->fd)
        it++;

    it->events |= POLLOUT; // adds POLLOUT while keeping POLLIN (bitmask)
}

std::string Server::getPassword(void) const
{
    return _password;
}

// TESTING

void Server::setPass(std::string pass)
{
    _password = pass;
}

// CHANNEL OPERATIONS

void Server::createChannel(const std::string &name)
{
    Channel *channel = new Channel(name, getCurrentTime());
    Channels.insert(std::make_pair(name, channel));
}

void Server::removeChannel(const std::string &name)
{
    std::map<std::string, Channel *>::iterator it = Channels.find(name);
    delete it->second;
    Channels.erase(it);
}

Channel *Server::getChannel(const std::string &name)
{
    std::map<std::string, Channel *>::iterator it = Channels.find(name);
    if (it == Channels.end())
        return NULL;
    return it->second;
}

Client *Server::getClient(const std::string &nick)
{
    std::string target = nick;
    std::transform(target.begin(), target.end(), target.begin(), ::tolower);

    for (std::map<int, Client *>::iterator it = Clients.begin(); it != Clients.end(); ++it)
    {
        if (it->second)
        {
            std::string storedNick = it->second->getNickname();
            std::transform(storedNick.begin(), storedNick.end(), storedNick.begin(), ::tolower);

            if (storedNick == target)
                return it->second;
        }
    }
    return NULL;
}

std::string Server::getCreationTime() const
{
    return _creationTime;
}

std::string Server::getCurrentTime()
{
    std::time_t now = std::time(NULL);
    std::string time = std::ctime(&now);
    time.erase(time.find_last_of("\n"));
    return time;
}
