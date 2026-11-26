#include "ui.hpp"


// The ImGui style is rebuilt from the accent colour, so every part of the interface follows the colour
// the user picked. ApplyTheme() is re-run whenever that colour changes; Push/PopAccentStyle only tints
// the accent coloured widgets inside one scope.


namespace AnimeUI
{

void ApplyTheme()
{
    ImGuiStyle& s = ImGui::GetStyle();
    ImVec4* c = s.Colors;
    s.WindowRounding = 18.0f;
    s.ChildRounding = 16.0f;
    s.PopupRounding = 16.0f;
    s.FrameRounding = 12.0f;
    s.GrabRounding = 12.0f;
    s.ScrollbarRounding = 12.0f;
    s.TabRounding = 12.0f;
    s.ImageRounding = 8.0f;
    s.WindowPadding = ImVec2(0.0f, 0.0f);
    s.FramePadding = ImVec2(PX(12.0f), PX(8.0f));
    s.ItemSpacing = ImVec2(PX(10.0f), PX(10.0f));
    s.ItemInnerSpacing = ImVec2(PX(8.0f), PX(8.0f));
    s.CellPadding = ImVec2(PX(8.0f), PX(6.0f));
    s.ScrollbarSize = PX(8.0f);
    s.GrabMinSize = PX(12.0f);
    s.WindowBorderSize = 0.0f;
    s.ChildBorderSize = 0.0f;
    s.PopupBorderSize = 0.0f;
    s.FrameBorderSize = 0.0f;
    s.TabBarBorderSize = 0.0f;
    s.SeparatorTextBorderSize = 0.0f;
    s.AntiAliasedLines = true;
    s.AntiAliasedFill = true;
    s.AntiAliasedLinesUseTex = true;
    s.CurveTessellationTol = 1.0f;
    s.CircleTessellationMaxError = 0.20f;
    c[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    c[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    c[ImGuiCol_PopupBg] = U4(Pal::Card);
    c[ImGuiCol_Border] = U4(Pal::Line);
    c[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    c[ImGuiCol_Text] = U4(Pal::Text);
    c[ImGuiCol_TextDisabled] = U4(Pal::TextDim);
    c[ImGuiCol_TextSelectedBg] = U4(Alpha(g_accent, 0.30f));
    c[ImGuiCol_FrameBg] = U4(Pal::FrameBg);
    c[ImGuiCol_FrameBgHovered] = U4(Pal::FrameHov);
    c[ImGuiCol_FrameBgActive] = U4(Pal::FrameAct);
    c[ImGuiCol_TitleBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    c[ImGuiCol_TitleBgActive] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    c[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    c[ImGuiCol_Button] = U4(g_accent);
    c[ImGuiCol_ButtonHovered] = U4(Mix(g_accent, Pal::White, 0.30f));
    c[ImGuiCol_ButtonActive] = U4(Mix(g_accent, Pal::PinkD, 0.35f));
    c[ImGuiCol_Header] = U4(Alpha(g_accent, 0.30f));
    c[ImGuiCol_HeaderHovered] = U4(Alpha(g_accent, 0.50f));
    c[ImGuiCol_HeaderActive] = U4(Alpha(g_accent, 0.70f));
    c[ImGuiCol_CheckMark] = U4(Pal::PinkD);
    c[ImGuiCol_SliderGrab] = U4(g_accent);
    c[ImGuiCol_SliderGrabActive] = U4(Pal::PinkD);
    c[ImGuiCol_Separator] = U4(Pal::LineSoft);
    c[ImGuiCol_SeparatorHovered] = U4(g_accent);
    c[ImGuiCol_SeparatorActive] = U4(g_accent);
    c[ImGuiCol_ScrollbarBg] = U4(Alpha(Pal::Track, 0.55f));
    c[ImGuiCol_ScrollbarGrab] = U4(Alpha(g_accent, 0.55f));
    c[ImGuiCol_ScrollbarGrabHovered] = U4(g_accent);
    c[ImGuiCol_ScrollbarGrabActive] = U4(g_accent);
    c[ImGuiCol_ResizeGrip] = U4(Alpha(g_accent, 0.30f));
    c[ImGuiCol_ResizeGripHovered] = U4(g_accent);
    c[ImGuiCol_ResizeGripActive] = U4(g_accent);
    c[ImGuiCol_NavCursor] = U4(g_accent);
    c[ImGuiCol_DragDropTarget] = U4(g_accent);
    c[ImGuiCol_TableHeaderBg] = U4(Alpha(Pal::Lilac, 0.35f));
    c[ImGuiCol_TableBorderStrong] = U4(Pal::Line);
    c[ImGuiCol_TableBorderLight] = U4(Pal::LineSoft);
    c[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    c[ImGuiCol_TableRowBgAlt] = U4(Alpha(Pal::Pink, 0.10f));
    c[ImGuiCol_PlotLines] = U4(g_accent);
    c[ImGuiCol_PlotLinesHovered] = U4(Pal::PinkD);
    c[ImGuiCol_PlotHistogram] = U4(Alpha(g_accent, 0.60f));
    c[ImGuiCol_PlotHistogramHovered] = U4(g_accent);
}
void PushAccentStyle()
{
    ImGui::PushStyleColor(ImGuiCol_Button, U4(g_accent));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, U4(Mix(g_accent, Pal::White, 0.30f)));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, U4(Mix(g_accent, Pal::PinkD, 0.35f)));
    ImGui::PushStyleColor(ImGuiCol_Header, U4(Alpha(g_accent, 0.30f)));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, U4(Alpha(g_accent, 0.50f)));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, U4(Alpha(g_accent, 0.70f)));
    ImGui::PushStyleColor(ImGuiCol_CheckMark, U4(Pal::PinkD));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, U4(g_accent));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, U4(Pal::PinkD));
    ImGui::PushStyleColor(ImGuiCol_Separator, U4(Pal::LineSoft));
    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, U4(Alpha(g_accent, 0.30f)));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, U4(Pal::FrameBg));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, U4(Pal::FrameHov));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, U4(Pal::FrameAct));
}
void PopAccentStyle() { ImGui::PopStyleColor(14); }

} // namespace AnimeUI
