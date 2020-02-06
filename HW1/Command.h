#include <string>
#include <vector>
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
