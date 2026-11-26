#include "ui.hpp"
namespace AnimeUI {
void Apply3DTilt(ImDrawList* dl, ImRect win)
{
    const ImVec2 center(win.GetCenter().x, win.GetCenter().y);
    const float half_w = win.GetWidth() * 0.5f;
    const float half_h = win.GetHeight() * 0.5f;
    const ImGuiIO& io = ImGui::GetIO();
    const bool mouse_valid = io.MousePos.x > -1e6f && io.MousePos.y > -1e6f;
    // While dragging the window the mouse can leave the (untransformed) window rect, so keep the
    // mouse tracking alive as long as the left button is held down.
    const bool inside = mouse_valid && (g_dragging ||
                        (io.MousePos.x >= win.Min.x && io.MousePos.x <= win.Max.x &&
                         io.MousePos.y >= win.Min.y && io.MousePos.y <= win.Max.y));
    float nx = 0.0f, ny = 0.0f;
    if (g_tilt_on && g_tilt_follow && inside)
    {
        nx = ImClamp((io.MousePos.x - center.x) / (half_w > 1.0f ? half_w : 1.0f), -1.0f, 1.0f);
        ny = ImClamp((io.MousePos.y - center.y) / (half_h > 1.0f ? half_h : 1.0f), -1.0f, 1.0f);
    }
    else if (g_tilt_on && g_tilt_orbit)
    {
        nx = std::sin(g_time * 0.55f);
        ny = std::sin(g_time * 0.37f + 1.1f) * 0.65f;
    }
    const float max_rad = g_tilt_deg * kPi / 180.0f;
    g_tilt_yaw   = ExpLerp(g_tilt_yaw, nx * max_rad, g_tilt_smooth, g_dt);
    g_tilt_pitch = ExpLerp(g_tilt_pitch, -ny * max_rad, g_tilt_smooth, g_dt);
    const float ay = std::fabs(g_tilt_yaw), ap = std::fabs(g_tilt_pitch);
    if (ay < 1e-4f && ap < 1e-4f)
        return;
    const float cy = std::cos(g_tilt_yaw), sy = std::sin(g_tilt_yaw);
    const float cp = std::cos(g_tilt_pitch), sp = std::sin(g_tilt_pitch);
    const float focal = win.GetWidth() * 1.8f;
    for (int i = 0; i < dl->VtxBuffer.Size; i++)
    {
        ImDrawVert& v = dl->VtxBuffer[i];
        const float x = v.pos.x - center.x;
        const float y = v.pos.y - center.y;
        const float z = -x * sy;
        const float rz = y * sp + z * cp;
        float s = focal / (focal - rz);
        if (s < 0.1f) s = 0.1f;
        v.pos.x = center.x + x * cy * s;
        v.pos.y = center.y + (y * cp - z * sp) * s;
    }
    const float pad = half_w * (ay > ap ? ay : ap) * 2.2f + PX(10.0f);
    for (int i = 0; i < dl->CmdBuffer.Size; i++)
    {
        ImDrawCmd& c = dl->CmdBuffer[i];
        c.ClipRect.x -= pad;
        c.ClipRect.y -= pad;
        c.ClipRect.z += pad;
        c.ClipRect.w += pad;
    }
}

ImVec2 UnprojectTiltMouse(ImVec2 mouse)
{
    const float x=mouse.x-410.0f, y=mouse.y-270.0f, focal=820.0f*1.8f;
    const float cy=std::cos(g_tilt_yaw), sy=std::sin(g_tilt_yaw);
    const float cp=std::cos(g_tilt_pitch), sp=std::sin(g_tilt_pitch);
    const float a=focal*cy-x*sy*cp, b=x*sp;
    const float c=focal*sy*sp-y*sy*cp, d=focal*cp+y*sp;
    const float determinant=a*d-b*c;
    if(std::fabs(determinant)<0.001f)return mouse;
    return ImVec2(410.0f+focal*(x*d-b*y)/determinant,
                  270.0f+focal*(a*y-x*c)/determinant);
}

}

