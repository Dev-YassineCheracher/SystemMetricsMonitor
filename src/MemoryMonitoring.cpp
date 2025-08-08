#include "../include/Components.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp> // Add this
#include <ftxui/dom/elements.hpp>
#include <string>
#include <utility>
#include <sys/sysinfo.h>
#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/mouse.hpp>
#include <iomanip>
#include <sys/statvfs.h>
#include <cmath>
#include "ftxui/screen/color.hpp"
#include <sys/ioctl.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <random>
#include <thread> // Add this
#include "../include/TitleComponent.hpp"
#include "../include/CpuMonitoring.hpp"
#include "../include/ProcessActivityMonitoring.hpp"
#include "../include/MemoryMonitoring.hpp"
#include "../include/DiskMonitoring.hpp"
#include "../include/Overview.hpp"

using namespace ftxui;

Component MemoryMonitor(const std::wstring& title, int usage_type, double precomputed_value) {
    return Renderer([=] {
        struct winsize size{};
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
        int terminal_width = (int)(size.ws_col);

        auto my_border = borderStyled(ROUNDED, Color::RGB(238, 238, 238));
        auto text_color = color(Color::RGB(255, 211, 105));
        auto background_color = bgcolor(Color::RGB(34, 40, 49));
        auto space = std::wstring(1, L' ');

        double memoryUsage = 0.0;
        if(usage_type == 1 || usage_type == 3) {
            memoryUsage = precomputed_value;
        } else if(usage_type == 2) {
            memoryUsage = 100.0 - precomputed_value;
        } else if (usage_type == 4) {
            memoryUsage = get_swap_usage();
        }

        int available_space = terminal_width - 33;
        if(terminal_width <= 38) {
            std::wstringstream memoryUsageStream;
            memoryUsageStream << std::fixed << std::setprecision(2) << memoryUsage;
            std::wstring memoryUsageString = memoryUsageStream.str() + L"%";
            return text((title + space) + L"[" + L"|" + L"] " + memoryUsageString) | hcenter | vcenter | flex | text_color | my_border | background_color;
        }

        int numBars = static_cast<int>((memoryUsage * available_space) / 100);
        std::wstring bars(numBars, L'|');
        std::wstring spaces(available_space - numBars, L' ');
        std::wstringstream memoryUsageStream;
        memoryUsageStream << std::fixed << std::setprecision(2) << memoryUsage;
        std::wstring memoryUsageString = memoryUsageStream.str() + L"%";
        return text((title + space) + L"[" + bars + spaces + L"] " + memoryUsageString) | hcenter | vcenter | flex | text_color | my_border | background_color;
    });
}


void showMemoryUsage() {
    double used_mem_percent = monitorMemory();
    
    std::wstring main_title = L"System Metrics Monitor: Memory Usage";
    std::wstring free_memory_title = L"Free Memory:";
    std::wstring used_memory_title = L"Used Memory:";
    std::wstring instructions = L"F1 Overview F2 Cpu F3 Memory F4 Disk F5 ProcessInfo F6 Quit";

    auto screen = ScreenInteractive::Fullscreen();

    // Use the new function-based components
    auto title_component = Title(main_title, L"");
    auto FreeMemory_component = MemoryMonitor(free_memory_title, 2, used_mem_percent);
    auto UsedMemory_component = MemoryMonitor(used_memory_title, 3, used_mem_percent);
    auto swapMemory_component = MemoryMonitor(L"Swap Memory", 4, 0.0); // 0.0 is a placeholder
    auto instructions_component = Title(instructions, L"");

    auto handleCustomEvent = [&screen]() {
        screen.PostEvent(Event::Custom);
    };

    std::thread timerThread([handleCustomEvent]() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        handleCustomEvent();
    });

    auto upper_container = Container::Vertical({ title_component });

    auto middle_container = Container::Vertical({
        FreeMemory_component,
        UsedMemory_component,
        swapMemory_component
    }) | flex;

    auto lower_container = Container::Vertical({ instructions_component });

    auto main_container = Container::Vertical({
        upper_container,
        middle_container,
        lower_container
    });

    main_container |= CatchEvent([&](Event event) {
        if(event == Event::F1) { screen.ExitLoopClosure()(); showOverview(); }
        else if(event == Event::Custom) { screen.ExitLoopClosure()(); showMemoryUsage(); }
        else if(event == Event::F2) { screen.ExitLoopClosure()(); showCpuUsage(); }
        else if (event == Event::F4) { screen.ExitLoopClosure()(); showDiskUsage(); }
        else if (event == Event::F5) { screen.ExitLoopClosure()(); showProcessInfo(1, getProcesses()); }
        else if(event == Event::F6) { screen.ExitLoopClosure()(); }
        return true;
    });

    screen.Loop(main_container);
}

double monitorMemory() {
    std::ifstream file("/proc/meminfo");
    if (!file) {
        std::cerr << "Failed to open /proc/meminfo" << std::endl;
        return 0.0;
    }

    std::string line;
    long totalMemorySize = 0, freeMemorySize = 0, buffersSize = 0, cacheSize = 0;

    while (std::getline(file, line)) {
        if (line.rfind("MemTotal:", 0) == 0) {
            totalMemorySize = std::stol(line.substr(9));
        } else if (line.rfind("MemFree:", 0) == 0) {
            freeMemorySize = std::stol(line.substr(8));
        } else if (line.rfind("Buffers:", 0) == 0) {
            buffersSize = std::stol(line.substr(8));
        } else if (line.rfind("Cached:", 0) == 0) {
            cacheSize = std::stol(line.substr(7));
        }
    }
    file.close();

    if (totalMemorySize == 0) return 0.0;

    long usedMemorySize = totalMemorySize - freeMemorySize - buffersSize - cacheSize;
    return (double)usedMemorySize / totalMemorySize * 100.0;
}


double get_swap_usage() {
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        if (info.totalswap == 0) return 0.0;
        unsigned long long used_swap = info.totalswap - info.freeswap;
        return (double)used_swap / info.totalswap * 100.0;
    }
    return 0.0;
}
