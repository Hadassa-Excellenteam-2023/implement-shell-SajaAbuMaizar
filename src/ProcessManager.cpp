#include "ProcessManager.h"
#include <iostream>

void ProcessManager::addBackgroundProcess(pid_t pid) {
    backgroundProcesses.push_back(pid);
}

void ProcessManager::showJobs() {
    std::cout << "Running background processes:" << std::endl;
    for (auto it = backgroundProcesses.begin(); it != backgroundProcesses.end();) {
        pid_t pid = *it;
        int childStatus;
        pid_t result = waitpid(pid, &childStatus, WNOHANG | WUNTRACED | WCONTINUED);

        if (result == -1) {
            std::cout << "PID: " << pid << ", Status: Not Found!" << std::endl;
            it = backgroundProcesses.erase(it);
        }
        else if (result == 0) {
            // Process still running
            std::cout << "PID: " << pid << ", Status: Running..." << std::endl;
            ++it;
        }
        else {
            // Process finished
            if (WIFEXITED(childStatus)) {
                std::cout << "PID: " << pid << ", Status: Exited." << std::endl;
            }
            else if (WIFSIGNALED(childStatus)) {
                std::cout << "PID: " << pid << ", Status: Terminated" << std::endl;
            }
            else if (WIFSTOPPED(childStatus)) {
                std::cout << "PID: " << pid << ", Status: Stopped" << std::endl;
            }
            else if (WIFCONTINUED(childStatus)) {
                std::cout << "PID: " << pid << ", Status: Continued" << std::endl;
            }
            else {
                std::cout << "PID: " << pid << ", Status: Unknown" << std::endl;
            }

            it = backgroundProcesses.erase(it);
        }
    }
}