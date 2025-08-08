#ifndef SMON_DISKMONITORING_HPP
#define SMON_DISKMONITORING_HPP
#include <string>
#include <vector>

struct DiskSpaceUsage {
    double percentageUsed;
    std::vector<double> sizes;
    std::string unit;
};
DiskSpaceUsage calculateDiskSpaceUsage();
void showDiskUsage();

#endif //SMON_DISKMONITORING_HPP
