#ifndef SMON_CPUMONITORING_HPP
#define SMON_CPUMONITORING_HPP
#include <string>
#include <vector>
#include <map>

std::map<std::string, double> get_cpu_usage();
void showCpuUsage();

#endif //SMON_CPUMONITORING_HPP
