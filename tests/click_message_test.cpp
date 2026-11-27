#include "../src/runtime.cpp"
#include <iostream>
#include <vector>

std::vector<UINT> received;
LRESULT CALLBACK FixtureProc(HWND w,UINT msg,WPARAM wp,LPARAM lp){
    if(msg==WM_LBUTTONDOWN||msg==WM_LBUTTONUP||msg==WM_RBUTTONDOWN||msg==WM_RBUTTONUP){
        received.push_back(msg);return 0;
    }
    return DefWindowProcW(w,msg,wp,lp);
}
int main(){
    using namespace std::chrono_literals;
    const auto noteStart=NotificationAnimation::Clock::time_point{}+1s;
    const auto atStart=NotificationAnimation::Evaluate(noteStart,noteStart,noteStart+3s);
    const auto entering=NotificationAnimation::Evaluate(noteStart+100ms,noteStart,noteStart+3s);
    const auto visible=NotificationAnimation::Evaluate(noteStart+500ms,noteStart,noteStart+3s);
    const auto exiting=NotificationAnimation::Evaluate(noteStart+2900ms,noteStart,noteStart+3s);
    const auto finished=NotificationAnimation::Evaluate(noteStart+3s,noteStart,noteStart+3s);
    if(atStart.opacity!=0||atStart.offsetX<=0||entering.opacity<=0||entering.opacity>=1||visible.opacity!=1||visible.offsetX!=0||exiting.opacity<=0||exiting.opacity>=1||finished.opacity!=0){std::cerr<<"Notification animation timing failed\n";return 10;}
    if(LeftClickTriggered(false,true)||!LeftClickTriggered(true,true)||!LeftClickTriggered(false,false)){std::cerr<<"Hold mode failed\n";return 11;}
    leftEnabled=false;capturingBind=false;mouseBindWasDown=false;
    UpdateMouseBind(true,true);UpdateMouseBind(true,true);if(!leftEnabled){std::cerr<<"Mouse bind repeated or failed\n";return 12;}
    UpdateMouseBind(false,true);UpdateMouseBind(true,false);if(!leftEnabled){std::cerr<<"Mouse bind reacted outside game\n";return 13;}
    UpdateMouseBind(false,true);capturingBind=true;UpdateMouseBind(true,true);capturingBind=false;if(!leftEnabled)return 14;
    UpdateMouseBind(false,true);UpdateMouseBind(true,true);if(leftEnabled)return 15;
    capturingBind=true;WindowProc(nullptr,WM_XBUTTONDOWN,MAKEWPARAM(0,XBUTTON1),0);if(capturingBind||clickerKey!=VK_XBUTTON1||bindLabel!="Mouse 4")return 16;
    capturingBind=true;WindowProc(nullptr,WM_XBUTTONDOWN,MAKEWPARAM(0,XBUTTON2),0);if(capturingBind||clickerKey!=VK_XBUTTON2||bindLabel!="Mouse 5")return 17;
    RightClickGuard guard;const auto start=RightClickGuard::Clock::time_point{}+1s;const auto period=100ms;
    if(guard.Update(start,true,true,true,ItemUseState::Idle,period)!=RightClickAction::None||
       guard.Update(start+100ms,true,true,true,ItemUseState::Idle,period)!=RightClickAction::Down||
       guard.Update(start+130ms,true,true,true,ItemUseState::Active,period)!=RightClickAction::None||
       guard.Update(start+250ms,true,true,true,ItemUseState::Active,period)!=RightClickAction::None){std::cerr<<"Continuous use was interrupted\n";return 6;}
    guard.Reset();
    if(guard.Update(start,true,true,true,ItemUseState::Idle,period)!=RightClickAction::None||
       guard.Update(start+100ms,true,true,true,ItemUseState::Idle,period)!=RightClickAction::Down||
       guard.Update(start+150ms,true,true,true,ItemUseState::Idle,period)!=RightClickAction::None||
       guard.Update(start+170ms,true,true,true,ItemUseState::Idle,period)!=RightClickAction::Up){std::cerr<<"Right click was not released after safe probe\n";return 7;}
    guard.Reset();if(guard.Update(start,true,true,true,ItemUseState::Unknown,period)!=RightClickAction::None){std::cerr<<"Unknown item state was not handled safely\n";return 8;}
    guard.Reset();
    if(guard.Update(start,true,true,true,ItemUseState::Active,period)!=RightClickAction::Down||
       guard.Update(start+500ms,true,true,true,ItemUseState::Active,period)!=RightClickAction::None||
       guard.Update(start+550ms,true,true,true,ItemUseState::Idle,period)!=RightClickAction::None){std::cerr<<"Held-use transition failed\n";return 9;}
    std::mt19937 rng(1234);double smooth=0,targetRate=0;int remaining=0;bool initialized=false;
    for(int i=0;i<1000;++i){
        const auto interval=ClickTiming::Next<std::chrono::steady_clock>(10,14,rng,smooth,targetRate,remaining,initialized);
        const double seconds=std::chrono::duration<double>(interval).count();
        if(seconds+1e-8<1.0/14.0||seconds-1e-8>1.0/10.0){std::cerr<<"Click interval outside CPS range\n";return 5;}
    }
    for(float yaw : {-0.18f,0.0f,0.18f}) for(float pitch : {-0.14f,0.0f,0.14f}) {
        AnimeUI::g_tilt_yaw=yaw;AnimeUI::g_tilt_pitch=pitch;
        for(ImVec2 point : {ImVec2(100,100),ImVec2(410,270),ImVec2(700,450)}) {
            const float x=point.x-410.0f,y=point.y-270.0f,f=820.0f*1.8f;
            const float z=-x*std::sin(yaw),rz=y*std::sin(pitch)+z*std::cos(pitch);
            const float scale=f/(f-rz);
            const ImVec2 projected(410.0f+x*std::cos(yaw)*scale,270.0f+(y*std::cos(pitch)-z*std::sin(pitch))*scale);
            const ImVec2 restored=AnimeUI::UnprojectTiltMouse(projected);
            if(std::fabs(restored.x-point.x)>0.01f||std::fabs(restored.y-point.y)>0.01f){std::cerr<<"Tilt mouse projection mismatch\n";return 4;}
        }
    }
    HINSTANCE app=GetModuleHandleW(nullptr);WNDCLASSW wc{};wc.hInstance=app;wc.lpfnWndProc=FixtureProc;wc.lpszClassName=L"AutoClickV.HiddenClickTest";
    if(!RegisterClassW(&wc))return 1;
    game=CreateWindowW(wc.lpszClassName,L"test",WS_OVERLAPPED,0,0,100,100,nullptr,nullptr,app,nullptr);
    if(!game)return 2;
    Click(false);Click(true);Click(true,false);ReleaseRightClick();
    MSG msg{};while(PeekMessageW(&msg,game,0,0,PM_REMOVE))DispatchMessageW(&msg);
    const std::vector<UINT> expected={WM_LBUTTONDOWN,WM_LBUTTONUP,WM_RBUTTONDOWN,WM_RBUTTONUP,WM_RBUTTONDOWN,WM_RBUTTONUP};
    const bool ok=received==expected;
    DestroyWindow(game);UnregisterClassW(wc.lpszClassName,app);
    if(!ok){std::cerr<<"Unexpected mouse message sequence\n";return 3;}
    std::cout<<"PASS: isolated left and right mouse message sequence\n";return 0;
}

