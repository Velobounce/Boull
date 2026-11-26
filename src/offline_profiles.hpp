#pragma once
#include <windows.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace OfflineProfiles {
inline std::vector<std::string> profiles;
inline std::string current, status="Use offline profiles from the main menu.";
inline char input[17]{};
inline std::string requested;
inline bool loaded=false, restoreRequested=false;
inline bool Valid(const std::string& name) {
 if(name.size()<3 || name.size()>16)return false;
 for(unsigned char c:name)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'))return false;
 return true;
}
inline std::filesystem::path Path(){wchar_t value[32768]{};DWORD n=GetEnvironmentVariableW(L"LOCALAPPDATA",value,32768);if(!n||n>=32768)return {};return std::filesystem::path(value)/L"Boull"/L"profiles.txt";}
inline void Load(){if(loaded)return;loaded=true;auto path=Path();if(path.empty()){status="Could not access local profiles.";return;}std::ifstream f(path);std::string name;while(std::getline(f,name)&&profiles.size()<64){if(!name.empty()&&name.back()=='\r')name.pop_back();if(Valid(name)&&std::none_of(profiles.begin(),profiles.end(),[&](const auto& s){return _stricmp(s.c_str(),name.c_str())==0;}))profiles.push_back(name);}}
inline bool Save(){auto path=Path();if(path.empty())return false;std::error_code ec;std::filesystem::create_directories(path.parent_path(),ec);if(ec)return false;auto temp=path;temp+=L".tmp";{std::ofstream f(temp,std::ios::trunc);if(!f)return false;for(const auto& s:profiles)f<<s<<'\n';f.flush();if(!f)return false;}return MoveFileExW(temp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=FALSE;}
inline bool Add(const std::string& name){if(!Valid(name)){status="Invalid nickname: use 3-16 letters, numbers or _.";return false;}if(std::none_of(profiles.begin(),profiles.end(),[&](const auto& s){return _stricmp(s.c_str(),name.c_str())==0;})){if(profiles.size()>=64){status="Maximum of 64 profiles reached.";return false;}profiles.push_back(name);if(!Save()){profiles.pop_back();status="Could not save profiles.";return false;}}return true;}
inline void Remove(size_t i){if(i>=profiles.size())return;auto old=profiles;profiles.erase(profiles.begin()+i);if(!Save()){profiles=std::move(old);status="Could not save profiles.";}}
}


