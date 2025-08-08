#include "../include/Components.hpp"
#include "../include/CpuMonitoring.hpp"
#include "../include/DiskMonitoring.hpp"
#include "../include/MemoryMonitoring.hpp"
#include "../include/Overview.hpp"
#include "../include/ProcessActivityMonitoring.hpp"
#include "../include/TitleComponent.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>
#include <vector>
#include <ftxui/component/screen_interactive.hpp>
#include <thread>


using namespace ftxui;

int CpuUseCounter = 0;

Component CpuMonitor(const std::wstring& title, int use_switch, double cpuUsage) {
  return Renderer([=] {
    struct winsize size {};
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
    int terminal_width = (int)(size.ws_col);

    auto my_border = borderStyled(ROUNDED, Color::RGB(238, 238, 238));
    auto text_color = color(Color::RGB(255, 211, 105));
    auto background_color = bgcolor(Color::RGB(34, 40, 49));
    auto space = std::wstring(1, L' ');

    if (use_switch == 2) {
      terminal_width /= 2;
    }

    int available_space = terminal_width - 30;

    if (terminal_width <= 35) {
      std::wstringstream cpuUsageStream;
      cpuUsageStream << std::fixed << std::setprecision(2) << cpuUsage;
      std::wstring cpuUsageString = cpuUsageStream.str() + L"%";
      return text((title + space) + L"[" + L"|" + L"] " + cpuUsageString) |
             hcenter | vcenter | flex | text_color | my_border |
             background_color;
    }

    int numBars = static_cast<int>((cpuUsage * available_space) / 100);
    std::wstring bars(numBars, L'|');
    std::wstring spaces(available_space - numBars, L' ');
    std::wstringstream cpuUsageStream;
    cpuUsageStream << std::fixed << std::setprecision(2) << cpuUsage;
    std::wstring cpuUsageString = cpuUsageStream.str() + L"%";
    return text((title + space) + L"[" + bars + spaces + L"] " +
                cpuUsageString) |
           hcenter | vcenter | flex | text_color | my_border |
           background_color;
  });
}

// Function to get CPU times from /proc/stat
std::vector<long> get_cpu_times() {
  std::ifstream proc_stat("/proc/stat");
  std::string cpu_name;
  long user, nice, system, idle, iowait, irq, softirq, steal, guest,
      guest_nice;
  std::vector<long> cpu_times;
  while (proc_stat >> cpu_name >> user >> nice >> system >> idle >> iowait >>
         irq >> softirq >> steal >> guest >> guest_nice) {
    if (cpu_name.substr(0, 3) == "cpu") {
      cpu_times.push_back(user + nice + system + idle + iowait + irq +
                          softirq + steal);
    }
  }
  return cpu_times;
}

// Function to get idle times from /proc/stat
std::vector<long> get_idle_times() {
  std::ifstream proc_stat("/proc/stat");
  std::string cpu_name;
  long user, nice, system, idle, iowait, irq, softirq, steal, guest,
      guest_nice;
  std::vector<long> idle_times;
  while (proc_stat >> cpu_name >> user >> nice >> system >> idle >> iowait >>
         irq >> softirq >> steal >> guest >> guest_nice) {
    if (cpu_name.substr(0, 3) == "cpu") {
      idle_times.push_back(idle + iowait);
    }
  }
  return idle_times;
}

// Function to calculate CPU usage
std::map<std::string, double> get_cpu_usage() {
  std::vector<long> total_time_start = get_cpu_times();
  std::vector<long> idle_time_start = get_idle_times();
  std::this_thread::sleep_for(std::chrono::seconds(1));
  std::vector<long> total_time_end = get_cpu_times();
  std::vector<long> idle_time_end = get_idle_times();

  std::map<std::string, double> cpu_usages;
  for (size_t i = 0; i < total_time_start.size(); ++i) {
    long totald = total_time_end[i] - total_time_start[i];
    long idled = idle_time_end[i] - idle_time_start[i];
    std::string cpu_name = (i == 0) ? "total" : "CPU" + std::to_string(i - 1);
    cpu_usages[cpu_name] = totald > 0 ? 100.0 * (totald - idled) / totald : 0.0;
  }
  return cpu_usages;
}

void showCpuUsage() {
  std::map<std::string, double> cpu_usage = get_cpu_usage();
  double total_usage = cpu_usage["total"];
  std::wstring main_title = L"System Metrics Monitor: Cpu Usage";
  std::wstring instructions =
      L"F1 Overview F2 Cpu F3 Memory F4 Disk F5 ProcessInfo F6 Quit";
  auto screen = ScreenInteractive::Fullscreen();

  auto title_component = Title(main_title, L"");
  auto cpu_component = CpuMonitor(L"Total Usage: ", 1, total_usage);
  auto instructions_component = Title(instructions, L"");

  struct winsize size {};
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
  int terminal_height = (int)(size.ws_row);
  int number_of_cores = cpu_usage.size() - 1;
  int needed_height = ((number_of_cores / 2) * 3) + 9;

  auto handleCustomEvent = [&screen]() { screen.PostEvent(Event::Custom); };
  std::thread timerThread([handleCustomEvent]() {
    std::this_thread::sleep_for(std::chrono::seconds(2));
    handleCustomEvent();
  });

  std::vector<Component> cpu_components;
  for (int i = 0; i < number_of_cores; i++) {
    std::wstring title = L"Core " + std::to_wstring(i) + L":";
    cpu_components.push_back(
        CpuMonitor(title, 2, cpu_usage[std::string("CPU") + std::to_string(i)]));
  }

  auto mid_upper_container = Container::Vertical({cpu_component});
  auto middle_container = Container::Vertical({});
  if (number_of_cores > 0) {
    if (number_of_cores % 2 == 0) {
      for (int i = 0; i < number_of_cores; i += 2) {
        middle_container->Add(
            Container::Horizontal({cpu_components[i], cpu_components[i + 1]}));
      }
    } else {
      for (int i = 0; i < number_of_cores - 1; i += 2) {
        middle_container->Add(
            Container::Horizontal({cpu_components[i], cpu_components[i + 1]}));
      }
      middle_container->Add(
          Container::Horizontal({cpu_components[number_of_cores - 1]}));
    }
  }

  auto upper_container = Container::Vertical({title_component});
  auto lower_container = Container::Vertical({instructions_component});

  Component main_container;
  if (needed_height > terminal_height) {
    main_container = Container::Vertical(
        {upper_container, mid_upper_container,
         Title(L"Terminal window is too small to display all cores", L""),
         lower_container});
  } else {
    main_container =
        Container::Vertical({upper_container, mid_upper_container,
                             middle_container | flex, lower_container});
  }

  main_container |= CatchEvent([&](Event event) {
    if (event == Event::F1) {
      screen.ExitLoopClosure()();
      showOverview();
    } else if (event == Event::Custom) {
      screen.ExitLoopClosure()();
      showCpuUsage();
    } else if (event == Event::F3) {
      screen.ExitLoopClosure()();
      showMemoryUsage();
    } else if (event == Event::F4) {
      screen.ExitLoopClosure()();
      showDiskUsage();
    } else if (event == Event::F5) {
      screen.ExitLoopClosure()();
      showProcessInfo(1, getProcesses());
    } else if (event == Event::F6) {
      screen.ExitLoopClosure()();
    }
    return true;
  });
  screen.Loop(main_container);
}
