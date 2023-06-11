#include "Shell.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <sys/wait.h>

void Shell::run() {
    while (true) {
        std::string command;
        std::cout << "shell> ";
        std::getline(std::cin, command);
        if (command == "exit") {
            break;
        }
        else if (command == "myhistory") {
            showHistory();
        }
        else if (command == "myjobs") {
            processManager.showJobs();
        }
        else {
            bool isBackground = isBackgroundCommand(command);
            addToHistory(command);
            execute(command, isBackground);
        }
    }
}

void Shell::addToHistory(const std::string& command) {
    commandHistory.push_back(command);
}

void Shell::showHistory() {
    int count = 1;
    for (const auto& line : commandHistory) {
        std::cout << count << ". " << line << std::endl;
        count++;
    }
}

void Shell::execute(const std::string& command, bool isBackground) {
    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "Failed to create child process." << std::endl;
        return;
    }
    else if (pid == 0) {
        executeChildCommand(command);
    }
    else {
        if (isBackground) {
            processManager.addBackgroundProcess(pid);
            std::cout << "Background process with PID " << pid << " started." << std::endl;
        }
        else {
            int status;
            waitpid(pid, &status, 0);
        }
    }
}

void Shell::executeChildCommand(const std::string& command) {
    std::istringstream iss(command);
    std::string program;
    iss >> program;

    // Get the command arguments
    std::string argument;
    std::vector<std::string> arguments;
    while (iss >> argument) {
        arguments.push_back(argument);
    }

    // Prepare the arguments for execvp
    const char* programChar = program.c_str();
    std::vector<char*> args;
    args.push_back(const_cast<char*>(programChar));
    for (const auto& arg : arguments) {
        args.push_back(const_cast<char*>(arg.c_str()));
    }
    args.push_back(nullptr);

    // Execute the command
    if (execvp(programChar, args.data()) == -1) {
        std::cerr << "Failed to execute command: " << program << std::endl;
        exit(EXIT_FAILURE);
    }
}

bool Shell::isBackgroundCommand(std::string& command) {
    size_t pos = command.find_last_not_of(" \t\n\r\f\v");
    if (pos != std::string::npos) {
        command.erase(pos + 1);
        if (command.back() == '&') {
            command.pop_back();
            return true;
        }
    }
    return false;
}