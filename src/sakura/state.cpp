#include "ui.hpp"


// Shared state. Everything the rest of the UI reads or writes is defined here, exactly once, so no
// translation unit ends up with its own copy.


namespace AnimeUI
{

ImU32  g_accent = IM_COL32(235, 38, 51, 255);
ImVec4 g_accent_f = ImVec4(0.92f, 0.15f, 0.20f, 1.00f);
ImVec4 g_background_f = ImVec4(0.10f, 0.035f, 0.045f, 1.0f);
bool g_custom_background = true;
float  g_dt = 1.0f / 60.0f;
float  g_time = 0.0f;
float  g_s = 1.0f;
bool   g_rainbow = false;
int    g_selectedModule = 0;
bool   g_petals = false;
bool   g_sparks = true;
float  g_card_a = 1.00f;
float  g_glow = 0.35f;
float  g_petal_spd = 0.45f;
bool   g_tilt_on = true;
bool   g_tilt_follow = true;
bool   g_tilt_orbit = true;
float  g_tilt_deg = 5.00f;
float  g_tilt_smooth = 8.00f;
float  g_tilt_yaw = 0.0f;
float  g_tilt_pitch = 0.0f;
bool   g_dragging = false;
bool   g_visible = true;
bool   g_drag_widget = true;
bool   g_widget_armed = false;
bool   g_widget_drag = false;
ImVec2 g_widget_press(0.0f, 0.0f);
ImTextureID g_logoTexture = ImTextureID_Invalid;
ImFont* g_font_bold = nullptr;
std::vector<Spark> g_spark_pool;
std::vector<Petal> g_petal_pool;
ImVec2 g_petal_anchor = ImVec2(-99999.0f, -99999.0f);

} // namespace AnimeUI


