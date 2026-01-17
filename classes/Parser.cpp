#include "Parser.hpp"
#include <iostream>
#include <sstream> // for istringstream
#include <cctype> // for toupper
#include <algorithm> // for transform
#include <set>


// private constructors
Parser::Parser(){}
Parser::Parser(const Parser& parser){ (void)parser; }
Parser& Parser::operator=(const Parser& parser) { (void)parser; return(*this); }

// the main thing: parseLine()
IrcMessage Parser::parseLine(const std::string& input)
{
    IrcMessage message;
    std::string line = input;
    handleTrailing(message, line);
    handleRest(message, line);
    normalizeCMD(message.command);

    return message;
}

// non members
template<typename Iter>
void print_range(Iter first, Iter last) {
    for (; first != last; ++first)
       std::cout << *first << " ";
}

void printIrcMessage(IrcMessage& message)
{
    std::cout<< "Command: " << message.command << std::endl;
    std::cout << "Parameters: ";
    print_range(message.params.begin(), message.params.end());
    std::cout << "\nTrailing: " << message.trailing << std::endl;
}

// private members
void Parser::handleTrailing(IrcMessage& message, std::string& line)
{
    int colonPos = line.find(" :");
    if (colonPos == -1)
        return;
    message.trailing = line.substr(colonPos + 2);
    line = line.substr(0, colonPos);
    
}

// the istringstream implements input operations o string based streams.
// It effectively stores an instance of std::basic_string and performs input operations on it.
// The operator  >> extract values from an input stream, potentially skipping preceding whitespaces.
// The value is stored to a given reference
// The iss>>temp evaluates as true/false depending on whether the extraction succeeded or not
void Parser::handleRest(IrcMessage& message, std::string& line)
{
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string temp;

    while (iss >> temp)
        tokens.push_back(temp);

    if (!tokens.empty())
    {
        message.command = tokens[0];
        message.params.assign(tokens.begin()+1, tokens.end());
    }
        

}
void Parser::normalizeCMD(std::string& cmd)
{
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
}

std::vector<std::string> Parser::splitByComma(const std::string &param, bool removeDuplicates)
{
    std::vector<std::string> vec;
    std::set<std::string> uniques;
    std::stringstream  ss(param);
    std::string str;
    while (getline(ss, str, ','))
    {
        if ((removeDuplicates && uniques.insert(str).second) || !removeDuplicates)
            vec.push_back(str);
    }
    (void)uniques;
        
    // std::cout << "params split by comma: ";
    // for (std::vector<std::string>::iterator it = vec.begin(); it != vec.end(); it++)
    // {
    //     std::cout << *it << " ";
    // }
    // std::cout << std::endl;
    return vec;
}