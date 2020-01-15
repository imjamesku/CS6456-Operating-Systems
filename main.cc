#include <cstdlib>
#include <iostream>
#include <string>

void parse_and_run_command(const std::string &command) {
    // TODO: Implement this.
    // Note that this is not the correct way to test for the exit command.
    // For example the command `exit` should also exit your shell.
    if (command == "exit") {
        exit(0);
    }
    std::cerr << "Not implemented.\n";
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
