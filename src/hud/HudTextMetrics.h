#pragma once
#include <imgui.h>
#include <cstdint>
#include <array>
#include <cfloat>

namespace HudTextMetrics {
    // Cache numeric measurements, never readable module names or key labels.
    // Text/font/size changes invalidate a slot without delaying UI updates.
    class Cache {
    public:
        float Width(size_t slot, ImFont* font, float size, const char* text) {
            if (!font || !text || !*text) return 0;
            auto& entry = entries[slot % entries.size()];
            std::uint64_t hash=1469598103934665603ull;
            for(const unsigned char* p=reinterpret_cast<const unsigned char*>(text);*p;++p)hash=(hash^*p)*1099511628211ull;
            const auto* context = ImGui::GetCurrentContext();
            if (!entry.valid || entry.hash != hash || entry.font != font ||
                entry.size != size || entry.context != context) {
                entry = {hash, font, context, size, font->CalcTextSizeA(size, FLT_MAX, 0, text).x, true};
#ifdef HUD_METRICS_TEST_STATS
                ++measurements;
#endif
            }
            return entry.width;
        }
        void Reset() { entries = {}; }
#ifdef HUD_METRICS_TEST_STATS
        unsigned Measurements() const { return measurements; }
#endif
    private:
        struct Entry {
            std::uint64_t hash = 0;
            ImFont* font = nullptr;
            const ImGuiContext* context = nullptr;
            float size = 0, width = 0;
            bool valid = false;
        };
        std::array<Entry, 128> entries{};
#ifdef HUD_METRICS_TEST_STATS
        unsigned measurements = 0;
#endif
    };
}

