#include "State.h"

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
void State::setLastCommandOutput(std::string output) {
    this->lastCommandOutput = output;
}

std::string State::getLastCommandOutput() {
    return this->lastCommandOutput;
}