#include <windows.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include <filesystem>
#include <cstdint>
#include <cwctype>
#include <string>
#include <vector>
#include "embedded_runtime.hpp"
#include "updater.hpp"

namespace {
struct Handle { HANDLE value{}; ~Handle(){if(value && value!=INVALID_HANDLE_VALUE) CloseHandle(value);} };
struct Target { HWND window{}; DWORD pid{}; };
BOOL CALLBACK FindGame(HWND window, LPARAM arg) {
    if(!IsWindowVisible(window)) return TRUE;
    wchar_t name[128]{}; if(!GetClassNameW(window,name,128)) return TRUE;
    if(wcsstr(name,L"LWJGL")==nullptr && wcsstr(name,L"GLFW")==nullptr) return TRUE;
    wchar_t title[1024]{}; if(!GetWindowTextW(window,title,1024)) return TRUE;
    std::wstring lower=title; for(auto& c:lower)c=static_cast<wchar_t>(towlower(c));
    if(lower.find(L"minecraft")==std::wstring::npos && lower.find(L"lunar client")==std::wstring::npos && lower.find(L"badlion")==std::wstring::npos) return TRUE;
    DWORD pid=0; GetWindowThreadProcessId(window,&pid); if(!pid) return TRUE;
    Handle process{OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid)}; if(!process.value) return TRUE;
    wchar_t image[32768]{}; DWORD length=32768;
    if(!QueryFullProcessImageNameW(process.value,0,image,&length)) return TRUE;
    std::filesystem::path path(image); if(_wcsicmp(path.filename().c_str(),L"javaw.exe") && _wcsicmp(path.filename().c_str(),L"java.exe")) return TRUE;
    auto& targets=*reinterpret_cast<std::vector<Target>*>(arg);
    for(const auto& target:targets) if(target.pid==pid) return TRUE;
    targets.push_back({window,pid}); return TRUE;
}
LPTHREAD_START_ROUTINE RemoteLoader(DWORD pid) {
    HMODULE kernel=GetModuleHandleW(L"kernel32.dll");
    const auto proc=kernel?GetProcAddress(kernel,"LoadLibraryW"):nullptr; HMODULE owner{};
    if(!proc||!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(proc),&owner)) return nullptr;
    wchar_t path[32768]{}; if(!GetModuleFileNameW(owner,path,32768))return nullptr;
    const auto name=std::filesystem::path(path).filename();
    Handle snapshot{CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid)};
    if(snapshot.value==INVALID_HANDLE_VALUE)return nullptr;
    MODULEENTRY32W entry{};entry.dwSize=sizeof(entry);
    for(BOOL ok=Module32FirstW(snapshot.value,&entry);ok;ok=Module32NextW(snapshot.value,&entry)){
        if(_wcsicmp(entry.szModule,name.c_str())==0){const auto offset=reinterpret_cast<std::uintptr_t>(proc)-reinterpret_cast<std::uintptr_t>(owner);return reinterpret_cast<LPTHREAD_START_ROUTINE>(reinterpret_cast<std::uintptr_t>(entry.modBaseAddr)+offset);}
    }
    return nullptr;
}
bool Attach(DWORD pid,const std::wstring& dll,std::wstring& error) {
    Handle process{OpenProcess(PROCESS_CREATE_THREAD|PROCESS_QUERY_INFORMATION|PROCESS_VM_OPERATION|PROCESS_VM_WRITE|PROCESS_VM_READ,FALSE,pid)};
    if(!process.value){error=L"Could not access Minecraft. Check that both programs have the same permission level.";return false;}
    auto loader=RemoteLoader(pid);
    if(!loader){error=L"Could not find LoadLibraryW.";return false;}
    const SIZE_T bytes=(dll.size()+1)*sizeof(wchar_t); void* remote=VirtualAllocEx(process.value,nullptr,bytes,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if(!remote){error=L"Could not allocate memory in the process.";return false;}
    SIZE_T written=0; if(!WriteProcessMemory(process.value,remote,dll.c_str(),bytes,&written)||written!=bytes){VirtualFreeEx(process.value,remote,0,MEM_RELEASE);error=L"Could not send the DLL path.";return false;}
    Handle thread{CreateRemoteThread(process.value,nullptr,0,loader,remote,0,nullptr)};
    if(!thread.value){VirtualFreeEx(process.value,remote,0,MEM_RELEASE);error=L"Could not start loading the DLL.";return false;}
    const DWORD wait=WaitForSingleObject(thread.value,15000);
    if(wait!=WAIT_OBJECT_0){error=L"Loading timed out. Close Minecraft before trying again.";return false;}
    DWORD result=0; const BOOL got=GetExitCodeThread(thread.value,&result); VirtualFreeEx(process.value,remote,0,MEM_RELEASE);
    if(!got||!result){error=L"Windows could not load the DLL.";return false;} return true;
}
}
int WINAPI wWinMain(HINSTANCE,HINSTANCE,PWSTR,int) {
    int argc=0;wchar_t** argv=CommandLineToArgvW(GetCommandLineW(),&argc);if(!argv)return 1;
    if(argc>=2&&wcscmp(argv[1],L"--apply-update")==0){const int result=ApplyUpdateCommand(argc,argv);LocalFree(argv);return result;}
    if(argc==2&&wcscmp(argv[1],L"--verify-package")==0){const auto resource=ExtractEmbeddedRuntime();LocalFree(argv);return resource.error.empty()?0:1;}
    const bool postUpdate=argc==3&&wcscmp(argv[1],L"--post-update")==0;
    if(postUpdate)CleanupUpdateCommand(argc,argv);
    LocalFree(argv);
    if(!postUpdate){std::wstring updateError;const auto result=CheckForUpdate(updateError);if(result==UpdateCheck::Started)return 0;if(result==UpdateCheck::Failed)MessageBoxW(nullptr,updateError.c_str(),L"Boull — Update",MB_OK|MB_ICONWARNING);}
    std::vector<Target> targets;
    if(!EnumWindows(FindGame,reinterpret_cast<LPARAM>(&targets))||targets.empty()){MessageBoxW(nullptr,L"Open Minecraft Java and try again.",L"Boull",MB_OK|MB_ICONWARNING);return 1;}

    const auto resource=ExtractEmbeddedRuntime();
    if(!resource.error.empty()){MessageBoxW(nullptr,resource.error.c_str(),L"Boull",MB_OK|MB_ICONERROR);return 1;}
    std::wstring error;
    if(!Attach(targets.front().pid,resource.path,error)){MessageBoxW(nullptr,error.c_str(),L"Boull",MB_OK|MB_ICONERROR);return 1;}
    return 0;
}



