#include "../include/Components.hpp"
#include "../include/TitleComponent.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/screen/color.hpp"

using namespace ftxui;

Component Title(std::wstring title_str, std::wstring content_str) {
  // Using Renderer to create a component from a lambda that returns an Element.
  return Renderer([=] {
    auto my_border = borderStyled(ROUNDED, Color::RGB(238, 238, 238));
    auto text_color = color(Color::RGB(255, 211, 105));
    auto background_color = bgcolor(Color::RGB(34, 40, 49));
    auto space = std::wstring(1, L' ');
    return vbox({
        text(title_str + space + content_str) | hcenter | vcenter | flex,
    }) | flex | text_color | my_border | background_color;
  });
}
