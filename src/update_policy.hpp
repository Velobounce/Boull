#pragma once
#include <array>
#include <charconv>
#include <optional>
#include <string>
#include <string_view>
#include "../third_party/nlohmann/json.hpp"

namespace UpdatePolicy {
using Version=std::array<unsigned,3>;
struct Release {Version version;std::string tag,url,sha256;};
inline std::optional<Version> ParseVersion(std::string_view text){
 if(!text.empty()&&(text.front()=='v'||text.front()=='V'))text.remove_prefix(1);
 Version v{};for(size_t i=0;i<v.size();++i){const size_t end=text.find('.');const auto part=text.substr(0,end);if(part.empty()||part.size()>5)return std::nullopt;unsigned n=0;const auto result=std::from_chars(part.data(),part.data()+part.size(),n);if(result.ec!=std::errc{}||result.ptr!=part.data()+part.size()||n>65535)return std::nullopt;v[i]=n;if(i<2){if(end==std::string_view::npos)return std::nullopt;text.remove_prefix(end+1);}else if(end!=std::string_view::npos)return std::nullopt;}
 return v;
}
inline bool IsSha256(std::string_view s){if(s.size()!=64)return false;for(char c:s)if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F')))return false;return true;}
inline std::optional<Release> ParseRelease(std::string_view text){
 try {
  const auto j=nlohmann::json::parse(text.begin(),text.end());
  if(j.value("draft",true)||j.value("prerelease",true))return std::nullopt;
  const std::string tag=j.at("tag_name").get<std::string>();auto version=ParseVersion(tag);if(!version)return std::nullopt;
  const std::string expected="https://github.com/Velobounce/Boull/releases/download/"+tag+"/Boull.exe";
  for(const auto& asset:j.at("assets")){
   if(asset.value("name",std::string{})!="Boull.exe")continue;
   const std::string url=asset.value("browser_download_url",std::string{});
   const std::string digest=asset.value("digest",std::string{});
   if(url!=expected||digest.rfind("sha256:",0)!=0||!IsSha256(std::string_view(digest).substr(7)))return std::nullopt;
   return Release{*version,tag,url,digest.substr(7)};
  }
 }catch(const nlohmann::json::exception&){}
 return std::nullopt;
}
}

