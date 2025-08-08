#include "../include/Components.hpp"
#include "../include/DiskMonitoring.hpp"
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <string>
#include <ftxui/component/captured_mouse.hpp>
#include <iomanip>
#include <sys/statvfs.h>
#include <cmath>
#include "ftxui/screen/color.hpp"
#include <sys/ioctl.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>
#include <map>
#include <dirent.h>
#include <cstring>

#include "../include/TitleComponent.hpp"
#include "../include/CpuMonitoring.hpp"
#include "../include/MemoryMonitoring.hpp"
#include "../include/Overview.hpp"
#include "../include/ProcessActivityMonitoring.hpp"

using namespace ftxui;

DiskSpaceUsage calculateDiskSpaceUsage() {
    struct statvfs stat;
    if (statvfs("/", &stat) != 0) {
        return {0.0, {}, "Error"};
    }
    unsigned long totalSize = stat.f_blocks * stat.f_frsize;
    unsigned long usedSize = (stat.f_blocks - stat.f_bfree) * stat.f_frsize;
    
    if (totalSize == 0) return {0.0, {}, "B"};

    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = static_cast<double>(totalSize);
    while (size >= 1024.0 && unitIndex < 4) {
        size /= 1024.0;
        ++unitIndex;
    }

    DiskSpaceUsage usage;
    usage.percentageUsed = static_cast<double>(usedSize) / static_cast<double>(totalSize) * 100.0;
    usage.sizes = {static_cast<double>(totalSize) / pow(1024.0, unitIndex),
                   static_cast<double>(usedSize) / pow(1024.0, unitIndex)};
    usage.unit = units[unitIndex];
    return usage;
}

Component DiskMonitor(const std::wstring& title, int usage_type, double value, std::wstring diskName) {
    return Renderer([=] {
        auto my_border = borderStyled(ROUNDED, Color::RGB(238, 238, 238));
        auto text_color = color(Color::RGB(255, 211, 105));
        auto background_color = bgcolor(Color::RGB(34, 40, 49));
        struct winsize size{};
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
        int terminal_width = (int) (size.ws_col);

        if (usage_type == 2) {
            return text(L"Current Devices:") | vcenter | hcenter | flex | text_color | my_border | background_color;
        } else if (usage_type == 3) {
            double fullMemoryGB = value / (1024.0 * 1024.0 * 1024.0);
            std::wstringstream fullMemoryStream;
            fullMemoryStream << std::fixed << std::setprecision(2) << fullMemoryGB;
            auto deviceInfo = diskName + L":   " + fullMemoryStream.str() + L" GB";
            return text(deviceInfo) | vcenter | hcenter | flex | text_color | my_border | background_color;
        } else if(usage_type == 4) {
            return text(L"disk    size") | vcenter | hcenter | flex | text_color | my_border | background_color;
        } else {
            auto space = std::wstring(1, L' ');
            double diskUsage = value;
            int available_space = terminal_width - 33;
            if (terminal_width <= 38) {
                std::wstringstream diskUsageStream;
                diskUsageStream << std::fixed << std::setprecision(2) << diskUsage;
                std::wstring diskUsageString = diskUsageStream.str() + L"%";
                return text((title + space) + L"[" + L"|" + L"] " + diskUsageString) | hcenter | vcenter | flex | text_color | my_border | background_color;
            }
            int numBars = static_cast<int>((diskUsage * available_space) / 100);
            std::wstring bars(numBars, L'|');
            std::wstring spaces(available_space - numBars, L' ');
            std::wstringstream diskUsageStream;
            diskUsageStream << std::fixed << std::setprecision(2) << diskUsage;
            std::wstring diskUsageString = diskUsageStream.str() + L"%";
            return text((title + space) + L"[" + bars + spaces + L"] " + diskUsageString) | hcenter | vcenter | flex | text_color | my_border | background_color;
        }
    });
}

Component make_device_components() {
    auto device_container = Container::Vertical({});
    device_container->Add(DiskMonitor(L"", 4, 0.0, L""));

    // This is placeholder data since reading /sys/block requires root and might not work in all Docker setups.
    // In a real scenario, you'd parse /sys/block or use a library.
    std::map<std::wstring, double> devices;
    devices[L"sda1"] = 500.0 * 1024 * 1024 * 1024; // 500 GB
    devices[L"sdb1"] = 1000.0 * 1024 * 1024 * 1024; // 1 TB

    for (const auto &entry: devices) {
        device_container->Add(DiskMonitor(L"", 3, entry.second, entry.first));
    }
    return device_container;
}

void showDiskUsage() {
    std::wstring main_title = L"System Metrics Monitor: Disk Usage";
    std::wstring instructions = L"F1 Overview F2 Cpu F3 Memory F4 Disk F5 ProcessInfo F6 Quit";
    auto screen = ScreenInteractive::Fullscreen();

    DiskSpaceUsage usage = calculateDiskSpaceUsage();

    auto title_component = Title(main_title, L"");
    auto disk_component = DiskMonitor(L"Total Usage: ", 1, usage.percentageUsed, L"");
    auto instructions_component = Title(instructions, L"");
    auto disk_devices_title = DiskMonitor(L"", 2, 0.0, L"");
    auto device_components = make_device_components();

    auto handleCustomEvent = [&screen]() {
        screen.PostEvent(Event::Custom);
    };
    std::thread timerThread([handleCustomEvent]() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        handleCustomEvent();
    });
    
    auto middle_container = Container::Vertical({
        disk_component,
        Container::Horizontal({
            Container::Vertical({ disk_devices_title }) | flex,
            Container::Vertical({ device_components }) | flex,
        }) | flex
    });

    auto main_container = Container::Vertical({
        Title(main_title, L""),
        middle_container,
        Title(instructions, L"")
    });

    main_container |= CatchEvent([&](Event event) {
        if(event == Event::F1) { screen.ExitLoopClosure()(); showOverview(); }
        else if(event == Event::Custom) { screen.ExitLoopClosure()(); showDiskUsage(); }
        else if(event == Event::F2) { screen.ExitLoopClosure()(); showCpuUsage(); }
        else if(event == Event::F3) { screen.ExitLoopClosure()(); showMemoryUsage(); }
        else if (event == Event::F5) { screen.ExitLoopClosure()(); showProcessInfo(1, getProcesses()); }
        else if (event == Event::F6) { screen.ExitLoopClosure()(); }
        return true;
    });
    screen.Loop(main_container);
}
