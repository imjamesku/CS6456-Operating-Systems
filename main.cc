#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <algorithm>

#define DEBUG
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

std::vector<char*> Command::getArgs() {
    return this->args;
}



std::vector<std::string> split(const std::string &s);
std::vector<char*> stringVectorToCharPtrVector(std::vector<std::string> &tokens);
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
        std::cout << "invalid command\n";
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
        D(std::cout << "status:" << status << std::endl;)
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

// Checks if the command is valid
bool processCommand(
    std::vector<std::string> &tokens,
    std::string &input,
    std::string &output){
    input = "";
    output = "";

    for(std::size_t i=0; i<tokens.size(); i++) {
        // output
        if (tokens[i] == ">") {
            if (i+1 < tokens.size() && output == "") {
                output = tokens[i+1];
                tokens[i] = tokens[i+1] = "";
            } else {
                return false;
            }
        } else if (tokens[i] == "<") {
            if (i+1 < tokens.size() && input == "") {
                input = tokens[i+1];
                tokens[i] = tokens[i+1] = "";
            } else {
                return false;
            }
        }
    }
    tokens.erase(std::remove(tokens.begin(), tokens.end(), ""), tokens.end());
    return true;
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
