#include "Shell.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cstring>

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
            handleRedirection(command);
            handlePipe(command);
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

void Shell::handleRedirection(const std::string& command) {
    size_t inputRedirectPos = command.find('<');
    size_t outputRedirectPos = command.find('>');

    if (inputRedirectPos != std::string::npos) {
        std::string inputFile = command.substr(inputRedirectPos + 1);
        inputFile = inputFile.substr(inputFile.find_first_not_of(" \t\n\r\f\v"));
        inputFile = inputFile.substr(0, inputFile.find_last_not_of(" \t\n\r\f\v") + 1);
        int fd = open(inputFile.c_str(), O_RDONLY);
        if (fd == -1) {
            std::cerr << "Failed to open input file: " << inputFile << std::endl;
            exit(EXIT_FAILURE);
        }
        if (dup2(fd, STDIN_FILENO) == -1) {
            std::cerr << "Failed to redirect input." << std::endl;
            exit(EXIT_FAILURE);
        }
        if (close(fd) == -1) {
            std::cerr << "Failed to close input file descriptor." << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    if (outputRedirectPos != std::string::npos) {
        std::string outputFile = command.substr(outputRedirectPos + 1);
        outputFile = outputFile.substr(outputFile.find_first_not_of(" \t\n\r\f\v"));
        outputFile = outputFile.substr(0, outputFile.find_last_not_of(" \t\n\r\f\v") + 1);
        int fd = open(outputFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (fd == -1) {
            std::cerr << "Failed to open output file: " << outputFile << std::endl;
            exit(EXIT_FAILURE);
        }
        if (dup2(fd, STDOUT_FILENO) == -1) {
            std::cerr << "Failed to redirect output." << std::endl;
            exit(EXIT_FAILURE);
        }
        if (close(fd) == -1) {
            std::cerr << "Failed to close output file descriptor." << std::endl;
            exit(EXIT_FAILURE);
        }
    }
}

void Shell::handlePipe(const std::string& command) {
    size_t pipePos = command.find('|');
    if (pipePos != std::string::npos) {
        std::string firstCommand = command.substr(0, pipePos);
        std::string secondCommand = command.substr(pipePos + 1);

        // Create a pipe
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            std::cerr << "Failed to create pipe." << std::endl;
            exit(EXIT_FAILURE);
        }

        pid_t pid1 = fork();
        if (pid1 < 0) {
            std::cerr << "Failed to create child process." << std::endl;
            exit(EXIT_FAILURE);
        }
        else if (pid1 == 0) {
            // Child process - first command
            close(pipefd[0]);  // Close unused read end

            // Redirect stdout to the write end of the pipe
            if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
                std::cerr << "Failed to redirect stdout." << std::endl;
                exit(EXIT_FAILURE);
            }

            close(pipefd[1]);  // Close write end

            executeChildCommand(firstCommand);
        }
        else {
            pid_t pid2 = fork();
            if (pid2 < 0) {
                std::cerr << "Failed to create child process." << std::endl;
                exit(EXIT_FAILURE);
            }
            else if (pid2 == 0) {
                // Child process - second command
                close(pipefd[1]);  // Close unused write end

                // Redirect stdin to the read end of the pipe
                if (dup2(pipefd[0], STDIN_FILENO) == -1) {
                    std::cerr << "Failed to redirect stdin." << std::endl;
                    exit(EXIT_FAILURE);
                }

                close(pipefd[0]);  // Close read end

                executeChildCommand(secondCommand);
            }
            else {
                // Parent process
                close(pipefd[0]);  // Close unused read end
                close(pipefd[1]);  // Close unused write end

                int status;
                waitpid(pid1, &status, 0);
                waitpid(pid2, &status, 0);
            }
        }
    }
}