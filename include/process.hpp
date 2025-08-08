#ifndef SMON_PROCESS_HPP
#define SMON_PROCESS_HPP
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>

class Process {
    private:

    static std::vector<std::string> split(const std::string& s, char delimiter);
    public:
        int pid;
        int ppid;
        double cpuUsage; // CPU usage in percentage
        double memoryUsage; // Memory usage in percentage
        std::string command;
        std::vector<Process*> children;
        bool populateInfo();
        void addChild(Process* child);
        double memoryUsagePerProcess(int pid);
    Process(int pid, double cpuUsage);
};
    void printTree(const Process* process, std::string indent, int depth);
    double cpuUsagePerProcess(int pid);
    std::unordered_map<int, double> cpuUsagePerProcesses(std::vector<int> pids);
#endif //SMON_PROCESS_HPP
