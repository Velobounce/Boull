#pragma once

// The whole menu UI. It used to be one header next to main.cpp; it is split by concern so each piece
// can be found and edited on its own:
//   state.cpp    shared state, defined exactly once
//   theme.cpp    palette application (ApplyTheme) and the accent scoped style push/pop
//   widgets.cpp  the hand drawn ImGui widget layer and the small drawing primitives
//   fx.cpp       petals and sparkles
//   menu.cpp     the menu window, the four tabs and the 3D tilt
// main.cpp only needs the three host facing symbols at the bottom of this file.

#include "imgui.h"
#include "imgui_internal.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <vector>


namespace AnimeUI
{

static const float kPi = 3.14159265358979323846f;

inline float Saturate(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }
inline float Lerpf(float a, float b, float t) { return a + (b - a) * t; }
inline float ExpLerp(float cur, float target, float speed, float dt) { return Lerpf(cur, target, 1.0f - std::exp(-speed * dt)); }
inline float Randf() { return (float)std::rand() / (float)RAND_MAX; }
inline ImVec4 U4(ImU32 c) { return ImGui::ColorConvertU32ToFloat4(c); }

inline ImU32 Mix(ImU32 a, ImU32 b, float t)
{
    const ImVec4 ca = ImGui::ColorConvertU32ToFloat4(a);
    const ImVec4 cb = ImGui::ColorConvertU32ToFloat4(b);
    return ImGui::ColorConvertFloat4ToU32(ImVec4(ca.x + (cb.x - ca.x) * t, ca.y + (cb.y - ca.y) * t,
                                                ca.z + (cb.z - ca.z) * t, ca.w + (cb.w - ca.w) * t));
}

inline ImU32 Alpha(ImU32 c, float a) { return (c & 0x00FFFFFFu) | ((ImU32)(Saturate(a) * 255.0f) << 24); }

inline ImU32 Hsv(float h, float s, float v, float a = 1.0f)
{
    h = std::fmod(h, 1.0f);
    if (h < 0.0f) h += 1.0f;
    const float i = std::floor(h * 6.0f);
    const float f = h * 6.0f - i;
    const float p = v * (1.0f - s);
    const float q = v * (1.0f - s * f);
    const float t = v * (1.0f - s * (1.0f - f));
    float r, g, b;
    switch ((int)i % 6)
    {
    case 0:  r = v; g = t; b = p; break;
    case 1:  r = q; g = v; b = p; break;
    case 2:  r = p; g = v; b = t; break;
    case 3:  r = p; g = q; b = v; break;
    case 4:  r = t; g = p; b = v; break;
    default: r = v; g = p; b = q; break;
    }
    return IM_COL32((int)(r * 255.0f), (int)(g * 255.0f), (int)(b * 255.0f), (int)(Saturate(a) * 255.0f));
}

inline ImVec4 Hsvf(float h, float s, float v, float a = 1.0f) { return U4(Hsv(h, s, v, a)); }

// The inverse of the above, needed because the picker keeps the colour as RGB but has to draw the hue
// bar and the saturation square from hue/saturation/value.
inline void RgbToHsv(const ImVec4& c, float* h, float* s, float* v)
{
    const float mx = ImMax(c.x, ImMax(c.y, c.z));
    const float mn = ImMin(c.x, ImMin(c.y, c.z));
    const float d = mx - mn;
    *v = mx;
    *s = (mx <= 0.0f) ? 0.0f : (d / mx);
    if (d <= 1e-6f) { *h = 0.0f; return; }
    if (mx == c.x)      *h = (c.y - c.z) / d + (c.y < c.z ? 6.0f : 0.0f);
    else if (mx == c.y) *h = (c.z - c.x) / d + 2.0f;
    else                *h = (c.x - c.y) / d + 4.0f;
    *h /= 6.0f;
}

namespace Pal
{
    inline ImU32 Bg0      = IM_COL32(255, 246, 251, 255);
    inline ImU32 Bg1      = IM_COL32(238, 233, 255, 255);
    inline ImU32 Head0    = IM_COL32(255, 205, 231, 255);
    inline ImU32 Head1    = IM_COL32(213, 197, 255, 255);
    inline ImU32 Card     = IM_COL32(255, 255, 255, 240);
    inline ImU32 CardSoft = IM_COL32(255, 241, 248, 240);
    inline ImU32 Side     = IM_COL32(255, 228, 242, 246);
    inline ImU32 Line     = IM_COL32(255, 198, 224, 255);
    inline ImU32 LineSoft = IM_COL32(255, 218, 236, 255);
    inline ImU32 Pink     = IM_COL32(255, 138, 192, 255);
    inline ImU32 PinkD    = IM_COL32(240,  86, 156, 255);
    inline ImU32 Lilac    = IM_COL32(186, 156, 255, 255);
    inline ImU32 Sky      = IM_COL32(141, 227, 255, 255);
    inline ImU32 Mint     = IM_COL32(168, 240, 208, 255);
    inline ImU32 Cream    = IM_COL32(255, 248, 231, 255);
    inline ImU32 Text     = IM_COL32( 91,  62,  88, 255);
    inline ImU32 TextDim  = IM_COL32(168, 142, 165, 255);
    inline ImU32 Track    = IM_COL32(248, 221, 238, 255);
    inline ImU32 FrameBg  = IM_COL32(255, 244, 249, 255);
    inline ImU32 FrameHov = IM_COL32(255, 230, 243, 255);
    inline ImU32 FrameAct = IM_COL32(255, 218, 236, 255);
    inline ImU32 Shadow   = IM_COL32(198, 140, 180, 255);
    inline ImU32 White    = IM_COL32(255, 255, 255, 255);
}

// ---- shared state (defined in state.cpp) --------------------------------------------
extern ImU32  g_accent;
// The accent the user picked in the Visuals tab. Kept as floats because that is what the picker edits,
// and converted to ImU32 once per frame into g_accent, which is what the whole UI is tinted from.
extern ImVec4 g_accent_f;
extern ImVec4 g_background_f;
extern bool g_custom_background;
extern float  g_dt;
extern float  g_time;
extern float  g_s;
extern bool   g_rainbow;
extern int    g_selectedModule;
extern bool   g_petals;
extern bool   g_sparks;
extern float  g_card_a;
extern float  g_glow;
extern float  g_petal_spd;

extern bool   g_tilt_on;
extern bool   g_tilt_follow;
extern bool   g_tilt_orbit;
extern float  g_tilt_deg;
extern float  g_tilt_smooth;
extern float  g_tilt_yaw;
extern float  g_tilt_pitch;
extern bool   g_dragging;
extern bool   g_visible;
extern bool   g_drag_widget;
extern bool   g_widget_armed;
extern bool   g_widget_drag;
extern ImVec2 g_widget_press;
extern ImTextureID g_logoTexture;
extern ImFont* g_font_bold;    // heavy weight, set by main.cpp when the fonts are loaded

inline float PX(float v) { return v * g_s; }

inline ImVec2 operator+(const ImVec2& a, const ImVec2& b) { return ImVec2(a.x + b.x, a.y + b.y); }
inline ImVec2 operator-(const ImVec2& a, const ImVec2& b) { return ImVec2(a.x - b.x, a.y - b.y); }
inline ImVec2 operator-(const ImVec2& a) { return ImVec2(-a.x, -a.y); }
inline ImVec2 operator*(const ImVec2& a, float s) { return ImVec2(a.x * s, a.y * s); }
inline ImVec2 operator*(float s, const ImVec2& a) { return ImVec2(a.x * s, a.y * s); }
inline ImVec2 operator/(const ImVec2& a, float s) { return ImVec2(a.x / s, a.y / s); }
inline ImVec2& operator+=(ImVec2& a, const ImVec2& b) { a.x += b.x; a.y += b.y; return a; }

struct Spark { ImVec2 pos, vel; float life, ttl, size; ImU32 col; bool heart; };
struct Petal { ImVec2 pos; float rot, rotvel, w, h, vy, sway, phase, a; };

extern std::vector<Spark> g_spark_pool;
extern std::vector<Petal> g_petal_pool;
extern ImVec2 g_petal_anchor;

// ---- module interface -------------------------------------------------------------
bool PressLatch(ImGuiID id, bool pressed);
void ApplyTheme();
void PushAccentStyle();
void PopAccentStyle();
bool SwatchButton(const char* id, ImU32 col, ImVec2 size);
void DrawHeart(ImDrawList* dl, ImVec2 c, float s, ImU32 col);
void DrawMark(ImDrawList* dl, ImVec2 c, float r, ImU32 col, ImU32 core);
void Burst(ImVec2 pos, int n, ImU32 col, float spd = 190.0f);
void UpdateAndDrawSparks(ImDrawList* dl);
void SpawnPetals(const ImRect& r);
void UpdateAndDrawPetals(ImDrawList* dl, const ImRect& r);
void DrawMascot(ImDrawList* dl, ImVec2 c, float t);
bool TabButton(const char* label, bool selected, ImVec2 size, int index);
bool ToggleRow(const char* label, bool* v, float width = 0.0f, float height = 36.0f);
bool SliderRow(const char* label, float* v, float vmin, float vmax, const char* fmt = "%.1f", float width = 0.0f);
bool SelectRow(const char* label, bool selected, ImVec2 size);
bool ComboBox(const char* id, const char* label, int* cur, const char* const items[], int count, float width = 0.0f);
bool PillButton(const char* label, ImVec2 size, bool soft = false);
void SectionTag(ImDrawList* dl, const char* text, ImVec2 pos);
void Apply3DTilt(ImDrawList* dl, ImRect win);
void RenderMenu();

// ---- host facing ------------------------------------------------------------------
//
// Everything main.cpp may touch, and nothing else:
//
//   g_font_bold  ImFont*  filled in by the host with the heavy font once the fonts are loaded
//   g_visible    bool     F1 toggles this, and the host reads it to decide if the overlay may
//                          take a click
//   RenderMenu()          builds the menu for the current frame, between ImGui::NewFrame() and
//                          ImGui::Render()

}


