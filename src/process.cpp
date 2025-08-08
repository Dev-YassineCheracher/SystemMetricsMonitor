#include "../include/Components.hpp"
#include "include/Process.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iterator>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <mutex>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>

using namespace std;
Process::Process(int pid, double cpuUsage) : pid(pid), cpuUsage(cpuUsage) {}


    // Function to populate process information from /proc/[PID]
    bool Process::populateInfo() {

        memoryUsage = memoryUsagePerProcess(pid);
        std::ifstream statusFile("/proc/" + std::to_string(pid) + "/status");
        if (!statusFile) {
            return false;
        }

        std::string line;
        while (getline(statusFile, line)) {
            std::vector<std::string> tokens = split(line, ':');
            if (tokens[0] == "PPid") {
                ppid = std::stoi(tokens[1]);
            }
        }

        std::ifstream cmdlineFile("/proc/" + std::to_string(pid) + "/cmdline");
        if (!cmdlineFile) {
            return false;
        }

        getline(cmdlineFile, command, '\0'); // Read the entire command line

        // Check if the command is empty and return false
        if (command.empty()) {
            return false;
        }

        return true;
    }

    void Process::addChild(Process* child) {
        children.push_back(child);
    }
    double Process::memoryUsagePerProcess(int pid) {
        // get resident memory
        std::ifstream statmFile("/proc/" + std::to_string(pid) + "/statm");
        if (!statmFile) {
            return false;
        }
        int residentMemory = 0;
        std::string line;
        if(std::getline(statmFile, line)) {
            std::istringstream iss(line);
            for (int i = 0; i < 2; ++i) {
                iss >> residentMemory;
            }

            if (iss) {
                // cool, the residentMemory is read.
            } else {
                std::cerr << "Failed to read the resident Memory." << std::endl;
                exit(1);
            }
        }

        statmFile.close();


        // open /proc/meminfo
        std::ifstream meminfoFile("/proc/meminfo");
        if (!meminfoFile) {
            exit(1);
        }
        int totalMemory = 0;
        // take the first line
        while(std::getline(meminfoFile, line)) {
            if (line.find("MemTotal") != std::string::npos) {
                std::string memTotal = line.substr(line.find(":") + 1);
                memTotal.erase(std::remove_if(memTotal.begin(), memTotal.end(), ::isspace), memTotal.end()); // Remove spaces
                memTotal = memTotal.substr(0, memTotal.size() - 2); // Remove "kB"
                totalMemory = std::stoi(memTotal);
                break;
            }
        }
        meminfoFile.close();

        // compute the memory usage percentage
        residentMemory *= getpagesize();
        double memory_usage = (double)residentMemory / totalMemory;
        return memory_usage/10;
    }



    std::vector<std::string> Process::split(const std::string& s, char delimiter) {
        std::vector<std::string> tokens;
        std::istringstream tokenStream(s);
        std::string token;
        while (getline(tokenStream, token, delimiter)) {
            tokens.push_back(token);
        }
        return tokens;
    }


    // --------------------------------------------------


