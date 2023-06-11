#ifndef PROCESSMANAGER_H
#define PROCESSMANAGER_H

#include <vector>
#include <sys/wait.h>

class ProcessManager {
public:
    void addBackgroundProcess(pid_t pid);
    void showJobs();

private:
    std::vector<pid_t> backgroundProcesses;
};

#endif  // PROCESSMANAGER_H