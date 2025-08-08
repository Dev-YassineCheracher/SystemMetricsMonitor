#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <thread>
#include <vector>
#include <string>
#include <map>
#include "../include/Overview.hpp"
#include "../include/Components.hpp"
#include "../include/CpuMonitoring.hpp"
#include "../include/MemoryMonitoring.hpp"
#include "../include/DiskMonitoring.hpp"
#include "../include/ProcessActivityMonitoring.hpp"

using namespace ftxui;

void showCpuUsage();
void showMemoryUsage();
void showDiskUsage();
void showProcessInfo(int, std::vector<Process>);
std::vector<Process> getProcesses();

void showOverview()
{
    // Get Data
    std::map<std::string, double> cpu_data = get_cpu_usage();
    double total_cpu_usage = cpu_data.count("total") ? cpu_data["total"] : 0.0;
    double memory_usage = monitorMemory();
    DiskSpaceUsage disk_usage = calculateDiskSpaceUsage();

    auto screen = ScreenInteractive::Fullscreen();

    auto title_component = Title(L"System Metrics Monitor: Overview", L"");
    auto cpu_component = CpuMonitor(L"CPU Usage:", 1, total_cpu_usage);
    auto memory_component = MemoryMonitor(L"Memory Usage:", 1, memory_usage);
    auto disk_component = DiskMonitor(L"Disk Usage:", 1, disk_usage.percentageUsed, L"");
    auto instructions_component = Title(L"F1 Overview F2 Cpu F3 Memory F4 Disk F5 ProcessInfo F6 Quit", L"");

    auto handleCustomEvent = [&screen]()
    {
        screen.PostEvent(Event::Custom);
    };
    std::thread timerThread([handleCustomEvent]()
                            {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        handleCustomEvent(); });

    auto main_container = Container::Vertical({title_component,
                                               cpu_component,
                                               memory_component,
                                               disk_component,
                                               instructions_component}) |
                          flex;

    main_container |= CatchEvent([&](Event event)
                                 {
        if (event == Event::F1) { screen.ExitLoopClosure()(); showOverview(); }
        else if (event == Event::F2) { screen.ExitLoopClosure()(); showCpuUsage(); }
        else if (event == Event::Custom) { screen.ExitLoopClosure()(); showOverview(); }
        else if (event == Event::F3) { screen.ExitLoopClosure()(); showMemoryUsage(); }
        else if (event == Event::F4) { screen.ExitLoopClosure()(); showDiskUsage(); }
        else if (event == Event::F5) { screen.ExitLoopClosure()(); showProcessInfo(1, getProcesses()); }
        else if (event == Event::F6) {
            timerThread.join();
            screen.ExitLoopClosure()();
            system("reset");
            exit(0);
        }
        return true; });

    screen.Loop(main_container);
}
