#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <algorithm>
#include <fstream>
#include <fcntl.h>

// #define DEBUG
#ifdef DEBUG 
#define D(x) x
#else 
#define D(x)
#endif

class Command
{
private:
    /* data */
    std::string input;
    std::string output;
    std::string cmdString;
    std::vector<std::string> tokens;
    std::vector<char*> args;
    
public:
    Command(
        const std::string &command,
        std::vector<std::string> tokens,
        std::string input="",
        std::string output="");
    ~Command();

    std::string getOutput();

    std::string getInput();

    std::vector<char*> stringVectorToCharPtrVector(std::vector<std::string> &tokens);

    std::vector<char*> getArgs();

};

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



std::vector<std::string> split(const std::string &s);
std::vector<char*> stringVectorToCharPtrVector(std::vector<std::string> &tokens);
bool isOperator(std::string s);

bool processCommand(
    std::vector<std::string> &tokens,
    std::string &input,
    std::string &output);

void parse_and_run_command(const std::string &command) {
    // TODO: Implement this.
    // Note that this is not the correct way to test for the exit command.
    // For example the command `exit` should also exit your shell.

    // Parse the command line
    std::vector<std::string> tokens = split(command);
    std::string input = "", output = "";
    if (!processCommand(tokens, input, output)) {
        std::cerr << "invalid command\n";
        return;
    }
    Command commandOjb(command, tokens, input, output);

    // std::vector<char*> args = stringVectorToCharPtrVector(tokens);
    std::vector<char*> args = commandOjb.getArgs();

    D(
    std::cout << "args:";
    for (unsigned int i=0; i<tokens.size(); i++) {
        std::cout << args[i] << ' ';
    }
    std::cout << '\n';
    )
    

    if (command == "exit") {
        exit(0);
    }
    // int err = execve(args[0], args.data(), NULL);
    // return;

    std::cout.flush();
    pid_t pid = fork();
    // std::cout << "pid:" << pid << '\n';

    if (pid == 0){
        //child
        D(std::cout << "I'm the child\n";)
        // output redicetion
        if (commandOjb.getOutput() != "") {
            int out = open(commandOjb.getOutput().c_str(), O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
            dup2(out, 1);
            close(out);
        }
        // input redirection
        if (commandOjb.getInput() != "") {
            int in = open(commandOjb.getInput().c_str(), O_RDONLY);
            dup2(in, 0);
            close(in);
        }
        execve(args[0], args.data(), NULL);
        // print the error
        perror("ERROR");
        D(std::cout << "Did not execute\n";)
        exit(1);
    } 
    if (pid > 0) {
        //parent
        D(std::cout << "I'm the parent" << std::endl;)
        D(std:: cout << "child pid: " << pid << std::endl;)
        int status;
        waitpid(pid, &status, 0);
        D(std::cout << "done waiting" << std::endl;)
        if (WIFEXITED(status)) {
            std::cout << "exit status: " << WEXITSTATUS(status) << std::endl;
        }
    }
    // std::cerr << "Not implemented.\n";
}

std::vector<std::string> split(const std::string &s) {
    std::vector<std::string> tokens;
    std::istringstream iss(s);
    std::string token;
    while (iss >> token) {
        D(std::cout << "token:" << token << std::endl;)
        tokens.push_back(token);
    }
    return tokens;
}

std::vector<char*> stringVectorToCharPtrVector(std::vector<std::string> &tokens) {
    std::vector<char*> charPtrs;
    for (unsigned int i=0; i<tokens.size(); i++) {
        charPtrs.push_back(&tokens[i][0]);
    }
    charPtrs.push_back((char*)0);
    return charPtrs;
}
bool isOperator(std::string s) {
    return s == "<" || s == ">" || s == "|";
}
// Checks if the command is valid
bool processCommand(
    std::vector<std::string> &tokens,
    std::string &input,
    std::string &output){
    input = "";
    output = "";

    for(std::size_t i=0; i<tokens.size(); i++) {
        D(std::cout << "i:" << i << "token: "<< tokens[i] << '\n';)
        if (tokens[i] == "|") {
            return false;
        }
        if (tokens[i] == ">") {
            if (i+1 < tokens.size() && output == "" && !isOperator(tokens[i+1])) {
                output = tokens[i+1];
                tokens[i] = tokens[i+1] = "";
            } else {
                return false;
            }
        } else if (tokens[i] == "<") {
            if (i+1 < tokens.size() && input == "" && !isOperator(tokens[i+1])) {
                input = tokens[i+1];
                tokens[i] = tokens[i+1] = "";
            } else {
                return false;
            }
        }
    }
    tokens.erase(std::remove(tokens.begin(), tokens.end(), ""), tokens.end());
    return tokens.size() > 0;
}

int main(void) {
    while (true) {
        std::string command;
        std::cout << "> ";
        std::getline(std::cin, command);
        parse_and_run_command(command);
    }
    return 0;
}
