#pragma once

#include <imgui.h>
#include <algorithm>
#include <cfloat>
#include <cmath>
#include "HudTextMetrics.h"

// Shared drawing primitives, also used by the offscreen visual regression fixture.
namespace HudCards {
    inline ImU32 Alpha(int r, int g, int b, int a, float opacity = 1.0f) {
        return IM_COL32(r, g, b, static_cast<int>(a * (std::clamp)(opacity, 0.0f, 1.0f)));
    }

    inline float TextWidth(ImFont* font, float size, const char* text) {
        return text && *text ? font->CalcTextSizeA(size, FLT_MAX, 0, text).x : 0;
    }

    inline void ShadowedText(ImDrawList* draw, ImFont* font, float size, ImVec2 pos,
                             ImU32 color, const char* value, bool shadow) {
        if (shadow) {
            draw->AddText(font, size, ImVec2(pos.x + 1, pos.y + 1), Alpha(0, 0, 0, 125), value);
        }
        draw->AddText(font, size, pos, color, value);
    }

    inline ImU32 ArrayListTagColor() { return Alpha(200, 200, 206, 255); }

    struct ArrayListRowLayout {
        ImVec2 min;
        ImVec2 max;
        ImVec2 text;
        float advance;
        float rounding;
    };

    inline ArrayListRowLayout ModernArrayListRowLayout(float right, float top,
                                                       float textWidth, float fontSize) {
        const float unit = fontSize / 14.0f;
        const float padding = 9.0f * unit;
        const float height = 24.0f * unit;
        return {ImVec2(right - textWidth - 2 * padding, top),
            ImVec2(right, top + height),
            ImVec2(right - textWidth - padding, top + (height - fontSize) * 0.5f),
            height + 2.0f * unit, 4.0f * unit};
    }

    inline void ModernArrayListBackground(ImDrawList* draw, const ArrayListRowLayout& row,
                                         ImU32 background = Alpha(14, 17, 23, 128)) {
        // A single tint; optional backdrop blur is drawn separately behind it.
        draw->AddRectFilled(row.min, row.max, background, row.rounding);
    }

    inline void Panel(ImDrawList* draw, ImVec2 min, ImVec2 max, ImU32 background,
                      float rounding, float opacity = 1.0f) {
        for (int i = 3; i > 0; --i) {
            const float spread = static_cast<float>(i);
            draw->AddRectFilled(ImVec2(min.x - spread, min.y + 1 - spread),
                ImVec2(max.x + spread, max.y + 2 + spread),
                Alpha(0, 0, 0, 12, opacity), rounding + spread);
        }
        draw->AddRectFilled(min, max, background, rounding);
    }

    inline void ToggleNotification(ImDrawList* draw, ImFont* font, ImVec2 min,
                                   float width, float height, const char* name,
                                   const char* state, bool enabled,
                                   float remaining, float opacity = 1.0f) {
        const ImVec2 max(min.x + width, min.y + height);
        Panel(draw, min, max, Alpha(9, 10, 12, 205, opacity), 6, opacity);
        const ImU32 status = enabled ? Alpha(89, 221, 109, 255, opacity)
                                    : Alpha(245, 96, 101, 255, opacity);
        const float barWidth = (width - 10) * (std::clamp)(remaining, 0.0f, 1.0f);
        if (barWidth > 0.5f) {
            draw->AddRectFilledMultiColor(ImVec2(min.x + 5, min.y),
                ImVec2(min.x + 5 + barWidth, min.y + 3),
                Alpha(255, 255, 255, 255, opacity), status, status,
                Alpha(255, 255, 255, 255, opacity));
        }
        const float fontSize = 14;
        const float stateWidth = TextWidth(font, fontSize, state);
        const float textY = std::floor(min.y + (height - fontSize) * 0.5f + 1);
        const float stateX = max.x - 11 - stateWidth;
        draw->PushClipRect(ImVec2(min.x + 10, min.y + 4), ImVec2(stateX - 12, max.y - 4), true);
        draw->AddText(font, fontSize, ImVec2(min.x + 10, textY),
            Alpha(237, 237, 241, 255, opacity), name);
        draw->PopClipRect();
        draw->AddText(font, fontSize, ImVec2(stateX, textY), status, state);
    }

