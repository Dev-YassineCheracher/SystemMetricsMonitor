#include "../include/Components.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <string>
#include <ftxui/component/captured_mouse.hpp>
#include <iomanip>
#include <cmath>
#include "ftxui/screen/color.hpp"
#include <sys/ioctl.h>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <thread>
#include <algorithm>

#include "../include/ProcessActivityMonitoring.hpp"
#include "../include/Overview.hpp"
#include "../include/CpuMonitoring.hpp"
#include "../include/DiskMonitoring.hpp"
#include "../include/MemoryMonitoring.hpp"

using namespace ftxui;

// Helper function to right-align text
std::wstring swipeRight(const std::wstring &original, int size) {
    if (size <= (int)original.size()) {
        return original;
    }
    std::wstring result(size, L' ');
    for (size_t i = 0; i < original.size(); ++i) {
        result[size - original.size() + i] = original[i];
    }
    return result;
}

Component ProcessMonitor(Process process_data, bool is_title) {
    return Renderer([=] {
        auto my_border = borderStyled(ROUNDED, Color::RGB(238, 238, 238));
        auto text_color = color(Color::RGB(255, 211, 105));
        auto background_color = bgcolor(Color::RGB(34, 40, 49));
        auto space = std::wstring(2, L' ');

        if (is_title) {
            std::wstring pid = swipeRight(L"PID", 5);
            std::wstring ppid = swipeRight(L"PPID", 5);
            std::wstring cpu_usage = swipeRight(L"CPU%", 5);
            std::wstring memory_usage = swipeRight(L"MEM%", 5);
            std::wstring command = L"COMMAND";
            return text(pid + space + ppid + space + cpu_usage + space + memory_usage + space + command) | vcenter | flex | text_color | my_border | background_color;
        }

        std::wstring pid = swipeRight(std::to_wstring(process_data.pid), 5);
        std::wstring ppid = swipeRight(std::to_wstring(process_data.ppid), 5);

        std::wstringstream cpuUsageStream;
        cpuUsageStream << std::fixed << std::setprecision(2) << process_data.cpu_usage;
        std::wstring cpu_usage = swipeRight(cpuUsageStream.str(), 5);

        std::wstringstream memoryUsageStream;
        memoryUsageStream << std::fixed << std::setprecision(2) << process_data.memory_usage;
        std::wstring memory_usage = swipeRight(memoryUsageStream.str(), 5);

        std::wstring command = std::wstring(process_data.command.begin(), process_data.command.end());

        return text(pid + space + ppid + space + cpu_usage + space + memory_usage + space + command) | vcenter | flex | text_color | my_border | background_color;
    });
}


std::vector<Process> getProcesses() {
    std::vector<Process> processes;
    for (int i = 0; i < 50; i++) {
        Process p;
        p.user = "user";
        p.pid = 12 * i ^ 2 + 1000;
        p.ppid = i + 1000;
        p.cpu_usage = (double)i * 0.75;
        p.memory_usage = (double)i * 0.5;
        p.command = "/usr/bin/some_application --with-long-arguments-that-take-space";
        processes.push_back(p);
    }
    return processes;
}

void showProcessInfo(int start, std::vector<Process> processes) {
    std::wstring main_title = L"System Metrics Monitor: Process Usage Information";
    std::wstring instructions = L"F1 Overview F2 Cpu F3 Memory F4 Disk F5 ProcessInfo F6 Quit";
    auto screen = ScreenInteractive::Fullscreen();

    // Use the new function-based components
    auto title_component = Title(main_title, L"");
    auto processTableTitle = ProcessMonitor({}, true); // Empty process, is_title = true
    auto instructions_component = Title(instructions, L"");
    auto process_command = Title(L"F9 Sort", L"");

    struct winsize size{};
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
    int terminal_height = (int) (size.ws_row);
    int max_to_show = (terminal_height > 6) ? terminal_height - 6 : 1;
    
    auto process_container = Container::Vertical({processTableTitle});
    for (int i = start - 1; i < start + max_to_show - 1 && i < (int)processes.size(); i++) {
        process_container->Add(ProcessMonitor(processes[i], false));
    }

    auto handleCustomEvent = [&screen]() { screen.PostEvent(Event::Custom); };
    std::thread timerThread([handleCustomEvent]() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        handleCustomEvent();
    });

    auto upper_container = Container::Horizontal({title_component, process_command});
    auto lower_container = Container::Vertical({instructions_component});

    auto main_container = Container::Vertical({
        upper_container,
        process_container | flex,
        lower_container
    });

    main_container |= CatchEvent([&](Event event) {
        if (event == Event::ArrowDown) {
            if (start < (int)processes.size() - max_to_show + 1) {
                screen.ExitLoopClosure()();
                showProcessInfo(start + 1, processes);
            }
        } else if (event == Event::ArrowUp) {
            if (start > 1) {
                screen.ExitLoopClosure()();
                showProcessInfo(start - 1, processes);
            }
        } else if (event == Event::F1) { screen.ExitLoopClosure()(); showOverview(); }
        else if(event == Event::Custom) { screen.ExitLoopClosure()(); showProcessInfo(start, getProcesses()); }
        else if (event == Event::F2) { screen.ExitLoopClosure()(); showCpuUsage(); }
        else if (event == Event::F3) { screen.ExitLoopClosure()(); showMemoryUsage(); }
        else if (event == Event::F4) { screen.ExitLoopClosure()(); showDiskUsage(); }
        else if (event == Event::F6) {
            screen.ExitLoopClosure()();
            system("reset");
            std::cout << "\x1B[2J\x1B[H";
            exit(0);
        } else if (event == Event::F9) {
            std::sort(processes.begin(), processes.end(), [](const Process& a, const Process& b) {
                return a.cpu_usage > b.cpu_usage;
            });
            screen.ExitLoopClosure()();
            showProcessInfo(1, processes); // Reset to start after sort
        }
        return true;
    });
    screen.Loop(main_container);
}
