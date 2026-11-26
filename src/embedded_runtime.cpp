#include "embedded_runtime.hpp"
#include <windows.h>
#include <filesystem>
#include <algorithm>
#include <vector>
#include <system_error>

namespace {
struct Handle { HANDLE value=INVALID_HANDLE_VALUE; ~Handle(){if(value!=INVALID_HANDLE_VALUE)CloseHandle(value);} Handle(const Handle&)=delete;Handle& operator=(const Handle&)=delete;explicit Handle(HANDLE h):value(h){} };
bool Matches(const std::filesystem::path& path,const unsigned char* data,DWORD size){
 Handle file(CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr));
 if(file.value==INVALID_HANDLE_VALUE)return false;
 LARGE_INTEGER length{};if(!GetFileSizeEx(file.value,&length)||length.QuadPart!=size)return false;
 std::vector<unsigned char> buffer(65536);DWORD offset=0;
 while(offset<size){DWORD read=0;const DWORD want=(std::min)(static_cast<DWORD>(buffer.size()),size-offset);
  if(!ReadFile(file.value,buffer.data(),want,&read,nullptr)||read!=want||!std::equal(buffer.begin(),buffer.begin()+read,data+offset))return false;offset+=read;}
 return true;
}
}
EmbeddedRuntimeResult ExtractEmbeddedRuntime(){
 HMODULE module=GetModuleHandleW(nullptr);if(!module)return {{},L"Could not locate the executable."};
 HRSRC resource=FindResourceW(module,MAKEINTRESOURCEW(101),RT_RCDATA);if(!resource)return {{},L"The embedded DLL was not found."};
 const DWORD size=SizeofResource(module,resource);HGLOBAL loaded=LoadResource(module,resource);const auto* data=loaded?static_cast<const unsigned char*>(LockResource(loaded)):nullptr;
 if(!size||!data)return {{},L"Could not read the embedded DLL."};
 const DWORD needed=GetEnvironmentVariableW(L"LOCALAPPDATA",nullptr,0);if(!needed)return {{},L"Could not locate the user data directory."};
 std::wstring base(needed,L'\0');const DWORD count=GetEnvironmentVariableW(L"LOCALAPPDATA",base.data(),needed);if(!count||count>=needed)return {{},L"Could not locate the user data directory."};base.resize(count);
 const auto directory=std::filesystem::path(base)/L"Boull"/L"Runtime"/std::filesystem::path(BOULL_VERSION);
 std::error_code ec;std::filesystem::create_directories(directory,ec);if(ec)return {{},L"Could not create the Boull runtime directory."};
 const auto destination=directory/L"BoullRuntime.dll";
 if(Matches(destination,data,size))return {destination.wstring(),{}};
 wchar_t temp[MAX_PATH]{};if(directory.wstring().size()>=MAX_PATH-20||!GetTempFileNameW(directory.c_str(),L"BUL",0,temp))return {{},L"Could not prepare the temporary DLL."};
 bool written=false;
 {
  Handle file(CreateFileW(temp,GENERIC_WRITE,0,nullptr,TRUNCATE_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr));
  if(file.value!=INVALID_HANDLE_VALUE){DWORD offset=0;written=true;while(offset<size){DWORD countWritten=0;const DWORD chunk=(std::min)(size-offset,static_cast<DWORD>(65536));if(!WriteFile(file.value,data+offset,chunk,&countWritten,nullptr)||countWritten!=chunk){written=false;break;}offset+=countWritten;}if(written&&!FlushFileBuffers(file.value))written=false;}
 }
 if(!written){DeleteFileW(temp);return {{},L"Could not write the embedded DLL."};}
 if(!MoveFileExW(temp,destination.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){
  DeleteFileW(temp);if(Matches(destination,data,size))return {destination.wstring(),{}};
  return {{},L"Could not update the DLL while it is in use. Close the client and try again."};
 }
 if(!Matches(destination,data,size)){DeleteFileW(destination.c_str());return {{},L"Embedded DLL verification failed."};}
 return {destination.wstring(),{}};
}