    inline void KeyboardIcon(ImDrawList* draw, ImVec2 p, float scale, ImU32 color) {
        draw->AddRect(p, ImVec2(p.x + 12 * scale, p.y + 9 * scale), color, 2 * scale, 0, scale);
        for (int row = 0; row < 2; ++row)
            for (int column = 0; column < 4; ++column) {
                const ImVec2 key(p.x + (2.2f + column * 2.4f) * scale,
                                 p.y + (2.3f + row * 2.2f) * scale);
                draw->AddRectFilled(key, ImVec2(key.x + scale, key.y + scale), color);
            }
        draw->AddLine(ImVec2(p.x + 3.5f * scale, p.y + 7 * scale),
                      ImVec2(p.x + 8.5f * scale, p.y + 7 * scale), color, scale);
    }

    inline void KeybindHeader(ImDrawList* draw, ImFont* font, ImVec2 min,
                             float width, float height, float scale,
                             const char* title, float opacity,
                             float backgroundOpacity = 1.0f, bool shadow = true) {
        const ImVec2 max(min.x + width, min.y + height);
        const float backgroundAlpha = opacity * (std::clamp)(backgroundOpacity, 0.0f, 1.0f);
        if (shadow) Panel(draw, min, max, Alpha(12, 11, 14, 210, backgroundAlpha), 6 * scale, backgroundAlpha);
        else draw->AddRectFilled(min, max, Alpha(12, 11, 14, 210, backgroundAlpha), 6 * scale);
        draw->AddRectFilled(min, ImVec2(max.x, min.y + 26 * scale),
            Alpha(0, 0, 0, 72, backgroundAlpha), 6 * scale, ImDrawFlags_RoundCornersTop);
        KeyboardIcon(draw, ImVec2(min.x + 12 * scale, min.y + 8 * scale), scale,
                     Alpha(220, 217, 224, 255, opacity));
        const float size = 14 * scale;
        draw->AddText(font, size,
            ImVec2(min.x + (width - TextWidth(font, size, title)) * 0.5f, min.y + 5 * scale),
            Alpha(228, 225, 233, 255, opacity), title);
        draw->AddLine(ImVec2(min.x + 8 * scale, min.y + 26 * scale),
            ImVec2(max.x - 8 * scale, min.y + 26 * scale), Alpha(255, 255, 255, 16, backgroundAlpha));
    }

    inline void KeybindRow(ImDrawList* draw, ImFont* font, ImVec2 p,
                          float width, float scale, const char* name,
                          const char* key, bool enabled, float opacity) {
        const float size = 13 * scale;
        const float keyX = p.x + width - 12 * scale - TextWidth(font, size, key);
        const ImU32 color = enabled ? Alpha(224, 221, 230, 255, opacity)
                                   : Alpha(153, 149, 162, 255, opacity);
        draw->PushClipRect(ImVec2(p.x + 11 * scale, p.y),
            ImVec2((std::max)(p.x + 11 * scale, keyX - 10 * scale), p.y + 20 * scale), true);
        draw->AddText(font, size, ImVec2(p.x + 11 * scale, p.y), color, name);
        draw->PopClipRect();
        draw->AddText(font, size, ImVec2(keyX, p.y), color, key);
    }

    inline ImVec2 WatermarkSize(ImFont* font, float size, const char* title,
                                const char* fps, const char* nickname,
                                HudTextMetrics::Cache* metrics = nullptr) {
        const auto widthOf = [&](size_t slot, const char* text) {
            return metrics ? metrics->Width(slot, font, size, text) : TextWidth(font, size, text);
        };
        float width = widthOf(0, title) + 24;
        size_t slot = 1;
        for (const char* detail : {fps, nickname}) {
            if (detail && *detail) width += 20 + widthOf(slot, detail);
            ++slot;
        }
        return ImVec2(width, size + 18);
    }

