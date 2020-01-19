#include <string>
class State
{
private:
    int status;
    std::string lastCommandOutput;
public:
    State(/* args */);
    ~State();
    void setStatus(int status);
    int getStatus();
    void setLastCommandOutput(std::string output);
    std::string getLastCommandOutput();
};