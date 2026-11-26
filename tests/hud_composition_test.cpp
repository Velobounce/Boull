#include "../src/runtime.cpp"
#include <iostream>
#include <thread>
#include <atomic>
LRESULT CALLBACK HostProc(HWND w,UINT m,WPARAM wp,LPARAM lp){return DefWindowProcW(w,m,wp,lp);}
int main(){
 self=GetModuleHandleW(nullptr);WNDCLASSW wc{};wc.hInstance=self;wc.lpfnWndProc=HostProc;wc.lpszClassName=L"Boull.HudTest";
 if(!RegisterClassW(&wc))return 1;
 HWND host=CreateWindowExW(WS_EX_NOREDIRECTIONBITMAP,wc.lpszClassName,L"",WS_POPUP,0,0,820,540,nullptr,nullptr,self,nullptr);
 if(!host)return 2;
 if(!CreateGraphics(host)||!CreateHud()||!hudChain||!hudVisual||!hudCompositionTarget){std::cerr<<"HUD composition initialization failed\n";return 3;}
 if(!CreateLogoTexture()||AnimeUI::g_logoTexture==ImTextureID_Invalid){std::cerr<<"Logo texture creation failed\n";return 31;}
 if(moduleSettings.arrayOpacity!=40||moduleSettings.watermarkOpacity!=40||((HudBackground(40)>>IM_COL32_A_SHIFT)&0xff)!=102){std::cerr<<"Incorrect default HUD opacity\n";return 19;}
 ImGui::CreateContext();if(!ImGui_ImplDX11_Init(device.Get(),context.Get())||!InitializeHudImGui())return 8;
 bool toggle=false;AnimeUI::g_drag_widget=false;
 const auto frame=[&](bool down){
  ImGui::SetCurrentContext(menuImGuiContext);ImGui_ImplDX11_NewFrame();auto& io=ImGui::GetIO();
  io.DisplaySize=ImVec2(320,200);io.DeltaTime=1.0f/60.0f;io.AddMousePosEvent(50,50);io.AddMouseButtonEvent(0,down);
  ImGui::NewFrame();ImGui::SetNextWindowPos(ImVec2(0,0));ImGui::SetNextWindowSize(ImVec2(300,180));
  ImGui::Begin("menu fixture",nullptr,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize);
  ImGui::SetCursorPos(ImVec2(20,30));AnimeUI::ToggleRow("module",&toggle,200);ImGui::End();ImGui::Render();
  ImGui::SetCurrentContext(hudImGuiContext);ImGui_ImplDX11_NewFrame();auto& hudIo=ImGui::GetIO();hudIo.DisplaySize=ImVec2(800,600);hudIo.DeltaTime=1.0f/60.0f;
  ImGui::NewFrame();ImGui::GetBackgroundDrawList()->AddText(ImVec2(10,10),IM_COL32_WHITE,"Boull");ImGui::Render();
  ImGui::SetCurrentContext(menuImGuiContext);
 };
 frame(false);frame(true);frame(false);if(!toggle){std::cerr<<"Menu click lost while HUD frames were active\n";return 9;}
 bool left=false,right=false,delay=false,jump=false,capture=false,requestBind=false,requestUnload=false;int cps=8;
 AnimeUI::g_tilt_on=false;AnimeUI::g_tilt_follow=false;AnimeUI::g_tilt_orbit=false;AnimeUI::g_selectedModule=0;
 const auto menuFrame=[&](bool down){
  ImGui::SetCurrentContext(menuImGuiContext);ImGui_ImplDX11_NewFrame();auto& io=ImGui::GetIO();io.DisplaySize=ImVec2(820,540);io.DeltaTime=1.0f/60;io.ConfigInputTrickleEventQueue=false;
  io.AddMousePosEvent(440,180);io.AddMouseButtonEvent(1,down);ImGui::NewFrame();
  AnimeUI::RenderAutoClickMenu(left,right,delay,jump,true,true,true,cps,capture,"F6","",requestBind,requestUnload);ImGui::Render();
  ImGui::SetCurrentContext(hudImGuiContext);ImGui_ImplDX11_NewFrame();auto& hi=ImGui::GetIO();hi.DisplaySize=ImVec2(800,600);hi.DeltaTime=1.0f/60;ImGui::NewFrame();ImGui::Render();
  ImGui::SetCurrentContext(menuImGuiContext);
 };
 menuFrame(false);if(AnimeUI::g_selectedModule!=0)return 10;
 menuFrame(true);if(AnimeUI::g_selectedModule!=1||left){std::cerr<<"Right-click did not open module options independently\n";return 11;}
 menuFrame(false);menuFrame(true);if(AnimeUI::g_selectedModule!=0){std::cerr<<"Right-click did not close module options\n";return 12;}
 menuFrame(false);menuFrame(true);menuFrame(false);
 if(AnimeUI::g_selectedModule!=1)return 13;
 for(int i=0;i<25;++i)menuFrame(false);
 const auto hideFrame=[&](bool down){
  ImGui::SetCurrentContext(menuImGuiContext);ImGui_ImplDX11_NewFrame();auto& io=ImGui::GetIO();io.DisplaySize=ImVec2(820,540);io.DeltaTime=1.0f/60;io.ConfigInputTrickleEventQueue=false;
  io.AddMousePosEvent(560,460);io.AddMouseButtonEvent(0,down);ImGui::NewFrame();
  AnimeUI::RenderAutoClickMenu(left,right,delay,jump,true,true,true,cps,capture,"F6","",requestBind,requestUnload);ImGui::Render();
 };
 moduleSettings.hideLeftFromArray=false;hideFrame(false);hideFrame(true);hideFrame(false);
 if(!moduleSettings.hideLeftFromArray||moduleSettings.hideRightFromArray){std::cerr<<"Per-module ArrayList visibility control failed\n";return 14;}
 const auto uiFrame=[&](float x,float y,bool l,bool r){
  ImGui::SetCurrentContext(menuImGuiContext);ImGui_ImplDX11_NewFrame();auto& io=ImGui::GetIO();io.DisplaySize=ImVec2(820,540);io.DeltaTime=1.0f/60;io.ConfigInputTrickleEventQueue=false;
  io.AddMousePosEvent(x,y);io.AddMouseButtonEvent(0,l);io.AddMouseButtonEvent(1,r);ImGui::NewFrame();
  AnimeUI::RenderAutoClickMenu(left,right,delay,jump,true,true,true,cps,capture,"F6","",requestBind,requestUnload);ImGui::Render();
 };
 uiFrame(70,145,false,false);uiFrame(70,145,true,false);uiFrame(70,145,false,false);
 uiFrame(440,180,false,true);uiFrame(440,180,false,false);
 if(AnimeUI::g_selectedModule!=5){std::cerr<<"Visuals tab or ArrayList options failed\n";return 15;}
 for(int i=0;i<25;++i)uiFrame(440,180,false,false);
 moduleSettings.arrayShadow=true;uiFrame(560,265,true,false);uiFrame(560,265,false,false);
 if(moduleSettings.arrayShadow){std::cerr<<"ArrayList shadow control failed\n";return 22;}
 moduleSettings.hudTextScale=130;uiFrame(700,326,true,false);uiFrame(700,326,false,false);
 if(moduleSettings.hudTextScale==130||HudTextSize()!=std::floor(14.0f*moduleSettings.hudTextScale/100.0f+0.5f)){std::cerr<<"Shared text scale failed\n";return 23;}
 const int arrayScale=moduleSettings.hudTextScale;
 moduleSettings.arrayOpacity=80;moduleSettings.watermarkOpacity=80;
 uiFrame(580,430,true,false);uiFrame(580,430,false,false);
 if(moduleSettings.arrayOpacity==80||moduleSettings.watermarkOpacity!=80){std::cerr<<"ArrayList opacity is not independent\n";return 16;}
 const int arrayOpacity=moduleSettings.arrayOpacity;
 uiFrame(440,265,false,true);uiFrame(440,265,false,false);
 if(AnimeUI::g_selectedModule!=7){std::cerr<<"Watermark options failed\n";return 17;}
 if(moduleSettings.hudTextScale!=arrayScale){std::cerr<<"Watermark did not retain the shared scale\n";return 24;}
 uiFrame(580,358,true,false);uiFrame(580,358,false,false);
 if(moduleSettings.hudTextScale==arrayScale){std::cerr<<"Watermark could not change the shared scale\n";return 25;}
 uiFrame(700,492,true,false);uiFrame(700,492,false,false);
 if(moduleSettings.watermarkOpacity==80||moduleSettings.arrayOpacity!=arrayOpacity){std::cerr<<"Watermark opacity is not independent\n";return 18;}
 moduleSettings.watermarkFps=true;moduleSettings.watermarkShadow=true;
 uiFrame(560,225,true,false);uiFrame(560,225,false,false);
 uiFrame(560,310,true,false);uiFrame(560,310,false,false);
 if(moduleSettings.watermarkFps||moduleSettings.watermarkShadow){std::cerr<<"Watermark options failed to toggle\n";return 20;}
 const auto watermarkVertices=[&](bool showFps,bool shadow,int fps){
  moduleSettings.watermarkFps=showFps;moduleSettings.watermarkShadow=shadow;
  ImGui::SetCurrentContext(hudImGuiContext);ImGui_ImplDX11_NewFrame();auto& io=ImGui::GetIO();io.DisplaySize=ImVec2(800,600);io.DeltaTime=1.0f/60;ImGui::NewFrame();
  auto* draw=ImGui::GetBackgroundDrawList();const int before=draw->VtxBuffer.Size;
  DrawWatermark(draw,ImGui::GetFont(),io.DisplaySize,IM_COL32_WHITE,fps);const int count=draw->VtxBuffer.Size-before;ImGui::Render();return count;
 };
 const int plain=watermarkVertices(false,false,144);
 const int withFps=watermarkVertices(true,false,144);
 const int withShadow=watermarkVertices(false,true,144);
 const int missingFps=watermarkVertices(true,false,-1);
 if(withFps<=plain||withShadow<=plain||missingFps!=plain){std::cerr<<"Watermark FPS or shadow drawing failed\n";return 21;}
 moduleSettings.rightGuardAvailable=true;
 uiFrame(70,97,false,false);uiFrame(70,97,true,false);uiFrame(70,97,false,false);
 uiFrame(650,180,false,true);uiFrame(650,180,false,false);
 if(AnimeUI::g_selectedModule!=2){std::cerr<<"Second-column RightClicker was not selectable\n";return 26;}
 for(int i=0;i<25;++i)uiFrame(350,220,false,false);
 uiFrame(350,220,false,true);uiFrame(350,220,false,false);
 if(AnimeUI::g_selectedModule!=0){std::cerr<<"Animated RightClicker did not close\n";return 27;}
 for(int i=0;i<30;++i)uiFrame(350,265,false,false);
 uiFrame(350,265,false,true);uiFrame(350,265,false,false);
 if(AnimeUI::g_selectedModule!=3){std::cerr<<"Second-row HitDelayFix was not selectable\n";return 28;}
 uiFrame(45,482,false,false);uiFrame(45,482,true,false);uiFrame(45,482,false,false);
 if(requestUnload){std::cerr<<"Gear unloaded immediately\n";return 29;}
 uiFrame(65,425,false,false);uiFrame(65,425,true,false);uiFrame(65,425,false,false);
 if(!requestUnload){std::cerr<<"Unload popup did not respond\n";return 30;}
 AnimeUI::g_logoTexture=ImTextureID_Invalid;logoTexture.Reset();
 ImGui::SetCurrentContext(hudImGuiContext);ImGui_ImplDX11_Shutdown();ImGui::DestroyContext(hudImGuiContext);hudImGuiContext=nullptr;
 ImGui::SetCurrentContext(menuImGuiContext);ImGui_ImplDX11_Shutdown();ImGui::DestroyContext(menuImGuiContext);menuImGuiContext=nullptr;
 const auto exStyle=GetWindowLongPtrW(hudWindow,GWL_EXSTYLE);
 if(!(exStyle&WS_EX_LAYERED)||!(exStyle&WS_EX_TRANSPARENT)||!(exStyle&WS_EX_NOACTIVATE)){std::cerr<<"HUD input styles missing\n";return 4;}
 std::atomic<HWND> below{};std::atomic<DWORD> belowThread{};HANDLE ready=CreateEventW(nullptr,TRUE,FALSE,nullptr);if(!ready)return 5;
 std::thread worker([&]{
  belowThread=GetCurrentThreadId();
  HWND w=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE,wc.lpszClassName,L"",WS_POPUP,20,20,100,100,nullptr,nullptr,self,nullptr);
  below=w;if(w)SetWindowPos(w,HWND_TOPMOST,20,20,100,100,SWP_NOACTIVATE|SWP_SHOWWINDOW);
  SetEvent(ready);MSG msg{};while(GetMessageW(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}if(w)DestroyWindow(w);
 });
 WaitForSingleObject(ready,5000);CloseHandle(ready);
 if(!below){PostThreadMessageW(belowThread,WM_QUIT,0,0);worker.join();return 6;}
 SetWindowPos(hudWindow,HWND_TOPMOST,20,20,100,100,SWP_NOACTIVATE|SWP_SHOWWINDOW);
 RECT targetRect{};GetWindowRect(below,&targetRect);POINT probe{targetRect.left+30,targetRect.top+30};const HWND hit=WindowFromPoint(probe);
 ShowWindow(hudWindow,SW_HIDE);PostThreadMessageW(belowThread,WM_QUIT,0,0);worker.join();
 if(hit==hudWindow){std::cerr<<"HUD intercepted cross-thread hit test\n";return 7;}
 if(hit!=below)std::cout<<"SKIP: cross-thread hit test was obscured by another window\n";
 DestroyWindow(hudWindow);hudTarget.Reset();hudVisual.Reset();hudCompositionTarget.Reset();hudChain.Reset();UnregisterClassW(L"Boull.Hud",self);
 target.Reset();compositionVisual.Reset();compositionTarget.Reset();composition.Reset();swapChain.Reset();context.Reset();device.Reset();DestroyWindow(host);UnregisterClassW(wc.lpszClassName,self);
 std::cout<<"PASS: HUD composition and menu clicks with HUD frames\n";return 0;
}


