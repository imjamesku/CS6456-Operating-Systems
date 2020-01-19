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

class State
{
private:
    int status;
public:
    State(/* args */);
    ~State();
    void setStatus(int status);
    int getStatus();
};

State::State(/* args */)
{
}

State::~State()
{
}

void State::setStatus(int status) {
    this->status = status;
}
int State::getStatus() {
    return this->status;
}


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
std::vector<std::string> split(const std::string &s, const std::string delimiter);
std::vector<char*> stringVectorToCharPtrVector(std::vector<std::string> &tokens);
bool isOperator(std::string s);


bool processCommand(
    std::vector<std::string> &tokens,
    std::string &input,
    std::string &output);

void parse_and_run_command(const std::string &command, State &state);
void parse_and_run_single_command(const std::string &command, State &state, int in, int out);

void parse_and_run_command(const std::string &command, State &state) {
    // TODO: Implement this.
    // Note that this is not the correct way to test for the exit command.
    // For example the command `exit` should also exit your shell.
    std::vector<std::string> commands = split(command, " | ");
    D(std::cout << "number of commands" << commands.size() << std::endl;)
    if (commands.size() == 0) {
        std::cerr << "invalid command" << std::endl;
    }
    int in = 0, out = 1;
    int oldFd[2];
    int newFd[2];
    int* oldFdPtr = oldFd;
    int* newFdPtr = newFd;
    for (size_t i=0; i<commands.size(); i++) {
        pipe(newFdPtr);

        D(std::cout << "oldFldPtr[0]:" << oldFdPtr[0] << " oldFdPtr[1]:" << oldFdPtr[1] << std::endl;)
        D(std::cout << "newFldPtr[0]:" << newFdPtr[0] << " newFdPtr[1]:" << newFdPtr[1] << std::endl;)

        in = i == 0 ? 0 : oldFdPtr[0];
        out = i == commands.size()-1 ? 1 : newFdPtr[1];
        
        parse_and_run_single_command(commands[i], state, in, out);
        if (i != 0) {
            close(oldFdPtr[0]);
        }
        close(newFdPtr[1]);
        int* temp = oldFdPtr;
        oldFdPtr = newFdPtr;
        newFdPtr = temp;
    }
    close(oldFdPtr[0]);
    // close(newFdPtr[1]);
    // close(newFdPtr[0]);
    
}


void parse_and_run_single_command(const std::string &command, State &state, int in, int out) {
    // TODO: Implement this.
    // Note that this is not the correct way to test for the exit command.
    // For example the command `exit` should also exit your shell.
    if (command == "exit") {
        exit(0);
    } else if (command == "status") {
        std::cout << "Status: " << state.getStatus() << std::endl;
        return;
    }

    D(std::cout << "command length: " << command.length() << std::endl;)
    D(std::cout << "in: " << in << ", out: " << out << std::endl;)
    // Parse the command line

    std::vector<std::string> tokens = split(command);
    std::string input = "", output = "";
    if (!processCommand(tokens, input, output)) {
        std::cerr << "invalid command\n";
        return;
    }
    Command commandOjb(command, tokens, input, output);

    std::vector<char*> args = commandOjb.getArgs();

    D(
        std::cout << "args:";
        for (unsigned int i=0; i<tokens.size(); i++) {
            std::cout << args[i] << ' ';
        }
        std::cout << '\n';
    )

    std::cout.flush();
    pid_t pid = fork();

    if (pid < 0) {
        std::cerr << "fork failed\n";
        return;
    }
    if (pid == 0){
        //child
        D(std::cout << "I'm the child\n";)
        // output redicetion
        if (commandOjb.getOutput() != "") {
            int fOut = open(commandOjb.getOutput().c_str(), O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
            dup2(fOut, 1);
            close(fOut);
        } else if (out != 1){
            dup2(out, 1);
        }
        if (out != 1) {
            close(out);
        }
        // input redirection
        if (commandOjb.getInput() != "") {
            int fIn = open(commandOjb.getInput().c_str(), O_RDONLY);
            if (fIn == -1) {
                perror("ERROR");
                exit(1);
            }
            dup2(fIn, 0);
            close(fIn);
        } else if (in != 0) {
            // No redirection specified. Direct to the next pipe
            dup2(in, 0);
        }
        if (in != 0) {
            close(in);
        }
        
        execve(args[0], args.data(), NULL);
        // If the program reaches here, execve has failed. Otherwise the process would've been replaced.
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
        // Set status
        if (WIFEXITED(status)) {
            if (WEXITSTATUS(status) == 0) {
                state.setStatus(0);
            } else {
                state.setStatus(255);
            }
            std::cout << "exit status: " << WEXITSTATUS(status) << std::endl;
        } else {
            state.setStatus(255);
        }
    }
    // std::cerr << "Not implemented.\n";
}

// Split the input command with space, tab or \n
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

// Split the input command with delimiters
std::vector<std::string> split(const std::string &s, const std::string delimiter) {
    std::vector<std::string> tokens;
    std::string token;

    auto start = 0U;
    auto end = s.find(delimiter);
    while (end != std::string::npos)
    {
        tokens.push_back(s.substr(start, end - start));
        start = end + delimiter.length();
        end = s.find(delimiter, start);
    }
    if(s.substr(start, end).length() > 0){
        tokens.push_back(s.substr(start, end));
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
    State state;
    while (true) {
        std::string command;
        std::cout << "> ";
        std::getline(std::cin, command);
        parse_and_run_command(command, state);
    }
    return 0;
}