    inline ImVec2 WatermarkPosition(ImVec2 viewport, ImVec2 bounds, int x, int y) {
        // Percentages refer to available space, so 50 centers the complete pill
        // even when its title, FPS value or scale changes. Keep a six-pixel inset.
        return ImVec2(
            std::floor(6 + (std::max)(0.0f, viewport.x - bounds.x - 12) *
                (std::clamp)(x, 0, 100) / 100.0f),
            std::floor(6 + (std::max)(0.0f, viewport.y - bounds.y - 12) *
                (std::clamp)(y, 0, 100) / 100.0f));
    }

    inline ImVec2 InventorySize(float scale) { return ImVec2(216 * scale, 110 * scale); }

    inline void InventoryPanel(ImDrawList* draw, ImFont* font, ImVec2 min, float scale,
                               float opacity, const char* title) {
        const ImVec2 size = InventorySize(scale);
        const ImVec2 max(min.x + size.x, min.y + size.y);
        draw->AddRectFilled(min, max, Alpha(12, 13, 17, 215, opacity), 6 * scale);
        draw->AddRectFilled(min, ImVec2(max.x, min.y + 26 * scale),
            Alpha(0, 0, 0, 65, opacity), 6 * scale, ImDrawFlags_RoundCornersTop);
        draw->AddText(font, 14 * scale, ImVec2(min.x + 11 * scale, min.y + 5 * scale),
            Alpha(228, 225, 233, 255, opacity), title);
        draw->AddLine(ImVec2(min.x + 9 * scale, min.y + 26 * scale),
            ImVec2(max.x - 9 * scale, min.y + 26 * scale), Alpha(255, 255, 255, 18, opacity));
        for (int row = 0; row < 3; ++row)
            for (int column = 0; column < 9; ++column) {
                const ImVec2 slot(min.x + (9 + column * 22) * scale,
                    min.y + (35 + row * 22) * scale);
                draw->AddRectFilled(slot, ImVec2(slot.x + 20 * scale, slot.y + 20 * scale),
                    Alpha(255, 255, 255, 10, opacity), 3 * scale);
            }
    }

    inline void Watermark(ImDrawList* draw, ImFont* font, ImVec2 min, float size,
                          const char* title, const char* fps, const char* nickname,
                          ImU32 titleColor, bool background, ImU32 backgroundColor,
                          bool shadow, HudTextMetrics::Cache* metrics = nullptr) {
        const ImVec2 bounds = WatermarkSize(font, size, title, fps, nickname, metrics);
        // The real backdrop blur is drawn by HUD before this tint. No stacked
        // rectangles or outer shadow: the silhouette is a single clean pill.
        if (background) draw->AddRectFilled(min, ImVec2(min.x + bounds.x, min.y + bounds.y),
            backgroundColor, bounds.y * 0.5f);
        float x = min.x + 12;
        const float y = std::floor(min.y + 9);
        const auto text = [&](size_t slot, const char* value, ImU32 color) {
            ShadowedText(draw,font,size,ImVec2(x,y),color,value,shadow);
            x += metrics ? metrics->Width(slot, font, size, value) : TextWidth(font, size, value);
        };
        text(0, title, titleColor);
        size_t slot = 0;
        for (const char* detail : {fps, nickname}) {
            ++slot;
            if (!detail || !*detail) continue;
            draw->AddCircleFilled(ImVec2(x + 10, min.y + bounds.y * 0.5f), 1.8f,
                                  Alpha(167, 171, 186, 220), 12);
            x += 20;
            text(slot, detail, Alpha(234, 235, 242, 255));
        }
    }
}

