#include <windows.h>
#include <filesystem>
int WINAPI wWinMain(HINSTANCE,HINSTANCE,PWSTR,int) {
    wchar_t path[MAX_PATH]{};
    if(!GetModuleFileNameW(nullptr,path,MAX_PATH))return 1;
    const auto dll=std::filesystem::path(path).parent_path()/L"BoullRuntime.dll";
    if(!LoadLibraryW(dll.c_str()))return 2;
    Sleep(120000);
    return 0;
}

