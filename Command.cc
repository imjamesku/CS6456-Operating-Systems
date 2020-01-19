#include "Command.h"

Command::Command(
    const std::string &command,
    std::vector<std::string> tokens,
    std::string input,
    std::string output)
{
    this->tokens = tokens;
    this->cmdString = command;
    this->input = input;
    this->output = output;
    this->args = stringVectorToCharPtrVector(this->tokens);
}

Command::~Command()
{
}

std::vector<char*> Command::stringVectorToCharPtrVector(std::vector<std::string> &tokens) {
    std::vector<char*> charPtrs;
    for (unsigned int i=0; i<tokens.size(); i++) {
        charPtrs.push_back(&tokens[i][0]);
    }
    charPtrs.push_back((char*)0);
    return charPtrs;
}

std::string Command::getOutput() {
    return this->output;
}

std::string Command::getInput() {
    return this->input;
}

std::vector<char*> Command::getArgs() {
    return this->args;
}