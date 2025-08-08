#ifndef SMON_PROCESSACTIVITYMONITORING_HPP
#define SMON_PROCESSACTIVITYMONITORING_HPP

#include <ftxui/component/component.hpp>
#include <string>
#include <vector>

struct Process {
    int pid;
    int ppid;
    std::string user;
    std::string command;
    double cpu_usage;
    double memory_usage;
    double read_speed;
    double write_speed;
};

// Declare the helper functions
void showProcessInfo(int start, std::vector<Process> processes);
std::vector<Process> getProcesses();

#endif //SMON_PROCESSACTIVITYMONITORING_HPP