double cpuUsagePerProcess(int pid) {
    std::ifstream statFile("/proc/" + std::to_string(pid) + "/stat");
    if (!statFile.is_open()) {
        std::cerr << "Failed to open /proc/" << pid << "/stat" << std::endl;
        return -1.0;
    }

    std::string line;
    getline(statFile, line);
    statFile.close();

    std::istringstream iss(line);
    std::string dummy; // To store the process name
    long utime, stime; // Values from /proc/<pid>/stat
    for (int i = 1; i <= 13; ++i) {
        iss >> dummy;
    }
    iss >> utime >> stime;

    std::this_thread::sleep_for(std::chrono::seconds(1));

    statFile.open("/proc/" + std::to_string(pid) + "/stat");
    if (!statFile.is_open()) {
        std::cerr << "Failed to open /proc/" << pid << "/stat" << std::endl;
        return -1.0;
    }

    getline(statFile, line);
    statFile.close();

    iss.clear();
    iss.str(line);
    for (int i = 1; i <= 13; ++i) {
        iss >> dummy;
    }
    long utime_end, stime_end;
    iss >> utime_end >> stime_end;

    long utime_diff = utime_end - utime;
    long stime_diff = stime_end - stime;
    long total_time_diff = utime_diff + stime_diff;

    std::ifstream cpuinfoFile("/proc/cpuinfo");
    double hz = 100.0; // Default value if unable to determine Hz
    std::string cpuinfoLine;
    while (getline(cpuinfoFile, cpuinfoLine)) {
        if (cpuinfoLine.find("cpu MHz") != std::string::npos) {
            std::istringstream cpuinfoStream(cpuinfoLine);
            std::string discard;
            double cpuMhz;
            cpuinfoStream >> discard >> discard >> cpuMhz;
            hz = cpuMhz * 1000000.0;
            break;
        }
    }

    double cpu_usage_percent = (total_time_diff / hz) * 100.0;
    return cpu_usage_percent;
}
std::mutex cpuUsageMutex;
std::unordered_map<int, double> cpuUsagePerProcesses(std::vector<int> pids) {
    std::unordered_map<int, double> cpuUsageMap;

    for (int pid : pids) {
        // Perform the CPU usage computation for each process concurrently
        std::thread cpuUsageThread([pid, &cpuUsageMap]() {
            double cpu_usage = cpuUsagePerProcess(pid);

            // Lock the unordered_map to prevent race conditions
            std::lock_guard<std::mutex> lock(cpuUsageMutex);

            // Store the CPU usage for the process in the map
            cpuUsageMap[pid] = cpu_usage;
        });

        cpuUsageThread.detach();
    }

    // Wait for a short while to allow CPU usage computations to complete
    std::this_thread::sleep_for(std::chrono::seconds(2));

    return cpuUsageMap;
}
//Function to print a tree
void printTree(const Process* process, string indent, int depth) {


    cout << process->pid << "\t" << process->ppid << " \t" << process->cpuUsage << "\t" << process->memoryUsage << "\t" << process->command.substr(process->command.find_first_not_of(' ')) << endl;

    for (const Process* child : process->children) {
        printTree(child, indent, depth + 1);
    }
}

int getMaxCommandLength() {
    int maxCommandLen = 0;
    for (int pid = 1; pid <= 32767; ++pid) {
        std::ifstream cmdlineFile("/proc/" + std::to_string(pid) + "/cmdline");
        if (cmdlineFile) {
            std::string command;
            getline(cmdlineFile, command, '\0');
            maxCommandLen = max(maxCommandLen, static_cast<int>(command.length()));
        }
    }
    return maxCommandLen;
}
std::vector<int> currentProcesses() {
    // returns a vector of pids
    std::vector<int> currentProcesses;
    for(int pid = 1; pid <= 32767; ++pid) {
        std::ifstream cmdlineFile("/proc/" + std::to_string(pid) + "/cmdline");
        if (cmdlineFile) {
            std::string command;
            getline(cmdlineFile, command, '\0');
            currentProcesses.push_back(pid);
        }
    }
    return currentProcesses;
}

int main() {
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<Process*> processes;
    std::unordered_map<int, Process*> processMap;
    // get cpuUsages for all processes

    std::unordered_map<int, double> cpuUsageMap = cpuUsagePerProcesses(currentProcesses());
    int maxCommandLen = getMaxCommandLength();
    // Read process information from /proc
    for (int pid = 1; pid <= 32767; ++pid) {
        Process* process = new Process(pid, cpuUsageMap[pid]);
        if (process->populateInfo()) {
            processes.push_back(process);
            processMap[pid] = process;
        } else {
            delete process;
        }
    }

    // Link child process to their parent processes

    for (Process* process : processes) {
        if(processMap.count(process->ppid)) {
            processMap[process->ppid]->addChild(process);
        }
    }
    cout << "PID" << "\t" << "PPID" << "\t" << "CPU%" << "\t" << "Mem%" << "\t" << "Command" << endl;
    for (Process* process : processes) {
        printTree(process, "  ", 0);
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    // write to log.txt
    std::ofstream logFile("log.txt", std::ios_base::app);
    logFile << "Time taken: " << elapsed.count() << " seconds" << std::endl;
    logFile.close();

    return 0;
}
