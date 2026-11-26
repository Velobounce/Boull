#include "ui.hpp"


// The widget layer. Every row, button and combo in the menu is drawn by hand rather than being a stock
// ImGui widget, so it matches the card style: an InvisibleButton provides the hit area, the look is
// painted onto the window draw list, and PressLatch fires the click on release so dragging the window
// never triggers a button.


namespace AnimeUI
{

static ImU32 ReadableText(ImU32 background)
{
    const ImVec4 c=U4(background);
    const auto linear=[](float x){return x<=0.04045f?x/12.92f:std::pow((x+0.055f)/1.055f,2.4f);};
    const float luminance=.2126f*linear(c.x)+.7152f*linear(c.y)+.0722f*linear(c.z);
    return luminance>.179f?IM_COL32(18,20,26,255):IM_COL32(255,255,255,255);
}

bool PressLatch(ImGuiID id, bool pressed)
{
    static std::map<ImGuiID, int> armed;
    int& st = armed[id];
    const ImGuiIO& io = ImGui::GetIO();
    if (pressed)
    {
        st = 1;
        g_widget_press = io.MousePos;
        if (g_drag_widget)
        {
            g_widget_armed = true;
            g_widget_press = io.MousePos;
        }
    }
    if (st == 1)
    {
        if (!io.MouseDown[ImGuiMouseButton_Left])
        {
            st = 0;
            g_widget_armed = false;
            return true;
        }
        const float dx = io.MousePos.x - g_widget_press.x;
        const float dy = io.MousePos.y - g_widget_press.y;
        const float th = PX(5.0f);
        if (g_drag_widget && dx * dx + dy * dy > th * th)
        {
            st = 0;
            g_widget_armed = false;
            g_widget_drag = true;
            g_dragging = true;
        }
    }
    return false;
}
// A round colour chip. Hand drawn instead of using a stock ImGui button, so it matches the rest of the
// menu; press it to make that colour the theme accent. The id must be unique per chip.
bool SwatchButton(const char* id, ImU32 col, ImVec2 size)
{
    ImGui::PushID(id);
    ImGui::InvisibleButton("##swatch", size);
    const ImGuiID wid = ImGui::GetItemID();
    const bool fired = PressLatch(wid, ImGui::IsItemClicked(ImGuiMouseButton_Left));
    const ImVec2 a = ImGui::GetItemRectMin();
    const ImVec2 b = ImGui::GetItemRectMax();
    const bool hovered = ImGui::IsItemHovered();
    const ImVec2 c = (a + b) * 0.5f;
    const float r = (b.x - a.x) * 0.5f;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (hovered)
        dl->AddCircleFilled(c + ImVec2(0.0f, PX(2.0f)), r + PX(3.0f), Alpha(Pal::Shadow, 0.16f * g_glow), 24);
    dl->AddCircleFilled(c, r, col, 24);
    // A white core keeps very light chips readable against the card, a ring separates dark ones.
    dl->AddCircleFilled(c, r * 0.30f, Alpha(Pal::White, 0.30f), 16);
    dl->AddCircle(c, r - PX(0.5f), hovered ? Mix(col, Pal::White, 0.40f) : Alpha(Pal::Line, 0.9f), 24, PX(1.0f));
    ImGui::PopID();
    return fired;
}
void DrawHeart(ImDrawList* dl, ImVec2 c, float s, ImU32 col)
{
    dl->AddCircleFilled(ImVec2(c.x - s * 0.48f, c.y - s * 0.30f), s * 0.50f, col);
    dl->AddCircleFilled(ImVec2(c.x + s * 0.48f, c.y - s * 0.30f), s * 0.50f, col);
    dl->AddTriangleFilled(ImVec2(c.x - s * 0.90f, c.y - s * 0.12f),
                          ImVec2(c.x + s * 0.90f, c.y - s * 0.12f),
                          ImVec2(c.x, c.y + s * 0.92f), col);
}
void DrawMark(ImDrawList* dl, ImVec2 c, float r, ImU32 col, ImU32 core)
{
    dl->AddQuadFilled(c+ImVec2(0,-r),c+ImVec2(r,0),c+ImVec2(0,r),c+ImVec2(-r,0),col);
    dl->AddCircleFilled(c,r*.25f,core,12);
}
void Burst(ImVec2 pos, int n, ImU32 col, float spd)
{
    if (!g_sparks) return;
    for (int i = 0; i < n; i++)
    {
        if (g_spark_pool.size() >= 320) break;
        const float a = Randf() * 2.0f * kPi;
        const float s = spd * (0.30f + 0.70f * Randf());
        Spark sp;
        sp.pos = pos;
        sp.vel = ImVec2(std::cos(a) * s, std::sin(a) * s - 55.0f);
        sp.ttl = 0.40f + 0.55f * Randf();
        sp.life = sp.ttl;
        sp.size = PX(2.0f) + PX(5.0f) * Randf();
        sp.col = col;
        sp.heart = (Randf() < 0.35f);
        g_spark_pool.push_back(sp);
    }
}
void DrawMascot(ImDrawList* dl, ImVec2 c, float t)
{
    const float bob = std::sin(t * 2.1f) * PX(2.2f);
    const float head_r = PX(15.0f);
    const ImVec2 h = ImVec2(c.x, c.y + bob);
    const bool blink = std::fmod(t, 3.4f) < 0.13f;
    const ImU32 hair = Mix(g_accent, Pal::Lilac, 0.35f);
    dl->AddCircleFilled(ImVec2(h.x - head_r * 1.05f, h.y - head_r * 0.15f), head_r * 0.62f, hair);
    dl->AddCircleFilled(ImVec2(h.x + head_r * 1.05f, h.y - head_r * 0.15f), head_r * 0.62f, hair);
    dl->AddCircleFilled(h, head_r, Pal::Cream);
    dl->AddTriangleFilled(ImVec2(h.x - head_r, h.y - head_r * 0.35f),
                          ImVec2(h.x + head_r, h.y - head_r * 0.35f),
                          ImVec2(h.x, h.y - head_r * 1.75f), hair);
    dl->AddTriangleFilled(ImVec2(h.x - head_r * 1.15f, h.y - head_r * 0.55f),
                          ImVec2(h.x - head_r * 0.05f, h.y - head_r * 0.55f),
                          ImVec2(h.x - head_r * 0.60f, h.y + head_r * 0.45f), hair);
    dl->AddTriangleFilled(ImVec2(h.x + head_r * 1.15f, h.y - head_r * 0.55f),
                          ImVec2(h.x + head_r * 0.05f, h.y - head_r * 0.55f),
                          ImVec2(h.x + head_r * 0.60f, h.y + head_r * 0.45f), hair);
    const float eye_dx = head_r * 0.42f, eye_y = h.y + head_r * 0.12f, eye_r = head_r * 0.19f;
    if (blink)
    {
        dl->AddLine(ImVec2(h.x - eye_dx - eye_r, eye_y), ImVec2(h.x - eye_dx + eye_r, eye_y), Pal::Text, PX(1.4f));
        dl->AddLine(ImVec2(h.x + eye_dx - eye_r, eye_y), ImVec2(h.x + eye_dx + eye_r, eye_y), Pal::Text, PX(1.4f));
    }
    else
    {
        dl->AddCircleFilled(ImVec2(h.x - eye_dx, eye_y), eye_r, Pal::Text);
        dl->AddCircleFilled(ImVec2(h.x + eye_dx, eye_y), eye_r, Pal::Text);
        dl->AddCircleFilled(ImVec2(h.x - eye_dx + eye_r * 0.35f, eye_y - eye_r * 0.4f), eye_r * 0.36f, Pal::White);
        dl->AddCircleFilled(ImVec2(h.x + eye_dx + eye_r * 0.35f, eye_y - eye_r * 0.4f), eye_r * 0.36f, Pal::White);
    }
    dl->AddCircleFilled(ImVec2(h.x - head_r * 0.72f, h.y + head_r * 0.48f), head_r * 0.20f, Alpha(Pal::PinkD, 0.35f));
    dl->AddCircleFilled(ImVec2(h.x + head_r * 0.72f, h.y + head_r * 0.48f), head_r * 0.20f, Alpha(Pal::PinkD, 0.35f));
    dl->AddLine(ImVec2(h.x - PX(2.4f), h.y + head_r * 0.50f), ImVec2(h.x, h.y + head_r * 0.62f), Pal::Text, PX(1.2f));
    dl->AddLine(ImVec2(h.x, h.y + head_r * 0.62f), ImVec2(h.x + PX(2.4f), h.y + head_r * 0.50f), Pal::Text, PX(1.2f));
    for (int i = 0; i < 3; i++)
    {
        const float a = t * 0.9f + i * 2.0943951f;
        const float rr = head_r * 1.75f + std::sin(t * 1.7f + (float)i) * PX(1.5f);
        const ImVec2 sp(h.x + std::cos(a) * rr, h.y + std::sin(a) * rr * 0.6f);
        const ImU32 cc = (i == 0) ? Pal::Sky : ((i == 1) ? Pal::Mint : g_accent);
        dl->AddCircleFilled(sp, PX(1.9f), Alpha(cc, 0.85f));
        dl->AddLine(sp - ImVec2(PX(4.0f), 0.0f), sp + ImVec2(PX(4.0f), 0.0f), Alpha(cc, 0.55f), PX(1.0f));
        dl->AddLine(sp - ImVec2(0.0f, PX(4.0f)), sp + ImVec2(0.0f, PX(4.0f)), Alpha(cc, 0.55f), PX(1.0f));
    }
}
bool TabButton(const char* label, bool selected, ImVec2 size, int index)
{
    ImGui::PushID(index);
    const bool clicked = ImGui::InvisibleButton(label, size);
    const bool pressed = PressLatch(ImGui::GetItemID(), clicked);
    const ImGuiID id = ImGui::GetItemID();
    const ImVec2 a = ImGui::GetItemRectMin();
    const ImVec2 b = ImGui::GetItemRectMax();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const bool hovered = ImGui::IsItemHovered();
    static std::map<ImGuiID, float> hover_map, sel_map;
    float& hv = hover_map[id];
    float& sl = sel_map[id];
    hv = ExpLerp(hv, hovered ? 1.0f : 0.0f, 15.0f, g_dt);
    sl = ExpLerp(sl, selected ? 1.0f : 0.0f, 19.0f, g_dt);
    const float r = size.y * 0.5f;
    dl->AddRectFilled(a + ImVec2(0.0f, PX(3.0f)), b + ImVec2(0.0f, PX(3.0f)),
                      Alpha(Pal::Shadow, 0.13f * g_glow * (0.4f + hv)), r);
    const ImU32 tabColor=Mix(Pal::FrameBg,g_accent,sl * 0.32f + hv * 0.08f);
    dl->AddRectFilled(a, b, tabColor, r);
    dl->AddRect(a, b, Alpha(Mix(Pal::LineSoft, g_accent, sl), 0.35f + 0.65f * sl), r, 0, PX(1.0f));
    const float ir = size.y * 0.30f;
    const ImVec2 ic(a.x + PX(9.0f) + ir, a.y + size.y * 0.5f);
    // The icon needs its own contrast: a plain white flower on the white/pink button was invisible
    // until the tab was selected, so it sits on a soft disc and the petals carry real pigment.
    dl->AddCircleFilled(ic, ir * 1.20f, Alpha(Mix(Pal::LineSoft, g_accent, sl), 0.55f - 0.30f * sl), 24);
    const ImU32 petal = Mix(Mix(Pal::PinkD, Pal::Lilac, 0.50f), g_accent, sl);
    const ImU32 core  = Mix(Pal::Cream, Pal::White, 0.10f + 0.45f * (1.0f - sl));
    DrawMark(dl, ic, ir * 0.75f, petal, core);
    const float ts = ImGui::GetTextLineHeight();
    dl->AddText(ImVec2(ic.x + ir * 1.20f + PX(8.0f), a.y + (size.y - ts) * 0.5f - PX(0.5f)),
                ReadableText(tabColor), label);
    if (sl > 0.02f)
    {
        const float ax = b.x - PX(20.0f), cy = a.y + size.y * 0.5f;
        dl->AddTriangleFilled(ImVec2(ax - PX(2.0f), cy - PX(4.0f)),
                              ImVec2(ax + PX(4.0f), cy),
                              ImVec2(ax - PX(2.0f), cy + PX(4.0f)), Alpha(g_accent, sl));
    }
    if (pressed)
    {
        Burst(ImVec2(b.x - PX(22.0f), a.y + size.y * 0.5f), 16, g_accent, 220.0f);
        if (!selected) hv = 1.0f;
    }
    else if (hovered && Randf() < 0.10f)
        Burst(ImVec2(a.x + Randf() * size.x, a.y + size.y * 0.5f), 1, Mix(g_accent, Pal::White, 0.4f), 60.0f);
    ImGui::PopID();
    return pressed;
}
bool ToggleRow(const char* label, bool* v, float width, float height)
{
    if (width <= 0.0f) width = ImGui::GetContentRegionAvail().x;
    const float h = PX(height);
    const float tw = PX(46.0f), th = PX(24.0f);
    ImGui::PushID(label);
    const bool clicked = ImGui::InvisibleButton("##tg", ImVec2(width, h));
    const ImGuiID id = ImGui::GetItemID();
    const bool pressed = PressLatch(id, clicked);
    const ImVec2 a = ImGui::GetItemRectMin();
    const ImVec2 b = ImGui::GetItemRectMax();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const bool hovered = ImGui::IsItemHovered();
    static std::map<ImGuiID, float> anim_map;
    float& t = anim_map[id];
    if (pressed) *v = !(*v);
    t = ExpLerp(t, *v ? 1.0f : 0.0f, 17.0f, g_dt);
    const ImU32 rowColor=Mix(Pal::FrameBg,g_accent,hovered?0.13f:0.03f);
    dl->AddRectFilled(a, b, Alpha(rowColor, g_card_a), PX(12.0f));
    dl->AddRect(a, b, Alpha(Mix(Pal::LineSoft, g_accent, t * 0.8f), 0.9f), PX(12.0f), 0, PX(1.0f));
    const float ts = ImGui::GetTextLineHeight();
    dl->AddText(ImVec2(a.x + PX(14.0f), a.y + (h - ts) * 0.5f - PX(0.5f)), ReadableText(rowColor), label);
    const ImVec2 tr(b.x - PX(13.0f) - tw, a.y + (h - th) * 0.5f);
    const ImVec2 tr2(tr.x + tw, tr.y + th);
    if (t > 0.02f)
        dl->AddRectFilled(tr + ImVec2(0.0f, PX(2.0f)), tr2 + ImVec2(0.0f, PX(2.0f)),
                          Alpha(Pal::Shadow, 0.18f * g_glow * t), th * 0.5f);
    dl->AddRectFilled(tr, tr2, Mix(hovered ? Pal::Track : Pal::FrameBg, g_accent, t), th * 0.5f);
    const float kr = th * 0.5f - PX(3.0f);
    const float kx = tr.x + PX(3.0f) + kr + t * (tw - PX(6.0f) - kr * 2.0f);
    const ImVec2 kc(kx, tr.y + th * 0.5f);
    dl->AddCircleFilled(kc + ImVec2(0.0f, PX(1.0f)), kr, Alpha(Pal::Shadow, 0.25f * g_glow));
    dl->AddCircleFilled(kc, kr, Pal::White);
    if (t > 0.5f) {
        const ImU32 mark=Alpha(Mix(g_accent,IM_COL32(0,0,0,255),0.35f),(t-0.5f)*2.0f);
        dl->AddLine(kc+ImVec2(-kr*0.48f,-kr*0.20f),kc+ImVec2(0,kr*0.32f),mark,PX(1.8f));
        dl->AddLine(kc+ImVec2(0,kr*0.32f),kc+ImVec2(kr*0.48f,-kr*0.20f),mark,PX(1.8f));
    }
    if (pressed) Burst(kc, 12, Mix(g_accent, Pal::White, 0.35f), 170.0f);
    ImGui::PopID();
    return pressed;
}
bool SliderRow(const char* label, float* v, float vmin, float vmax, const char* fmt, float width)
{
    if (width <= 0.0f) width = ImGui::GetContentRegionAvail().x;
    const float h = PX(44.0f);
    ImGui::PushID(label);
    ImGui::InvisibleButton("##sl", ImVec2(width, h));
    const ImGuiID id = ImGui::GetItemID();
    const ImVec2 a = ImGui::GetItemRectMin();
    const ImVec2 b = ImGui::GetItemRectMax();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const bool hovered = ImGui::IsItemHovered();
    const bool active = ImGui::IsItemActive();
    const bool drag = active && ImGui::IsMouseDragging(ImGuiMouseButton_Left);
    const float track_h = PX(9.0f);
    const float track_w = width - PX(24.0f);
    const ImVec2 tr(a.x + PX(12.0f), a.y + h - track_h - PX(9.0f));
    const ImVec2 tr2(tr.x + track_w, tr.y + track_h);
    const bool clicked = hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    bool changed = false;
    if ((drag || clicked) && track_w > 1.0f)
    {
        const float nt = Saturate((ImGui::GetIO().MousePos.x - tr.x) / track_w);
        const float nv = vmin + (vmax - vmin) * nt;
        if (nv != *v)
        {
            *v = nv;
            changed = true;
            Burst(ImVec2(tr.x + track_w * nt, tr.y + track_h * 0.5f), 3, Mix(g_accent, Pal::White, 0.4f), 70.0f);
        }
    }
    if (hovered && ImGui::GetIO().MouseWheel != 0.0f)
    {
        const float nv = *v + ImGui::GetIO().MouseWheel * (vmax - vmin) * 0.03f;
        if (nv != *v) { *v = nv; changed = true; }
    }
    if (*v < vmin) *v = vmin;
    if (*v > vmax) *v = vmax;
    const float nt = Saturate((*v - vmin) / (vmax - vmin));
    static std::map<ImGuiID, float> knob_map;
    float& ka = knob_map[id];
    ka = ExpLerp(ka, nt, 24.0f, g_dt);
    dl->AddText(ImVec2(a.x + PX(13.0f), a.y + PX(1.0f)), Pal::Text, label);
    char buf[64];
    std::snprintf(buf, sizeof(buf), fmt, *v);
    const ImVec2 vs = ImGui::CalcTextSize(buf);
    dl->AddText(ImVec2(b.x - PX(13.0f) - vs.x, a.y + PX(1.0f)), active ? g_accent : Pal::TextDim, buf);
    dl->AddRectFilled(tr + ImVec2(0.0f, PX(2.0f)), tr2 + ImVec2(0.0f, PX(2.0f)),
                      Alpha(Pal::Shadow, 0.14f * g_glow), track_h * 0.5f);
    dl->AddRectFilled(tr, tr2, hovered ? Pal::FrameHov : Pal::FrameBg, track_h * 0.5f);
    if (nt > 0.001f)
    {
        const ImVec2 f2(tr.x + track_w * nt, tr2.y);
        dl->AddRectFilled(tr, f2, Mix(g_accent, Pal::Lilac, 0.30f), track_h * 0.5f);
    }
    const float kr = track_h * 0.85f;
    const ImVec2 kc(tr.x + track_w * ka, tr.y + track_h * 0.5f);
    if (hovered || active) dl->AddCircleFilled(kc, kr * 2.0f, Alpha(g_accent, active ? 0.22f : 0.12f));
    dl->AddCircleFilled(kc + ImVec2(0.0f, PX(1.0f)), kr, Alpha(Pal::Shadow, 0.22f * g_glow));
    dl->AddCircleFilled(kc, kr, Pal::White);
    dl->AddCircleFilled(kc, kr * 0.55f, Mix(g_accent, Pal::Lilac, 0.35f));
    if (active) DrawHeart(dl, kc, kr * 0.95f, Pal::White);
    ImGui::PopID();
    return changed;
}
bool SelectRow(const char* label, bool selected, ImVec2 size)
{
    ImGui::PushID(label);
    const bool pressed = ImGui::InvisibleButton("##sr", size);
    const ImVec2 a = ImGui::GetItemRectMin();
    const ImVec2 b = ImGui::GetItemRectMax();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const bool hovered = ImGui::IsItemHovered();
    if (selected) dl->AddRectFilled(a, b, Alpha(g_accent, 0.20f), PX(10.0f));
    else if (hovered) dl->AddRectFilled(a, b, Alpha(g_accent, 0.09f), PX(10.0f));
    const float ts = ImGui::GetTextLineHeight();
    dl->AddText(ImVec2(a.x + PX(12.0f), a.y + (size.y - ts) * 0.5f), selected ? Pal::PinkD : Pal::Text, label);
    if (selected) DrawHeart(dl, ImVec2(b.x - PX(14.0f), a.y + size.y * 0.5f), PX(9.0f), Alpha(g_accent, 0.85f));
    ImGui::PopID();
    return pressed;
}
bool ComboBox(const char* id, const char* label, int* cur, const char* const items[], int count, float width)
{
    if (width <= 0.0f) width = ImGui::GetContentRegionAvail().x;
    const float h = PX(36.0f);
    const float bw = ImMin(PX(190.0f), width * 0.58f);
    const int safe = (*cur < 0 || *cur >= count) ? 0 : *cur;
    char buf[256];
    std::snprintf(buf, sizeof(buf), "%s##%s", items[safe], id);
    ImGui::PushID(id);
    ImGui::InvisibleButton(buf, ImVec2(width, h));
    const ImGuiID wid = ImGui::GetItemID();
    const bool fired = PressLatch(wid, ImGui::IsItemClicked(ImGuiMouseButton_Left));
    const ImVec2 a = ImGui::GetItemRectMin();
    const ImVec2 b = ImGui::GetItemRectMax();
    const bool pressed = fired;
    const bool hovered = ImGui::IsItemHovered();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 box(b.x - bw, a.y);
    const ImVec2 box2(b.x, a.y + h);
    dl->AddRectFilled(box + ImVec2(0.0f, PX(2.0f)), box2 + ImVec2(0.0f, PX(2.0f)),
                      Alpha(Pal::Shadow, 0.12f * g_glow), PX(12.0f));
    dl->AddRectFilled(box, box2, hovered ? Pal::FrameHov : Pal::FrameBg, PX(12.0f));
    dl->AddRect(box, box2, Alpha(g_accent, hovered ? 0.85f : 0.35f), PX(12.0f), 0, PX(1.0f));
    const float ts = ImGui::GetTextLineHeight();
    dl->AddText(ImVec2(box.x + PX(13.0f), box.y + (h - ts) * 0.5f), Pal::Text, items[safe]);
    const float ax = box2.x - PX(15.0f), cy = box.y + h * 0.5f;
    dl->AddTriangleFilled(ImVec2(ax - PX(4.0f), cy - PX(2.0f)),
                          ImVec2(ax + PX(4.0f), cy - PX(2.0f)),
                          ImVec2(ax, cy + PX(3.0f)), Pal::TextDim);
    dl->AddText(ImVec2(a.x + PX(2.0f), a.y + (h - ts) * 0.5f), Pal::TextDim, label);
    if (pressed)
    {
        ImGui::OpenPopup(id);
        Burst((box + box2) * 0.5f - ImVec2(0.0f, h * 0.5f), 8, g_accent, 130.0f);
    }
    ImGui::SetNextWindowPos(ImVec2(box.x, box2.y + PX(6.0f)), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(bw, count * PX(31.0f) + PX(14.0f)), ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, U4(Pal::Card));
    ImGui::PushStyleColor(ImGuiCol_Border, U4(Pal::Line));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, PX(1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(PX(7.0f), PX(7.0f)));
    if (ImGui::BeginPopup(id))
    {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, PX(5.0f)));
        for (int i = 0; i < count; i++)
        {
            if (SelectRow(items[i], i == safe, ImVec2(bw - PX(14.0f), PX(26.0f))))
            {
                *cur = i;
                Burst(ImVec2(box.x + bw * 0.5f, box2.y), 10, g_accent, 160.0f);
            }
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
    ImGui::PopID();
    return pressed;
}
bool PillButton(const char* label, ImVec2 size, bool soft)
{
    ImGui::PushID(label);
    const bool clicked = ImGui::InvisibleButton("##pb", size);
    const ImGuiID id = ImGui::GetItemID();
    const bool pressed = PressLatch(id, clicked);
    const ImVec2 a = ImGui::GetItemRectMin();
    const ImVec2 b = ImGui::GetItemRectMax();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const bool hovered = ImGui::IsItemHovered();
    static std::map<ImGuiID, float> shine_map;
    float& sh = shine_map[id];
    sh = std::fmod(sh + (hovered ? 0.60f : 0.15f) * g_dt, 1.0f);
    const ImU32 c0 = soft ? Mix(Pal::FrameBg,g_accent,hovered?0.16f:0.06f) : g_accent;
    const float r = size.y * 0.5f;
    dl->AddRectFilled(a + ImVec2(0.0f, PX(3.0f)), b + ImVec2(0.0f, PX(3.0f)),
                      Alpha(Pal::Shadow, (0.20f + 0.15f * hovered) * g_glow), r);
    dl->AddRectFilled(a, b, c0, r);
    dl->AddRect(a, b, Alpha(soft ? Pal::Line : Mix(c0, Pal::White, 0.5f), soft ? 0.9f : 0.6f), r, 0, PX(1.0f));
    const float ts = ImGui::GetTextLineHeight();
    const ImVec2 vs = ImGui::CalcTextSize(label);
    dl->AddText(ImVec2(a.x + (size.x - vs.x) * 0.5f, a.y + (size.y - ts) * 0.5f - PX(0.5f)),
                ReadableText(c0), label);
    if (pressed)
    {
        Burst(ImVec2(a.x + size.x * 0.5f, a.y + size.y * 0.5f), 20, soft ? g_accent : Pal::White, 240.0f);
        sh = 0.0f;
    }
    ImGui::PopID();
    return pressed;
}
void SectionTag(ImDrawList* dl, const char* text, ImVec2 pos)
{
    const ImVec2 ts = ImGui::CalcTextSize(text);
    const ImVec2 box(pos.x, pos.y);
    const ImVec2 box2(pos.x + ts.x + PX(30.0f), pos.y + PX(19.0f));
    dl->AddRectFilled(box, box2, Alpha(g_accent, 0.14f), PX(9.0f));
    DrawMark(dl, ImVec2(box.x + PX(11.0f), box.y + PX(9.5f)), PX(5.0f), Mix(g_accent, Pal::White, 0.2f), Pal::White);
    dl->AddText(ImVec2(box.x + PX(22.0f), box.y + PX(1.0f)), Pal::Text, text);
}

} // namespace AnimeUI
