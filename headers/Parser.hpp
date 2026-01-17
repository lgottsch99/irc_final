#ifndef PARSER_HPP
#define PARSER_HPP

#include <vector>
#include <string>

// the client never sends a prefix in a message, the prefix is only for server->client communication
// TODO: create a function to generating a valid server->client message in pseudo BNF (Backus–Naur Form)
// example:
// message    =  [ ":" prefix " " ] command [ params ] crlf

// handle MODE with multiple modes
// JOIN - split by ','
// check KICK message (comma separated?, no white spaces?)
struct IrcMessage {
    std::string command;
    std::vector<std::string> params;
    std::string trailing;      // empty if none
};

class Parser {
    private:
        Parser();
        Parser(const Parser&);
        Parser& operator=(const Parser&);
        
        // members
        static void handleTrailing(IrcMessage&, std::string&);
        static void handleRest(IrcMessage&, std::string&);
        static void normalizeCMD(std::string& cmd);

    public:
        static IrcMessage parseLine(const std::string& input);
        static std::vector<std::string> splitByComma(const std::string &param, bool removeDuplicates = true);

};

void printIrcMessage(IrcMessage& message);

#endif
