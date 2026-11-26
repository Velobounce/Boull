#pragma once
#include <chrono>

enum class ItemUseState { Unknown, Idle, Active };
enum class RightClickAction { None, Down, Up };

class RightClickGuard {
public:
 using Clock=std::chrono::steady_clock;
 RightClickAction Update(Clock::time_point now,bool enabled,bool held,bool ready,ItemUseState item,Clock::duration interval){
  if(!enabled||!held||!ready||item==ItemUseState::Unknown){Reset();return RightClickAction::None;}
  if(item==ItemUseState::Active){
   if(continuous_)return RightClickAction::None;
   continuous_=true;
   if(pending_){pending_=false;return RightClickAction::None;}
   return RightClickAction::Down;
  }
  if(continuous_){continuous_=false;next_=now+interval;return RightClickAction::None;}
  if(pending_){if(now<releaseAt_)return RightClickAction::None;pending_=false;return RightClickAction::Up;}
  if(next_==Clock::time_point{}){next_=now+interval;return RightClickAction::None;}
  if(now<next_)return RightClickAction::None;
  pending_=true;next_=now+interval;releaseAt_=now+std::chrono::milliseconds(70);return RightClickAction::Down;
 }
 void Reset(){pending_=false;continuous_=false;next_={};releaseAt_={};}
private:
 bool pending_=false,continuous_=false;
 Clock::time_point next_{},releaseAt_{};
};
