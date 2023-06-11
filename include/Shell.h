#ifndef SHELL_H
#define SHELL_H

#include <string>
#include <vector>
#include "ProcessManager.h"

class Shell {
public:
    void run();

private:
    ProcessManager processManager;
    std::vector<std::string> commandHistory;
    void addToHistory(const std::string& command);
    void showHistory();
    void execute(const std::string& command, bool isBackground);
    void executeChildCommand(const std::string& command);
    bool isBackgroundCommand(std::string& command);
};

#endif  // SHELL_H