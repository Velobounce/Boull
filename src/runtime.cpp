#include <windows.h>
#include <jni.h>
#include <jvmti.h>
#include <d3d11.h>
#include <dcomp.h>
#include <windowsx.h>
#include <wrl/client.h>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <array>
#include <chrono>
#include <string>
#include <random>
#include <deque>
#include "click_timing.hpp"
#include "right_click_guard.hpp"
#include "notification_animation.hpp"
#include "module_settings.hpp"
#include "offline_session.hpp"
#include "hud/HudCards.h"
#include "hud/inter_regular_font.h"
#include "assets/logo_rgba.hpp"
#include "sakura/ui.hpp"

namespace AnimeUI { void RenderAutoClickMenu(bool&,bool&,bool&,bool&,bool,bool,bool,int&,bool&,const std::string&,const std::string&,bool&,bool&); ImVec2 UnprojectTiltMouse(ImVec2); }

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND,UINT,WPARAM,LPARAM);
namespace {
HMODULE self{}; HWND panel{}, game{};
HWND hudWindow{};
bool hudReady=false;
ImGuiContext* menuImGuiContext{};
ImGuiContext* hudImGuiContext{};
bool capturingBind=false, hotkeyRegistered=false, leftEnabled=false, rightEnabled=false, delayEnabled=false, jumpEnabled=false, unloadRequested=false;
bool mapped=false; int cps=8; UINT clickerKey=VK_F6;
std::string bindLabel="F6", statusText="AutoClicker keybind";
Microsoft::WRL::ComPtr<ID3D11Device> device;
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> logoTexture;
Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
Microsoft::WRL::ComPtr<IDCompositionDevice> composition;
Microsoft::WRL::ComPtr<IDCompositionTarget> compositionTarget;
Microsoft::WRL::ComPtr<IDCompositionVisual> compositionVisual;
Microsoft::WRL::ComPtr<ID3D11RenderTargetView> target;
Microsoft::WRL::ComPtr<IDXGISwapChain1> hudChain;
Microsoft::WRL::ComPtr<IDCompositionTarget> hudCompositionTarget;
Microsoft::WRL::ComPtr<IDCompositionVisual> hudVisual;
Microsoft::WRL::ComPtr<ID3D11RenderTargetView> hudTarget;
int hudWidth=0,hudHeight=0;
struct Notification {std::string text;std::chrono::steady_clock::time_point created,until;float renderY=0;bool positioned=false;};
std::deque<Notification> hudNotifications;
JavaVM* vm{}; JNIEnv* env{}; jclass minecraft{}; jfieldID instance{}, world{}, player{}, screen{}, delay{};
jclass jumpPlayerClass{}; jfieldID jumpTicks{}; bool obfuscatedMapping=false;
jclass usePlayerClass{};jfieldID itemInUseField{};
jfieldID debugFpsField{};int gameFps=-1;
std::chrono::steady_clock::time_point nextLeft{};
RightClickGuard rightClickGuard;
std::mt19937 clickRng{std::random_device{}()};
double smoothCps=0,targetCps=0;int targetClicks=0;bool cpsInitialized=false;
std::chrono::steady_clock::time_point nextMappingAttempt{};
std::chrono::steady_clock::time_point nextJavaCheck{};
std::chrono::steady_clock::time_point nextFpsCheck{};
std::chrono::steady_clock::time_point nextItemUseCheck{};
ItemUseState cachedItemUse=ItemUseState::Unknown;
bool cachedWorldReady=false;
struct Mapping {const char* cls; const char* singleton; const char* world; const char* worldSig; const char* player; const char* playerSig; const char* screen; const char* screenSig; const char* delay;};
constexpr Mapping mappings[]={
 {"Lnet/minecraft/client/Minecraft;","theMinecraft","theWorld","Lnet/minecraft/client/multiplayer/WorldClient;","thePlayer","Lnet/minecraft/client/entity/EntityPlayerSP;","currentScreen","Lnet/minecraft/client/gui/GuiScreen;","leftClickCounter"},
 {"Lave;","S","f","Lbdb;","h","Lbew;","m","Laxu;","ag"}
};
bool ClearException(){if(!env->ExceptionCheck())return false;env->ExceptionClear();return true;}
void ResetJumpMapping(){if(jumpPlayerClass&&env)env->DeleteGlobalRef(jumpPlayerClass);jumpPlayerClass=nullptr;jumpTicks=nullptr;}
void ResetUseMapping(){if(usePlayerClass&&env)env->DeleteGlobalRef(usePlayerClass);usePlayerClass=nullptr;itemInUseField=nullptr;moduleSettings.rightGuardAvailable=false;cachedItemUse=ItemUseState::Unknown;rightClickGuard.Reset();}
void ResetFields(){OriginalSession::Reset(env);ResetJumpMapping();ResetUseMapping();if(minecraft&&env)env->DeleteGlobalRef(minecraft);minecraft=nullptr;instance=world=player=screen=delay=debugFpsField=nullptr;gameFps=-1;}
bool TryMapping(jclass cls,const Mapping& m) {
    const std::string singletonSig=m.cls;
    const auto getStatic=[&](const char* n,const char* s){auto f=env->GetStaticFieldID(cls,n,s);if(ClearException())return static_cast<jfieldID>(nullptr);return f;};
    const auto get=[&](const char* n,const char* s){auto f=env->GetFieldID(cls,n,s);if(ClearException())return static_cast<jfieldID>(nullptr);return f;};
    jfieldID i=getStatic(m.singleton,singletonSig.c_str()),w=get(m.world,m.worldSig),p=get(m.player,m.playerSig),s=get(m.screen,m.screenSig),d=get(m.delay,"I");
    if(!i||!w||!p||!s||!d)return false;
    auto ref=static_cast<jclass>(env->NewGlobalRef(cls)); if(!ref || ClearException())return false;
    ResetFields();minecraft=ref;instance=i;world=w;player=p;screen=s;delay=d;obfuscatedMapping=(&m==&mappings[1]);
    if(obfuscatedMapping)debugFpsField=getStatic("ao","I");
    else {debugFpsField=getStatic("debugFPS","I");if(!debugFpsField)debugFpsField=getStatic("field_71470_ab","I");}
    return true;
}
void RefreshJumpMapping(){
 if(!env||!minecraft)return;
 jobject mc=env->GetStaticObjectField(minecraft,instance);if(ClearException()||!mc)return;
 jobject p=env->GetObjectField(mc,player);const bool failed=ClearException();env->DeleteLocalRef(mc);
 if(failed||!p)return;
 if(jumpPlayerClass&&env->IsInstanceOf(p,jumpPlayerClass)){env->DeleteLocalRef(p);return;}
 ResetJumpMapping();jclass cls=env->GetObjectClass(p);env->DeleteLocalRef(p);if(ClearException()||!cls)return;
 const char* namesNormal[]={"jumpTicks","field_70773_bE"};const char* namesObfuscated[]={"bn"};
 const auto find=[&](const char* name){jfieldID f=env->GetFieldID(cls,name,"I");if(ClearException())return static_cast<jfieldID>(nullptr);return f;};
 if(obfuscatedMapping){for(const char* name:namesObfuscated)if((jumpTicks=find(name)))break;}
 else {for(const char* name:namesNormal)if((jumpTicks=find(name)))break;}
 if(jumpTicks){jumpPlayerClass=static_cast<jclass>(env->NewGlobalRef(cls));if(ClearException()||!jumpPlayerClass)jumpTicks=nullptr;}
 env->DeleteLocalRef(cls);
}
void RefreshUseMapping(){
 if(!env||!minecraft)return;
 jobject mc=env->GetStaticObjectField(minecraft,instance);if(ClearException()||!mc)return;
 jobject p=env->GetObjectField(mc,player);const bool failed=ClearException();env->DeleteLocalRef(mc);
 if(failed||!p)return;
 if(usePlayerClass&&env->IsInstanceOf(p,usePlayerClass)){env->DeleteLocalRef(p);return;}
 ResetUseMapping();jclass cls=env->GetObjectClass(p);env->DeleteLocalRef(p);if(ClearException()||!cls)return;
 const char* signature=obfuscatedMapping?"Lzx;":"Lnet/minecraft/item/ItemStack;";
 const char* namesNormal[]={"itemInUse","field_71074_e"};const char* namesObfuscated[]={"g"};
 const auto find=[&](const char* name){jfieldID field=env->GetFieldID(cls,name,signature);if(ClearException())return static_cast<jfieldID>(nullptr);return field;};
 if(obfuscatedMapping){for(const char* name:namesObfuscated)if((itemInUseField=find(name)))break;}
 else {for(const char* name:namesNormal)if((itemInUseField=find(name)))break;}
 if(itemInUseField){usePlayerClass=static_cast<jclass>(env->NewGlobalRef(cls));if(ClearException()||!usePlayerClass)itemInUseField=nullptr;}
 moduleSettings.rightGuardAvailable=itemInUseField!=nullptr;env->DeleteLocalRef(cls);
}
ItemUseState ReadItemUseState(){
 if(!env||!minecraft||!itemInUseField||!usePlayerClass)return ItemUseState::Unknown;
 jobject mc=env->GetStaticObjectField(minecraft,instance);if(ClearException()||!mc)return ItemUseState::Unknown;
 jobject p=env->GetObjectField(mc,player);bool error=ClearException();env->DeleteLocalRef(mc);
 if(error||!p)return ItemUseState::Unknown;
 if(!env->IsInstanceOf(p,usePlayerClass)){env->DeleteLocalRef(p);return ItemUseState::Unknown;}
 jobject item=env->GetObjectField(p,itemInUseField);error=ClearException();env->DeleteLocalRef(p);
 if(error)return ItemUseState::Unknown;
 const ItemUseState state=item?ItemUseState::Active:ItemUseState::Idle;if(item)env->DeleteLocalRef(item);return state;
}
bool FindMinecraft() {
    if(!env||!vm)return false; jvmtiEnv* ti=nullptr;
    if(vm->GetEnv(reinterpret_cast<void**>(&ti),JVMTI_VERSION_1_2)!=JNI_OK||!ti)return false;
    jint count=0; jclass* classes=nullptr; if(ti->GetLoadedClasses(&count,&classes)!=JVMTI_ERROR_NONE||!classes)return false;
    bool found=false;
    for(jint i=0;i<count;++i){char* sig=nullptr; if(!found&&ti->GetClassSignature(classes[i],&sig,nullptr)==JVMTI_ERROR_NONE&&sig){for(const auto& m:mappings)if(std::string(sig)==m.cls&&TryMapping(classes[i],m)){found=true;break;}}if(sig)ti->Deallocate(reinterpret_cast<unsigned char*>(sig));env->DeleteLocalRef(classes[i]);}
    ti->Deallocate(reinterpret_cast<unsigned char*>(classes));return found;
}
bool AttachJvm(){HMODULE jvm=GetModuleHandleW(L"jvm.dll");if(!jvm)return false;using GetVMs=jint(JNICALL*)(JavaVM**,jsize,jsize*);auto get=reinterpret_cast<GetVMs>(GetProcAddress(jvm,"JNI_GetCreatedJavaVMs"));jsize count=0;if(!get||get(&vm,1,&count)!=JNI_OK||count!=1||!vm)return false;return vm->AttachCurrentThread(reinterpret_cast<void**>(&env),nullptr)==JNI_OK;}
void ResetDelay(){
 if(!minecraft||!env)return; jobject mc=env->GetStaticObjectField(minecraft,instance); if(ClearException()||!mc)return;
 jobject w=env->GetObjectField(mc,world);bool error=ClearException(); jobject p=error?nullptr:env->GetObjectField(mc,player);error=ClearException()||error;
 jobject s=error?nullptr:env->GetObjectField(mc,screen);error=ClearException()||error;
 if(!error&&w&&p&&!s){env->SetIntField(mc,delay,0);ClearException();}
 if(s)env->DeleteLocalRef(s);if(p)env->DeleteLocalRef(p);if(w)env->DeleteLocalRef(w);env->DeleteLocalRef(mc);
}
void ResetJumpDelay(){
 if(!minecraft||!env||!jumpTicks||!jumpPlayerClass)return;
 jobject mc=env->GetStaticObjectField(minecraft,instance);if(ClearException()||!mc)return;
 jobject w=env->GetObjectField(mc,world);bool error=ClearException();
 jobject p=error?nullptr:env->GetObjectField(mc,player);error=ClearException()||error;
 jobject s=error?nullptr:env->GetObjectField(mc,screen);error=ClearException()||error;
 if(!error&&w&&p&&!s&&env->IsInstanceOf(p,jumpPlayerClass)){env->SetIntField(p,jumpTicks,0);ClearException();}
 if(s)env->DeleteLocalRef(s);if(p)env->DeleteLocalRef(p);if(w)env->DeleteLocalRef(w);env->DeleteLocalRef(mc);
}
void Click(bool right,bool releaseAfterDown=true){POINT pt{};if(!GetCursorPos(&pt)||!ScreenToClient(game,&pt))return;const LPARAM pos=MAKELPARAM(static_cast<short>(pt.x),static_cast<short>(pt.y));PostMessageW(game,right?WM_RBUTTONDOWN:WM_LBUTTONDOWN,right?MK_RBUTTON:MK_LBUTTON,pos);if(releaseAfterDown)PostMessageW(game,right?WM_RBUTTONUP:WM_LBUTTONUP,0,pos);}
void ReleaseRightClick(){POINT pt{};if(!GetCursorPos(&pt)||!ScreenToClient(game,&pt))return;PostMessageW(game,WM_RBUTTONUP,0,MAKELPARAM(static_cast<short>(pt.x),static_cast<short>(pt.y)));}
bool IsWorldReady(){
 if(!env||!minecraft)return false;
 jobject mc=env->GetStaticObjectField(minecraft,instance);if(ClearException()||!mc)return false;
 jobject w=env->GetObjectField(mc,world);bool error=ClearException();
 jobject p=error?nullptr:env->GetObjectField(mc,player);error=ClearException()||error;
 jobject s=error?nullptr:env->GetObjectField(mc,screen);error=ClearException()||error;
 const bool ready=!error&&w&&p&&!s;
 if(s)env->DeleteLocalRef(s);if(p)env->DeleteLocalRef(p);if(w)env->DeleteLocalRef(w);env->DeleteLocalRef(mc);return ready;
}
BOOL CALLBACK FindGame(HWND window,LPARAM){if(!IsWindowVisible(window))return TRUE;DWORD pid=0;GetWindowThreadProcessId(window,&pid);if(pid!=GetCurrentProcessId())return TRUE;wchar_t cls[128]{};if(GetClassNameW(window,cls,128)&&(wcsstr(cls,L"LWJGL")||wcsstr(cls,L"GLFW"))){game=window;return FALSE;}return TRUE;}
void Tick(){const auto now=std::chrono::steady_clock::now();
 if(!mapped&&env&&now>=nextMappingAttempt){mapped=FindMinecraft();nextMappingAttempt=now+std::chrono::seconds(5);}
 if(mapped&&now>=nextMappingAttempt){RefreshJumpMapping();RefreshUseMapping();nextMappingAttempt=now+std::chrono::seconds(5);}
 if(moduleSettings.watermark&&debugFpsField&&env&&minecraft&&now>=nextFpsCheck){const jint value=env->GetStaticIntField(minecraft,debugFpsField);if(!ClearException())gameFps=value>=0&&value<10000?value:-1;nextFpsCheck=now+std::chrono::seconds(1);}
 if(!game||!IsWindow(game)){game=nullptr;EnumWindows(FindGame,0);}
 if(!game||GetForegroundWindow()!=game){nextLeft={};rightClickGuard.Reset();cpsInitialized=false;cachedWorldReady=false;cachedItemUse=ItemUseState::Unknown;nextJavaCheck={};nextItemUseCheck={};return;}
 const bool heldLeft=(GetAsyncKeyState(VK_LBUTTON)&0x8000)!=0;
 const bool heldRight=(GetAsyncKeyState(VK_RBUTTON)&0x8000)!=0;
 const bool needWorld=(leftEnabled&&heldLeft&&moduleSettings.inventory)||(rightEnabled&&heldRight);
 if(now>=nextJavaCheck){cachedWorldReady=IsWorldReady();nextJavaCheck=now+std::chrono::milliseconds(50);if(delayEnabled)ResetDelay();if(jumpEnabled)ResetJumpDelay();}
 const bool inWorld=!needWorld||cachedWorldReady;
 if(leftEnabled&&heldLeft&&(!moduleSettings.inventory||inWorld)){
  if(nextLeft==std::chrono::steady_clock::time_point{})nextLeft=now+ClickTiming::Next<std::chrono::steady_clock>(moduleSettings.leftMin,moduleSettings.leftMax,clickRng,smoothCps,targetCps,targetClicks,cpsInitialized);
  if(now>=nextLeft){Click(false,!moduleSettings.breakBlocks);nextLeft=now+ClickTiming::Next<std::chrono::steady_clock>(moduleSettings.leftMin,moduleSettings.leftMax,clickRng,smoothCps,targetCps,targetClicks,cpsInitialized);}
 }else{nextLeft={};cpsInitialized=false;}
 if(rightEnabled&&heldRight&&inWorld){
  if(now>=nextItemUseCheck){cachedItemUse=ReadItemUseState();nextItemUseCheck=now+std::chrono::milliseconds(10);}
  const auto interval=[&]{const double rate=std::uniform_real_distribution<double>(moduleSettings.rightMin,moduleSettings.rightMax)(clickRng);return std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(1.0/rate));};
  const auto action=rightClickGuard.Update(now,true,true,true,cachedItemUse,interval());
  if(action==RightClickAction::Down)Click(true,false);else if(action==RightClickAction::Up)ReleaseRightClick();
 }else {rightClickGuard.Reset();cachedItemUse=ItemUseState::Unknown;nextItemUseCheck={};}
}
void RegisterClickerKey(HWND w,UINT key){
    if(hotkeyRegistered)UnregisterHotKey(w,1);hotkeyRegistered=false;
    if(!RegisterHotKey(w,1,MOD_NOREPEAT,key)){
        hotkeyRegistered=RegisterHotKey(w,1,MOD_NOREPEAT,clickerKey)!=FALSE;
        statusText=hotkeyRegistered?"Key unavailable. Previous keybind retained.":"Key unavailable. Choose another.";
        return;
    }
    clickerKey=key;hotkeyRegistered=true;wchar_t wide[80]{};char utf8[160]{};
    const UINT scan=MapVirtualKeyW(key,MAPVK_VK_TO_VSC);const LONG code=static_cast<LONG>(scan<<16);
    if(GetKeyNameTextW(code,wide,80)&&WideCharToMultiByte(CP_UTF8,0,wide,-1,utf8,160,nullptr,nullptr))bindLabel=utf8;else bindLabel="Key "+std::to_string(key);
    statusText="AutoClicker keybind";
}
bool CreateTarget(){Microsoft::WRL::ComPtr<ID3D11Texture2D> buffer;if(FAILED(swapChain->GetBuffer(0,IID_PPV_ARGS(buffer.GetAddressOf()))))return false;return SUCCEEDED(device->CreateRenderTargetView(buffer.Get(),nullptr,target.ReleaseAndGetAddressOf()));}
LRESULT CALLBACK WindowProc(HWND w,UINT msg,WPARAM wp,LPARAM lp){
 if(msg==WM_NCHITTEST){POINT pt{GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};if(ScreenToClient(w,&pt)){const ImVec2 p=AnimeUI::UnprojectTiltMouse(ImVec2(static_cast<float>(pt.x),static_cast<float>(pt.y)));if(p.x>=0&&p.x<820&&p.y>=0&&p.y<58)return HTCAPTION;}return HTCLIENT;}
 if(msg==WM_KEYDOWN&&capturingBind){capturingBind=false;RegisterClickerKey(w,wp==VK_ESCAPE?clickerKey:static_cast<UINT>(wp));return 0;}
 if(msg==WM_HOTKEY&&wp==1){leftEnabled=!leftEnabled;return 0;}
 if(ImGui::GetCurrentContext()&&ImGui_ImplWin32_WndProcHandler(w,msg,wp,lp))return 1;
 switch(msg){case WM_SIZE:if(swapChain&&wp!=SIZE_MINIMIZED){target.Reset();if(SUCCEEDED(swapChain->ResizeBuffers(0,LOWORD(lp),HIWORD(lp),DXGI_FORMAT_UNKNOWN,0)))CreateTarget();}return 0;
 case WM_CLOSE:DestroyWindow(w);return 0;case WM_DESTROY:if(hotkeyRegistered)UnregisterHotKey(w,1);PostQuitMessage(0);return 0;default:return DefWindowProcW(w,msg,wp,lp);}
}
bool CreateGraphics(HWND w){
 D3D_FEATURE_LEVEL level{};const D3D_FEATURE_LEVEL levels[]={D3D_FEATURE_LEVEL_11_0,D3D_FEATURE_LEVEL_10_0};
 if(FAILED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT,levels,2,D3D11_SDK_VERSION,device.ReleaseAndGetAddressOf(),&level,context.ReleaseAndGetAddressOf())))return false;
 Microsoft::WRL::ComPtr<IDXGIDevice> dxgi;Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;Microsoft::WRL::ComPtr<IDXGIFactory2> factory;Microsoft::WRL::ComPtr<IDXGISwapChain1> chain;
 if(FAILED(device.As(&dxgi))||FAILED(dxgi->GetAdapter(&adapter))||FAILED(adapter->GetParent(IID_PPV_ARGS(factory.GetAddressOf()))))return false;
 DXGI_SWAP_CHAIN_DESC1 desc{};desc.Width=820;desc.Height=540;desc.Format=DXGI_FORMAT_B8G8R8A8_UNORM;desc.SampleDesc.Count=1;desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;desc.BufferCount=2;desc.SwapEffect=DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;desc.AlphaMode=DXGI_ALPHA_MODE_PREMULTIPLIED;
 if(FAILED(factory->CreateSwapChainForComposition(device.Get(),&desc,nullptr,&chain))||FAILED(chain.As(&swapChain)))return false;
 if(FAILED(DCompositionCreateDevice(dxgi.Get(),IID_PPV_ARGS(composition.GetAddressOf())))||FAILED(composition->CreateTargetForHwnd(w,TRUE,&compositionTarget))||FAILED(composition->CreateVisual(&compositionVisual))||FAILED(compositionVisual->SetContent(chain.Get()))||FAILED(compositionTarget->SetRoot(compositionVisual.Get()))||FAILED(composition->Commit()))return false;
 return CreateTarget();
}
LRESULT CALLBACK HudWindowProc(HWND w,UINT msg,WPARAM wp,LPARAM lp){
 if(msg==WM_NCHITTEST)return HTTRANSPARENT;
 if(msg==WM_MOUSEACTIVATE)return MA_NOACTIVATE;
 return DefWindowProcW(w,msg,wp,lp);
}
bool CreateHud(){
 WNDCLASSW wc{};wc.hInstance=self;wc.lpfnWndProc=HudWindowProc;wc.lpszClassName=L"Boull.Hud";
 if(!RegisterClassW(&wc))return false;
 hudWindow=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE|WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_NOREDIRECTIONBITMAP,wc.lpszClassName,L"Boull HUD",WS_POPUP,0,0,1,1,nullptr,nullptr,self,nullptr);
 if(!hudWindow)return false;
 if(!SetLayeredWindowAttributes(hudWindow,0,255,LWA_ALPHA))return false;
 Microsoft::WRL::ComPtr<IDXGIDevice> dxgi;Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;Microsoft::WRL::ComPtr<IDXGIFactory2> factory;
 if(FAILED(device.As(&dxgi))||FAILED(dxgi->GetAdapter(&adapter))||FAILED(adapter->GetParent(IID_PPV_ARGS(factory.GetAddressOf()))))return false;
 DXGI_SWAP_CHAIN_DESC1 d{};d.Width=1;d.Height=1;d.Format=DXGI_FORMAT_B8G8R8A8_UNORM;d.SampleDesc.Count=1;d.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;d.BufferCount=2;d.SwapEffect=DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;d.AlphaMode=DXGI_ALPHA_MODE_PREMULTIPLIED;
 if(FAILED(factory->CreateSwapChainForComposition(device.Get(),&d,nullptr,&hudChain))||FAILED(composition->CreateTargetForHwnd(hudWindow,TRUE,&hudCompositionTarget))||FAILED(composition->CreateVisual(&hudVisual))||FAILED(hudVisual->SetContent(hudChain.Get()))||FAILED(hudCompositionTarget->SetRoot(hudVisual.Get()))||FAILED(composition->Commit()))return false;
 return true;
}
bool InitializeHudImGui(){
 menuImGuiContext=ImGui::GetCurrentContext();
 hudImGuiContext=ImGui::CreateContext();if(!hudImGuiContext){ImGui::SetCurrentContext(menuImGuiContext);return false;}
 ImGui::SetCurrentContext(hudImGuiContext);ImGuiIO& io=ImGui::GetIO();io.IniFilename=nullptr;io.LogFilename=nullptr;
 ImFontConfig fontConfig{};fontConfig.FontDataOwnedByAtlas=false;
 io.FontDefault=io.Fonts->AddFontFromMemoryTTF(const_cast<unsigned char*>(fonts::inter_regular_data),fonts::inter_regular_size,15.0f,&fontConfig);
 const bool initialized=ImGui_ImplDX11_Init(device.Get(),context.Get());
 if(!initialized){ImGui::DestroyContext(hudImGuiContext);hudImGuiContext=nullptr;}
 ImGui::SetCurrentContext(menuImGuiContext);return initialized;
}
bool UpdateHudBounds(){
 const HWND foreground=GetForegroundWindow();
 if(!hudWindow||!game||!IsWindow(game)||IsIconic(game)||(foreground!=game&&foreground!=panel)){if(hudWindow)ShowWindow(hudWindow,SW_HIDE);return false;}
 RECT client{};POINT origin{};if(!GetClientRect(game,&client)||!ClientToScreen(game,&origin))return false;
 const int width=client.right-client.left,height=client.bottom-client.top;if(width<=0||height<=0)return false;
 if(width!=hudWidth||height!=hudHeight){
  hudTarget.Reset();if(FAILED(hudChain->ResizeBuffers(0,width,height,DXGI_FORMAT_UNKNOWN,0)))return false;
  Microsoft::WRL::ComPtr<ID3D11Texture2D> buffer;
  if(FAILED(hudChain->GetBuffer(0,IID_PPV_ARGS(buffer.GetAddressOf())))||FAILED(device->CreateRenderTargetView(buffer.Get(),nullptr,hudTarget.ReleaseAndGetAddressOf())))return false;
  hudWidth=width;hudHeight=height;
 }
 SetWindowPos(hudWindow,HWND_TOPMOST,origin.x,origin.y,width,height,SWP_NOACTIVATE|SWP_SHOWWINDOW);
 return true;
}
void NotifyHud(const std::string& text){
 if(!moduleSettings.notifications)return;
 const auto now=std::chrono::steady_clock::now();
 if(hudNotifications.size()>=4){auto& oldest=hudNotifications[hudNotifications.size()-4];oldest.until=(std::min)(oldest.until,now+std::chrono::milliseconds(320));}
 hudNotifications.push_back({text,now,now+std::chrono::seconds(3)});if(hudNotifications.size()>8)hudNotifications.pop_front();
}
void TrackModuleChanges(){
 static std::array<bool,7> previous{};static bool initialized=false;
 const std::array<bool,7> current{leftEnabled,rightEnabled,delayEnabled,jumpEnabled,moduleSettings.arrayList,moduleSettings.notifications,moduleSettings.watermark};
 static const char* names[]={"AutoClicker","RightClicker","HitDelayFix","NoJumpDelay","ArrayList","Notifications","Watermark"};
 if(initialized&&moduleSettings.notifications)for(size_t i=0;i<current.size();++i)if(current[i]!=previous[i]){
  NotifyHud(std::string(names[i])+(current[i]?" enabled":" disabled"));
 }
 previous=current;initialized=true;
 if(!moduleSettings.notifications)hudNotifications.clear();
}
ImU32 HudBackground(int opacity){return HudCards::Alpha(7,14,18,((std::clamp)(opacity,0,100)*255+50)/100);}
float HudTextSize(){return (std::max)(10.0f,std::floor(14.0f*moduleSettings.hudTextScale/100.0f+0.5f));}
void DrawWatermark(ImDrawList* draw,ImFont* font,ImVec2 viewport,ImU32 accent,int fps){
 const float fontSize=HudTextSize();
 const char* name=moduleSettings.watermarkName[0]?moduleSettings.watermarkName:"Boull";
 char fpsText[32]{};if(moduleSettings.watermarkFps&&fps>=0)std::snprintf(fpsText,sizeof(fpsText),"%d fps",fps);
 const ImVec2 size=HudCards::WatermarkSize(font,fontSize,name,fpsText,"",nullptr);
 const ImVec2 pos=HudCards::WatermarkPosition(viewport,size,moduleSettings.watermarkX,moduleSettings.watermarkY);
 HudCards::Watermark(draw,font,pos,fontSize,name,fpsText,"",accent,moduleSettings.watermarkBackground,HudBackground(moduleSettings.watermarkOpacity),moduleSettings.watermarkShadow,nullptr);
}
void RenderHud(){
 TrackModuleChanges();
 const auto now=std::chrono::steady_clock::now();std::erase_if(hudNotifications,[&](const Notification& notification){return notification.until<=now;});
 if(!moduleSettings.arrayList&&!moduleSettings.watermark&&hudNotifications.empty()){if(hudWindow)ShowWindow(hudWindow,SW_HIDE);return;}
 if(!UpdateHudBounds())return;
 ImGui::SetCurrentContext(hudImGuiContext);
 ImGui_ImplDX11_NewFrame();ImGuiIO& io=ImGui::GetIO();io.DisplaySize=ImVec2(static_cast<float>(hudWidth),static_cast<float>(hudHeight));io.DeltaTime=1.0f/60.0f;ImGui::NewFrame();
 ImDrawList* dl=ImGui::GetBackgroundDrawList();ImFont* font=ImGui::GetFont();
 const double seconds=std::chrono::duration<double>(now.time_since_epoch()).count();const float blend=static_cast<float>((1.0-std::cos(seconds*6.283185307179586/10.0))*0.5*0.28);
 const ImVec4 color=AnimeUI::g_accent_f;const ImU32 accent=ImGui::ColorConvertFloat4ToU32(ImVec4(color.x+(1-color.x)*blend,color.y+(1-color.y)*blend,color.z+(1-color.z)*blend,1));
 if(moduleSettings.watermark)DrawWatermark(dl,font,io.DisplaySize,accent,gameFps);
 if(moduleSettings.arrayList){
  struct Entry{std::string name,tag;float width,nameWidth;int id;};std::vector<Entry> entries;
  const float fontSize=HudTextSize();
  const float spacing=HudCards::TextWidth(font,fontSize," ");
  const auto add=[&](const char* name,bool enabled,int id,std::string tag){if(!enabled)return;if(!moduleSettings.arrayTags)tag.clear();const float nw=HudCards::TextWidth(font,fontSize,name);const float w=nw+(tag.empty()?0:spacing+HudCards::TextWidth(font,fontSize,tag.c_str()));entries.push_back({name,std::move(tag),w,nw,id});};
  const auto cpsTag=[](int min,int max){return min==max?std::to_string(min)+"cps":std::to_string(min)+"-"+std::to_string(max)+"cps";};
  add("AutoClicker",leftEnabled&&!moduleSettings.hideLeftFromArray,0,cpsTag(moduleSettings.leftMin,moduleSettings.leftMax));add("RightClicker",rightEnabled&&!moduleSettings.hideRightFromArray,1,cpsTag(moduleSettings.rightMin,moduleSettings.rightMax));add("HitDelayFix",delayEnabled&&!moduleSettings.hideHitDelayFromArray,2,"");add("NoJumpDelay",jumpEnabled&&!moduleSettings.hideJumpDelayFromArray,3,"");
  std::sort(entries.begin(),entries.end(),[](const Entry& a,const Entry& b){return a.width>b.width;});
  const float right=static_cast<float>(hudWidth)-(moduleSettings.arrayModern?4.0f:8.0f);
  const float top=(std::max)(2.0f,4.0f+moduleSettings.arrayPositionY*(hudHeight-8.0f)/100.0f-8.0f);
  const float advance=moduleSettings.arrayModern?HudCards::ModernArrayListRowLayout(right,top,0,fontSize).advance:fontSize+2.0f;
  static std::array<float,4> rows{};const float rowBlend=1.0f-std::exp(-16.0f*io.DeltaTime);
  for(size_t i=0;i<entries.size();++i){const auto& e=entries[i];rows[e.id]+=(static_cast<float>(i)-rows[e.id])*rowBlend;const float y=top+rows[e.id]*advance;
   HudCards::ArrayListRowLayout layout{};
   if(moduleSettings.arrayModern){layout=HudCards::ModernArrayListRowLayout(right,y,e.width,fontSize);HudCards::ModernArrayListBackground(dl,layout,HudBackground(moduleSettings.arrayOpacity));}
   else {const float inset=3*fontSize/14;layout={ImVec2(right-e.width-inset,y-1),ImVec2(right+inset,y+fontSize+1),ImVec2(right-e.width,y),advance,inset};}
   HudCards::ShadowedText(dl,font,fontSize,layout.text,accent,e.name.c_str(),moduleSettings.arrayShadow);
   if(!e.tag.empty())HudCards::ShadowedText(dl,font,fontSize,ImVec2(layout.text.x+e.nameWidth+spacing,layout.text.y),HudCards::ArrayListTagColor(),e.tag.c_str(),moduleSettings.arrayShadow);
  }
 }
 static auto lastNotificationFrame=now;const float delta=(std::clamp)(std::chrono::duration<float>(now-lastNotificationFrame).count(),0.0f,0.1f);lastNotificationFrame=now;
 const float movement=1.0f-std::exp(-14.0f*delta);
 float y=static_cast<float>(hudHeight)-25.0f;
 for(auto it=hudNotifications.rbegin();it!=hudNotifications.rend();++it){
  if(!it->positioned){it->renderY=y;it->positioned=true;}else it->renderY+=(y-it->renderY)*movement;
  const auto animation=NotificationAnimation::Evaluate(now,it->created,it->until);
  const ImVec2 size=ImGui::CalcTextSize(it->text.c_str());const float x=static_cast<float>(hudWidth)-size.x-35+animation.offsetX;
  dl->AddRectFilled(ImVec2(x,it->renderY-25),ImVec2(x+size.x+23,it->renderY+1),HudCards::Alpha(16,17,22,190,animation.opacity),5);
  dl->AddText(ImVec2(x+8,it->renderY-20),HudCards::Alpha(255,255,255,255,animation.opacity),it->text.c_str());y-=32;
 }
 ImGui::Render();const float clear[4]={0,0,0,0};ID3D11RenderTargetView* view=hudTarget.Get();context->OMSetRenderTargets(1,&view,nullptr);context->ClearRenderTargetView(view,clear);ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());hudChain->Present(0,0);
 ImGui::SetCurrentContext(menuImGuiContext);
}
bool CreateLogoTexture(){
 D3D11_TEXTURE2D_DESC desc{};desc.Width=360;desc.Height=78;desc.MipLevels=1;desc.ArraySize=1;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_IMMUTABLE;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
 D3D11_SUBRESOURCE_DATA data{};data.pSysMem=boullLogoRgba;data.SysMemPitch=360*4;
 Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
 if(FAILED(device->CreateTexture2D(&desc,&data,&texture)))return false;
 if(FAILED(device->CreateShaderResourceView(texture.Get(),nullptr,&logoTexture)))return false;
 AnimeUI::g_logoTexture=static_cast<ImTextureID>(reinterpret_cast<uintptr_t>(logoTexture.Get()));return true;
}
void RenderPanel(){
 bool requestBind=false,requestUnload=false;
 AnimeUI::RenderAutoClickMenu(leftEnabled,rightEnabled,delayEnabled,jumpEnabled,mapped,jumpTicks!=nullptr,hudReady,cps,capturingBind,bindLabel,statusText,requestBind,requestUnload);
 if(!OfflineProfiles::requested.empty()){auto name=std::move(OfflineProfiles::requested);OfflineProfiles::requested.clear();if(ApplyOfflineProfile(vm,env,minecraft,instance,world,name,OfflineProfiles::status)){OfflineProfiles::current=name;NotifyHud("Profile "+name+" applied");}}
 if(OfflineProfiles::restoreRequested){OfflineProfiles::restoreRequested=false;if(RestoreOriginalSession(env,minecraft,instance,world,OfflineProfiles::status)){OfflineProfiles::current.clear();NotifyHud("Original profile restored");}}
 if(requestBind){capturingBind=true;if(hotkeyRegistered){UnregisterHotKey(panel,1);hotkeyRegistered=false;}statusText="Press a key; Esc cancels.";SetFocus(panel);}
 if(requestUnload){leftEnabled=false;rightEnabled=false;delayEnabled=false;jumpEnabled=false;unloadRequested=true;}
}
DWORD WINAPI Run(void*){
 const bool attached=AttachJvm();mapped=attached&&FindMinecraft();
 WNDCLASSW wc{};wc.lpfnWndProc=WindowProc;wc.hInstance=self;wc.lpszClassName=L"Boull.Window";wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);wc.hIcon=LoadIconW(self,MAKEINTRESOURCEW(2));wc.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);
 if(!RegisterClassW(&wc)){ResetFields();if(attached)vm->DetachCurrentThread();FreeLibraryAndExitThread(self,1);}
 panel=CreateWindowExW(WS_EX_NOREDIRECTIONBITMAP,wc.lpszClassName,L"Boull",WS_POPUP,CW_USEDEFAULT,CW_USEDEFAULT,820,540,nullptr,nullptr,self,nullptr);
 if(!panel||!CreateGraphics(panel)){MessageBoxW(nullptr,L"Could not start the DirectX 11 window.",L"Boull",MB_OK|MB_ICONERROR);if(panel)DestroyWindow(panel);target.Reset();swapChain.Reset();context.Reset();device.Reset();UnregisterClassW(wc.lpszClassName,self);ResetFields();if(attached)vm->DetachCurrentThread();FreeLibraryAndExitThread(self,1);}
 if(!CreateLogoTexture())MessageBoxW(nullptr,L"Could not load the Boull logo.",L"Boull",MB_OK|MB_ICONWARNING);
 IMGUI_CHECKVERSION();ImGui::CreateContext();
 char winDir[MAX_PATH]{};const UINT winDirLen=GetWindowsDirectoryA(winDir,MAX_PATH);
 if(winDirLen>0&&winDirLen<MAX_PATH){
  const std::string fontDir=std::string(winDir)+"\\Fonts\\";
  const std::string regular=fontDir+"segoeui.ttf",bold=fontDir+"seguisb.ttf";
  ImGuiIO& io=ImGui::GetIO();
  if(GetFileAttributesA(regular.c_str())!=INVALID_FILE_ATTRIBUTES)io.FontDefault=io.Fonts->AddFontFromFileTTF(regular.c_str(),16.0f);
  if(GetFileAttributesA(bold.c_str())!=INVALID_FILE_ATTRIBUTES)AnimeUI::g_font_bold=io.Fonts->AddFontFromFileTTF(bold.c_str(),16.0f);
 }
 AnimeUI::g_drag_widget=false;AnimeUI::ApplyTheme();
 if(!ImGui_ImplWin32_Init(panel)){ImGui::DestroyContext();DestroyWindow(panel);target.Reset();swapChain.Reset();context.Reset();device.Reset();UnregisterClassW(wc.lpszClassName,self);ResetFields();if(attached)vm->DetachCurrentThread();FreeLibraryAndExitThread(self,1);}
 if(!ImGui_ImplDX11_Init(device.Get(),context.Get())){ImGui_ImplWin32_Shutdown();ImGui::DestroyContext();DestroyWindow(panel);target.Reset();swapChain.Reset();context.Reset();device.Reset();UnregisterClassW(wc.lpszClassName,self);ResetFields();if(attached)vm->DetachCurrentThread();FreeLibraryAndExitThread(self,1);}
 hudReady=CreateHud()&&InitializeHudImGui();
 RegisterClickerKey(panel,VK_F6);ShowWindow(panel,SW_SHOW);SetWindowPos(panel,nullptr,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);UpdateWindow(panel);
 bool running=true;MSG msg{};auto nextFrame=std::chrono::steady_clock::now();while(running){while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){if(msg.message==WM_QUIT){running=false;break;}TranslateMessage(&msg);DispatchMessageW(&msg);}if(!running)break;
  Tick();const auto frameNow=std::chrono::steady_clock::now();if(frameNow<nextFrame){Sleep(1);continue;}nextFrame=frameNow+std::chrono::milliseconds(16);ImGui_ImplDX11_NewFrame();ImGui_ImplWin32_NewFrame();
  POINT mouse{};if(GetForegroundWindow()==panel&&GetCursorPos(&mouse)&&ScreenToClient(panel,&mouse)){const ImVec2 pos=AnimeUI::UnprojectTiltMouse(ImVec2(static_cast<float>(mouse.x),static_cast<float>(mouse.y)));ImGui::GetIO().AddMousePosEvent(pos.x,pos.y);}
  ImGui::NewFrame();RenderPanel();ImGui::Render();
  if(unloadRequested){if(!PostMessageW(panel,WM_CLOSE,0,0)){unloadRequested=false;statusText="Could not close the window.";}}
  if(target){const float clear[4]={0,0,0,0};ID3D11RenderTargetView* view=target.Get();context->OMSetRenderTargets(1,&view,nullptr);context->ClearRenderTargetView(view,clear);ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());}
  if(swapChain->Present(0,0)==DXGI_STATUS_OCCLUDED)Sleep(50);
  if(hudReady)RenderHud();
 }
 if(hudWindow)DestroyWindow(hudWindow);hudTarget.Reset();hudVisual.Reset();hudCompositionTarget.Reset();hudChain.Reset();UnregisterClassW(L"Boull.Hud",self);
 if(hudImGuiContext){ImGui::SetCurrentContext(hudImGuiContext);ImGui_ImplDX11_Shutdown();ImGui::DestroyContext(hudImGuiContext);hudImGuiContext=nullptr;ImGui::SetCurrentContext(menuImGuiContext);}
 ImGui_ImplDX11_Shutdown();ImGui_ImplWin32_Shutdown();ImGui::DestroyContext();AnimeUI::g_logoTexture=ImTextureID_Invalid;logoTexture.Reset();target.Reset();compositionVisual.Reset();compositionTarget.Reset();composition.Reset();swapChain.Reset();context.Reset();device.Reset();ResetFields();if(attached)vm->DetachCurrentThread();UnregisterClassW(wc.lpszClassName,self);FreeLibraryAndExitThread(self,0);
}
}
BOOL APIENTRY DllMain(HMODULE module,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){self=module;DisableThreadLibraryCalls(module);HANDLE thread=CreateThread(nullptr,0,Run,nullptr,0,nullptr);if(!thread)return FALSE;CloseHandle(thread);}return TRUE;}






