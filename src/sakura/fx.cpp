#include "ui.hpp"


// Decoration: the sakura petals falling past the card, and the sparkles that Burst() throws off every
// interaction. Both use fixed size pools, so a busy frame never allocates.


namespace AnimeUI
{

void UpdateAndDrawSparks(ImDrawList* dl)
{
    for (size_t i = 0; i < g_spark_pool.size();)
    {
        Spark& s = g_spark_pool[i];
        s.life -= g_dt;
        if (s.life <= 0.0f) { g_spark_pool.erase(g_spark_pool.begin() + (int)i); continue; }
        s.vel.y += 300.0f * g_dt;
        s.vel = ImVec2(s.vel.x * 0.965f, s.vel.y * 0.965f);
        s.pos = ImVec2(s.pos.x + s.vel.x * g_dt, s.pos.y + s.vel.y * g_dt);
        const float t = s.life / s.ttl;
        const ImU32 col = Alpha(s.col, Saturate(t) * 0.95f);
        if (s.heart)
        {
            DrawHeart(dl, s.pos, s.size * (0.7f + 0.6f * t), col);
        }
        else
        {
            dl->AddCircleFilled(s.pos, s.size * 1.8f * t, Alpha(col, 0.18f));
            dl->AddCircleFilled(s.pos, s.size * t, col);
            dl->AddCircleFilled(s.pos + ImVec2(-s.size * 0.25f, -s.size * 0.3f), s.size * 0.38f * t, Alpha(Pal::White, t));
            const float a = s.size * 2.6f * t;
            dl->AddLine(s.pos - ImVec2(a, 0.0f), s.pos + ImVec2(a, 0.0f), Alpha(col, 0.55f), PX(1.0f));
            dl->AddLine(s.pos - ImVec2(0.0f, a), s.pos + ImVec2(0.0f, a), Alpha(col, 0.55f), PX(1.0f));
        }
        i++;
    }
}
void SpawnPetals(const ImRect& r)
{
    g_petal_pool.clear();
    const int n = 22;
    for (int i = 0; i < n; i++)
    {
        Petal p;
        p.pos = ImVec2(r.Min.x - PX(30.0f) + Randf() * (r.GetWidth() + PX(60.0f)),
                       r.Min.y - PX(40.0f) + Randf() * (r.GetHeight() + PX(40.0f)));
        p.rot = Randf() * 360.0f;
        p.rotvel = (Randf() - 0.5f) * 40.0f;
        p.w = PX(2.5f) + PX(2.5f) * Randf();
        p.h = p.w * (1.4f + 0.6f * Randf());
        p.vy = PX(16.0f) + PX(26.0f) * Randf();
        p.sway = 0.6f + 1.4f * Randf();
        p.phase = Randf() * 6.2831853f;
        p.a = 0.18f + 0.22f * Randf();
        g_petal_pool.push_back(p);
    }
    g_petal_anchor = r.Min;
}
void UpdateAndDrawPetals(ImDrawList* dl, const ImRect& r)
{
    if (!g_petals) { g_petal_pool.clear(); return; }
    if (g_petal_pool.empty() || std::fabs(g_petal_anchor.x - r.Min.x) > PX(40.0f) ||
        std::fabs(g_petal_anchor.y - r.Min.y) > PX(40.0f))
        SpawnPetals(r);
    for (size_t i = 0; i < g_petal_pool.size(); i++)
    {
        Petal& p = g_petal_pool[i];
        const float sp = 0.35f + 1.65f * g_petal_spd;
        p.pos.y += p.vy * sp * g_dt;
        p.pos.x += std::sin(g_time * p.sway + p.phase) * PX(11.0f) * g_dt;
        p.rot += p.rotvel * g_dt;
        if (p.pos.y > r.Max.y + PX(20.0f) || p.pos.x < r.Min.x - PX(30.0f) || p.pos.x > r.Max.x + PX(30.0f))
        {
            p.pos = ImVec2(r.Min.x - PX(25.0f) + Randf() * (r.GetWidth() + PX(50.0f)), r.Min.y - PX(20.0f) - Randf() * PX(60.0f));
            p.a = 0.25f + 0.30f * Randf();
        }
        const float ca = std::cos(p.rot * 0.0174533f), sa = std::sin(p.rot * 0.0174533f);
        ImVec2 q[4];
        const float qx[4] = { 0.0f, p.w, 0.0f, -p.w };
        const float qy[4] = { -p.h, 0.0f, p.h, 0.0f };
        for (int k = 0; k < 4; k++)
            q[k] = ImVec2(p.pos.x + qx[k] * ca - qy[k] * sa, p.pos.y + qx[k] * sa + qy[k] * ca);
        const ImU32 col = Alpha(Mix(Pal::Pink, Pal::Lilac, 0.5f + 0.5f * std::sin(p.phase)), p.a);
        dl->AddQuadFilled(q[0], q[1], q[2], q[3], col);
        dl->AddLine(q[0], q[3], Alpha(Pal::White, p.a * 0.5f), PX(0.8f));
    }
}

} // namespace AnimeUI
