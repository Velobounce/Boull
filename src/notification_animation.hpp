#pragma once
#include <algorithm>
#include <chrono>

namespace NotificationAnimation {
using Clock=std::chrono::steady_clock;
struct Frame { float opacity; float offsetX; };
inline float EaseOut(float value){const float t=(std::clamp)(value,0.0f,1.0f);const float rest=1.0f-t;return 1.0f-rest*rest*rest;}
inline Frame Evaluate(Clock::time_point now,Clock::time_point created,Clock::time_point until){
 const float age=std::chrono::duration<float>(now-created).count();
 const float remaining=std::chrono::duration<float>(until-now).count();
 const float entering=EaseOut(age/0.26f);
 const float exiting=EaseOut(remaining/0.32f);
 const float opacity=(std::min)(entering,exiting);
 return {opacity,36.0f*(1.0f-opacity)};
}
}
