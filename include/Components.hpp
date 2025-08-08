#ifndef SMON_COMPONENTS_HPP
#define SMON_COMPONENTS_HPP

#include <ftxui/component/component.hpp>
#include <string>
#include "DiskMonitoring.hpp" // For DiskSpaceUsage struct

ftxui::Component Title(std::wstring title, std::wstring content);
ftxui::Component CpuMonitor(const std::wstring& title, int use_switch, double cpuUsage);
ftxui::Component MemoryMonitor(const std::wstring& title, int usage_type, double precomputed_value);
ftxui::Component DiskMonitor(const std::wstring& title, int usage_type, double value, std::wstring diskName);

#endif //SMON_COMPONENTS_HPP
