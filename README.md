*This project has been created as part of the 42 curriculum by lgottsch, yhusieva, jgrigorj.*

# IRC Server (ft_irc)

## Description

This project consists of implementing a simplified **IRC (Internet Relay Chat) server** in C++, following the IRC protocol specifications.  
The goal is to understand and apply concepts related to **network programming**, **TCP/IP communication**, **socket management**, and **event-driven architectures**, while respecting the constraints of modern C++.

The server allows multiple clients to connect simultaneously, communicate through channels, and exchange private messages using a subset of the IRC protocol commands.

This project emphasizes:
- Low-level network programming
- Non-blocking I/O
- Proper protocol parsing
- Robust error handling
- Clean object-oriented design in C++

### Features

- TCP server using IPv4
- Multiple simultaneous clients
- Non-blocking sockets
- Client authentication (PASS, NICK, USER)
- Channel creation and management
- Join / leave channels
- Private messages and channel messages
- Operator privileges
- Basic error handling according to IRC replies

## Instructions

### Requirements

- C++ compiler supporting **C++98**
- Unix-based system (Linux or macOS)
- `make`

### Compilation

To compile the server, run:

```bash
make
```
### Run the program
```bash
./ircserv <port_number> <password>
```
For instance:
```bash
./ircserv 1111 pass
```
Then run the client of your choice: **irssi**, **weechat**.
Connect to the server (shown for irssi):
```bash
/connect <server_ip_address> <port_number> <password>
```

## Resources
- The irc protocol: https://www.rfc-editor.org/rfc/rfc1459.html
- Another irc protocol desription https://modern.ircdocs.horse/
- the client irssi: https://irssi.org/User-interface/