#include "updater.hpp"
#include "update_policy.hpp"
#include <winhttp.h>
#include <bcrypt.h>
#include <algorithm>
#include <cctype>
#include <cwctype>
#include <filesystem>
#include <limits>
#include <memory>
#include <vector>

namespace {
using ByteVector=std::vector<unsigned char>;
struct InternetHandle {HINTERNET h{};~InternetHandle(){if(h)WinHttpCloseHandle(h);}explicit InternetHandle(HINTERNET v):h(v){} InternetHandle(const InternetHandle&)=delete;};
struct KernelHandle {HANDLE h=INVALID_HANDLE_VALUE;~KernelHandle(){if(h&&h!=INVALID_HANDLE_VALUE)CloseHandle(h);}explicit KernelHandle(HANDLE v):h(v){} KernelHandle(const KernelHandle&)=delete;};
struct Algorithm {BCRYPT_ALG_HANDLE h{};~Algorithm(){if(h)BCryptCloseAlgorithmProvider(h,0);}};
struct HashHandle {BCRYPT_HASH_HANDLE h{};~HashHandle(){if(h)BCryptDestroyHash(h);}};
constexpr size_t kMaxAsset=64u*1024u*1024u;
struct HttpResponse {DWORD status=0;ByteVector data;};
std::wstring Widen(std::string_view s){if(s.empty())return {};const int count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),static_cast<int>(s.size()),nullptr,0);if(count<=0)return {};std::wstring result(count,L'\0');if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),static_cast<int>(s.size()),result.data(),count))return {};return result;}
bool ApprovedUrl(const std::wstring& url){
 URL_COMPONENTS c{};c.dwStructSize=sizeof(c);c.dwHostNameLength=DWORD(-1);c.dwSchemeLength=DWORD(-1);
 if(!WinHttpCrackUrl(url.c_str(),0,0,&c)||c.nScheme!=INTERNET_SCHEME_HTTPS)return false;
 std::wstring host(c.lpszHostName,c.dwHostNameLength);std::transform(host.begin(),host.end(),host.begin(),towlower);
 constexpr std::wstring_view suffix=L".githubusercontent.com";
 return host==L"github.com"||host==L"api.github.com"||(host.size()>suffix.size()&&host.compare(host.size()-suffix.size(),suffix.size(),suffix)==0);
}
bool Get(const std::wstring& url,size_t limit,HttpResponse& out){
 if(!ApprovedUrl(url))return false;
 URL_COMPONENTS c{};c.dwStructSize=sizeof(c);c.dwHostNameLength=DWORD(-1);c.dwUrlPathLength=DWORD(-1);c.dwExtraInfoLength=DWORD(-1);
 if(!WinHttpCrackUrl(url.c_str(),0,0,&c)||c.nScheme!=INTERNET_SCHEME_HTTPS)return false;
 const std::wstring host(c.lpszHostName,c.dwHostNameLength);std::wstring path(c.lpszUrlPath,c.dwUrlPathLength);if(c.lpszExtraInfo)path.append(c.lpszExtraInfo,c.dwExtraInfoLength);if(path.empty())path=L"/";
 const std::wstring agent=L"Boull/"+Widen(BOULL_VERSION);
 InternetHandle session(WinHttpOpen(agent.c_str(),WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0));if(!session.h)return false;
 if(!WinHttpSetTimeouts(session.h,3000,3000,3000,5000))return false;
 InternetHandle connection(WinHttpConnect(session.h,host.c_str(),c.nPort,0));if(!connection.h)return false;
 InternetHandle request(WinHttpOpenRequest(connection.h,L"GET",path.c_str(),nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE));if(!request.h)return false;
 DWORD policy=WINHTTP_OPTION_REDIRECT_POLICY_DISALLOW_HTTPS_TO_HTTP;if(!WinHttpSetOption(request.h,WINHTTP_OPTION_REDIRECT_POLICY,&policy,sizeof(policy)))return false;
 constexpr wchar_t headers[]=L"Accept: application/vnd.github+json\r\nX-GitHub-Api-Version: 2022-11-28\r\n";
 if(!WinHttpSendRequest(request.h,headers,DWORD(-1),WINHTTP_NO_REQUEST_DATA,0,0,0)||!WinHttpReceiveResponse(request.h,nullptr))return false;
 DWORD urlBytes=0;WinHttpQueryOption(request.h,WINHTTP_OPTION_URL,nullptr,&urlBytes);if(GetLastError()!=ERROR_INSUFFICIENT_BUFFER||urlBytes<sizeof(wchar_t))return false;
 std::vector<wchar_t> finalUrl(urlBytes/sizeof(wchar_t)+1);if(!WinHttpQueryOption(request.h,WINHTTP_OPTION_URL,finalUrl.data(),&urlBytes)||!ApprovedUrl(finalUrl.data()))return false;
 DWORD length=sizeof(out.status);if(!WinHttpQueryHeaders(request.h,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&out.status,&length,WINHTTP_NO_HEADER_INDEX))return false;
 if(out.status!=200)return true;
 for(;;){DWORD available=0;if(!WinHttpQueryDataAvailable(request.h,&available))return false;if(!available)break;if(out.data.size()+available>limit)return false;const size_t old=out.data.size();out.data.resize(old+available);DWORD read=0;if(!WinHttpReadData(request.h,out.data.data()+old,available,&read)||!read)return false;out.data.resize(old+read);}
 return true;
}
std::string Sha256(const ByteVector& bytes){
 Algorithm algorithm;if(BCryptOpenAlgorithmProvider(&algorithm.h,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)return {};
 DWORD objectSize=0,hashSize=0,received=0;
 if(BCryptGetProperty(algorithm.h,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&objectSize),sizeof(objectSize),&received,0)<0||BCryptGetProperty(algorithm.h,BCRYPT_HASH_LENGTH,reinterpret_cast<PUCHAR>(&hashSize),sizeof(hashSize),&received,0)<0||hashSize!=32)return {};
 std::vector<unsigned char> object(objectSize),hash(hashSize);HashHandle handle;
 if(BCryptCreateHash(algorithm.h,&handle.h,object.data(),objectSize,nullptr,0,0)<0||BCryptHashData(handle.h,const_cast<PUCHAR>(bytes.data()),static_cast<ULONG>(bytes.size()),0)<0||BCryptFinishHash(handle.h,hash.data(),hashSize,0)<0)return {};
 constexpr char digits[]="0123456789abcdef";std::string hex;hex.reserve(64);for(unsigned char b:hash){hex+=digits[b>>4];hex+=digits[b&15];}return hex;
}
std::filesystem::path SelfPath(){std::vector<wchar_t> buffer(32768);const DWORD n=GetModuleFileNameW(nullptr,buffer.data(),static_cast<DWORD>(buffer.size()));return n&&n<buffer.size()?std::filesystem::path(buffer.data()):std::filesystem::path{};}
bool IsBoull(const std::filesystem::path& path){return _wcsicmp(path.filename().c_str(),L"Boull.exe")==0;}
bool SamePath(const std::filesystem::path& a,const std::filesystem::path& b){std::error_code ec1,ec2;const auto left=std::filesystem::weakly_canonical(a,ec1),right=std::filesystem::weakly_canonical(b,ec2);return !ec1&&!ec2&&_wcsicmp(left.c_str(),right.c_str())==0;}
bool TempFile(const std::filesystem::path& directory,std::filesystem::path& out){wchar_t buffer[MAX_PATH]{};if(directory.wstring().size()>=MAX_PATH-20||!GetTempFileNameW(directory.c_str(),L"BUP",0,buffer))return false;out=buffer;return true;}
bool WriteFileBytes(const std::filesystem::path& path,const ByteVector& bytes){KernelHandle file(CreateFileW(path.c_str(),GENERIC_WRITE,0,nullptr,TRUNCATE_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr));if(file.h==INVALID_HANDLE_VALUE)return false;size_t offset=0;while(offset<bytes.size()){DWORD written=0;const DWORD n=static_cast<DWORD>((std::min)(bytes.size()-offset,size_t(65536)));if(!WriteFile(file.h,bytes.data()+offset,n,&written,nullptr)||written!=n)return false;offset+=written;}return FlushFileBuffers(file.h)!=FALSE;}
bool ReadFileBytes(const std::filesystem::path& path,ByteVector& bytes){KernelHandle file(CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr));if(file.h==INVALID_HANDLE_VALUE)return false;LARGE_INTEGER size{};if(!GetFileSizeEx(file.h,&size)||size.QuadPart<=0||size.QuadPart>static_cast<LONGLONG>(kMaxAsset))return false;bytes.resize(static_cast<size_t>(size.QuadPart));size_t offset=0;while(offset<bytes.size()){DWORD read=0;const DWORD n=static_cast<DWORD>((std::min)(bytes.size()-offset,size_t(65536)));if(!ReadFile(file.h,bytes.data()+offset,n,&read,nullptr)||!read)return false;offset+=read;}return true;}
bool IsPortableExecutable(const std::filesystem::path& file){DWORD type=0;return GetBinaryTypeW(file.c_str(),&type)&&type==SCS_64BIT_BINARY;}
std::optional<UpdatePolicy::Version> BinaryVersion(const std::filesystem::path& file){
 DWORD ignored=0;const DWORD bytes=GetFileVersionInfoSizeW(file.c_str(),&ignored);if(!bytes||bytes>1024u*1024u)return std::nullopt;
 std::vector<unsigned char> data(bytes);if(!GetFileVersionInfoW(file.c_str(),0,bytes,data.data()))return std::nullopt;
 VS_FIXEDFILEINFO* info=nullptr;UINT length=0;if(!VerQueryValueW(data.data(),L"\\",reinterpret_cast<void**>(&info),&length)||!info||length<sizeof(*info)||info->dwSignature!=0xFEEF04BD)return std::nullopt;
 return UpdatePolicy::Version{HIWORD(info->dwFileVersionMS),LOWORD(info->dwFileVersionMS),HIWORD(info->dwFileVersionLS)};
}
std::wstring Quote(const std::filesystem::path& p){return L"\""+p.wstring()+L"\"";}
bool Spawn(const std::filesystem::path& exe,std::wstring args){std::wstring cmd=Quote(exe)+L" "+args;STARTUPINFOW si{};si.cb=sizeof(si);PROCESS_INFORMATION pi{};if(!CreateProcessW(exe.c_str(),cmd.data(),nullptr,nullptr,FALSE,0,nullptr,exe.parent_path().c_str(),&si,&pi))return false;CloseHandle(pi.hThread);CloseHandle(pi.hProcess);return true;}
bool TempName(const std::filesystem::path& candidate,const std::filesystem::path& folder,const wchar_t* extension){const auto name=candidate.filename().wstring();return SamePath(candidate.parent_path(),folder)&&name.size()>=7&&_wcsnicmp(name.c_str(),L"BUP",3)==0&&_wcsicmp(candidate.extension().c_str(),extension)==0;}
bool StartUpdater(const std::filesystem::path& current,const std::filesystem::path& candidate,const std::string& expected,const std::string& version){
 std::filesystem::path helperTemp;if(!TempFile(current.parent_path(),helperTemp))return false;
 auto helper=helperTemp;helper+=L".exe";
 if(!MoveFileExW(helperTemp.c_str(),helper.c_str(),0)||!CopyFileW(current.c_str(),helper.c_str(),FALSE)){DeleteFileW(helperTemp.c_str());DeleteFileW(helper.c_str());return false;}
 SECURITY_ATTRIBUTES attributes{};attributes.nLength=sizeof(attributes);attributes.bInheritHandle=TRUE;
 KernelHandle ready(CreateEventW(&attributes,TRUE,FALSE,nullptr));if(!ready.h||ready.h==INVALID_HANDLE_VALUE){DeleteFileW(helper.c_str());return false;}
 const std::wstring args=L"--apply-update "+std::to_wstring(GetCurrentProcessId())+L" "+Quote(current)+L" "+Quote(candidate)+L" "+Widen(expected)+L" "+Widen(version)+L" "+std::to_wstring(reinterpret_cast<std::uintptr_t>(ready.h));
 SIZE_T attributeBytes=0;InitializeProcThreadAttributeList(nullptr,1,0,&attributeBytes);if(!attributeBytes){DeleteFileW(helper.c_str());return false;}
 std::vector<unsigned char> attributeStorage(attributeBytes);auto* list=reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attributeStorage.data());
 if(!InitializeProcThreadAttributeList(list,1,0,&attributeBytes)){DeleteFileW(helper.c_str());return false;}
 if(!UpdateProcThreadAttribute(list,0,PROC_THREAD_ATTRIBUTE_HANDLE_LIST,&ready.h,sizeof(ready.h),nullptr,nullptr)){DeleteProcThreadAttributeList(list);DeleteFileW(helper.c_str());return false;}
 std::wstring command=Quote(helper)+L" "+args;STARTUPINFOEXW si{};si.StartupInfo.cb=sizeof(si);si.lpAttributeList=list;PROCESS_INFORMATION pi{};
 const BOOL created=CreateProcessW(helper.c_str(),command.data(),nullptr,nullptr,TRUE,EXTENDED_STARTUPINFO_PRESENT,nullptr,helper.parent_path().c_str(),&si.StartupInfo,&pi);
 DeleteProcThreadAttributeList(list);if(!created){DeleteFileW(helper.c_str());return false;}
 CloseHandle(pi.hThread);KernelHandle child(pi.hProcess);HANDLE waiters[]={ready.h,child.h};
 if(WaitForMultipleObjects(2,waiters,FALSE,5000)==WAIT_OBJECT_0)return true;
 TerminateProcess(child.h,1);WaitForSingleObject(child.h,2000);DeleteFileW(helper.c_str());return false;
}
}
UpdateCheck CheckForUpdate(std::wstring& error){
 const auto current=SelfPath();if(current.empty()||!IsBoull(current))return UpdateCheck::NoUpdate;
 HttpResponse response;if(!Get(L"https://api.github.com/repos/Velobounce/Boull/releases/latest",1024u*1024u,response)||response.status==404)return UpdateCheck::NoUpdate;
 if(response.status!=200)return UpdateCheck::NoUpdate;
 const std::string json(response.data.begin(),response.data.end());const auto release=UpdatePolicy::ParseRelease(json);const auto version=UpdatePolicy::ParseVersion(BOULL_VERSION);
 if(!release||!version){error=L"The release does not contain a valid version and Boull.exe SHA-256 digest.";return UpdateCheck::Failed;}
 if(release->version<=*version)return UpdateCheck::NoUpdate;
 HttpResponse asset;if(!Get(Widen(release->url),kMaxAsset,asset)||asset.status!=200){error=L"Could not download the update. The current version remains available.";return UpdateCheck::Failed;}
 std::string expected=release->sha256;std::transform(expected.begin(),expected.end(),expected.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
 if(asset.data.empty()||Sha256(asset.data)!=expected){error=L"The update failed SHA-256 verification and will not be installed.";return UpdateCheck::Failed;}
 std::filesystem::path candidate;if(!TempFile(current.parent_path(),candidate)){error=L"Could not prepare the update in this directory.";return UpdateCheck::Failed;}
 if(!WriteFileBytes(candidate,asset.data)||!IsPortableExecutable(candidate)||BinaryVersion(candidate)!=release->version){DeleteFileW(candidate.c_str());error=L"The updated executable does not match the advertised version.";return UpdateCheck::Failed;}
 if(!StartUpdater(current,candidate,expected,release->tag)){DeleteFileW(candidate.c_str());error=L"Could not start the updater.";return UpdateCheck::Failed;}
 return UpdateCheck::Started;
}
int ApplyUpdateCommand(int argc,wchar_t** argv){
 if(argc!=8)return 1;wchar_t* end=nullptr;const unsigned long pid=wcstoul(argv[2],&end,10);if(!pid||!end||*end||pid>std::numeric_limits<DWORD>::max())return 1;
 const std::filesystem::path target=argv[3],candidate=argv[4],helper=SelfPath();const std::wstring hashArg=argv[5];std::string expected;
 for(wchar_t c:hashArg){if(c>127)return 1;expected.push_back(static_cast<char>(c));}
 const std::wstring versionArg=argv[6];std::string versionText;for(wchar_t c:versionArg){if(c>127)return 1;versionText.push_back(static_cast<char>(c));}const auto expectedVersion=UpdatePolicy::ParseVersion(versionText);
 if(!IsBoull(target)||!TempName(candidate,target.parent_path(),L".tmp")||!TempName(helper,target.parent_path(),L".exe")||!UpdatePolicy::IsSha256(expected)||!expectedVersion)return 1;
 KernelHandle parent(OpenProcess(SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,static_cast<DWORD>(pid)));if(!parent.h||parent.h==INVALID_HANDLE_VALUE)return 1;
 wchar_t parentPath[32768]{};DWORD length=32768;if(!QueryFullProcessImageNameW(parent.h,0,parentPath,&length)||!SamePath(parentPath,target))return 1;
 wchar_t* handleEnd=nullptr;const unsigned long long rawHandle=wcstoull(argv[7],&handleEnd,10);if(!rawHandle||!handleEnd||*handleEnd)return 1;
 KernelHandle ready(reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(rawHandle)));if(!SetEvent(ready.h))return 1;
 if(WaitForSingleObject(parent.h,30000)!=WAIT_OBJECT_0)return 1;
 ByteVector data;if(!ReadFileBytes(candidate,data)||Sha256(data)!=expected||!IsPortableExecutable(candidate)||BinaryVersion(candidate)!=expectedVersion){DeleteFileW(candidate.c_str());MessageBoxW(nullptr,L"Update verification failed; the previous version was kept.",L"Boull",MB_OK|MB_ICONERROR);Spawn(target,L"--post-update "+Quote(helper));return 1;}
 if(!MoveFileExW(candidate.c_str(),target.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){DeleteFileW(candidate.c_str());MessageBoxW(nullptr,L"Could not replace the executable; the previous version was kept.",L"Boull",MB_OK|MB_ICONERROR);Spawn(target,L"--post-update "+Quote(helper));return 1;}
 if(!Spawn(target,L"--post-update "+Quote(helper))){MessageBoxW(nullptr,L"The update was installed. Open Boull again.",L"Boull",MB_OK|MB_ICONINFORMATION);return 1;}
 return 0;
}
void CleanupUpdateCommand(int argc,wchar_t** argv){
 if(argc!=3)return;const auto self=SelfPath();const std::filesystem::path helper=argv[2];if(!IsBoull(self)||!TempName(helper,self.parent_path(),L".exe"))return;
 for(int i=0;i<40;++i){if(DeleteFileW(helper.c_str())||GetLastError()==ERROR_FILE_NOT_FOUND)break;Sleep(50);}
}


