#include "../src/updater.cpp"
#include <iostream>
int wmain(int argc,wchar_t** argv){
 if(argc>=2&&wcscmp(argv[1],L"--apply-update")==0)return ApplyUpdateCommand(argc,argv);
 if(argc==3&&wcscmp(argv[1],L"--post-update")==0){CleanupUpdateCommand(argc,argv);const auto marker=SelfPath().parent_path()/L"test-update-complete";KernelHandle file(CreateFileW(marker.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr));return file.h==INVALID_HANDLE_VALUE?11:0;}
 if(argc==4&&wcscmp(argv[1],L"--fixture-parent")==0){
  const std::wstring wideHash=argv[3];std::string expected;for(wchar_t c:wideHash){if(c>127)return 12;expected.push_back(static_cast<char>(c));}
  return StartUpdater(SelfPath(),argv[2],expected,BOULL_VERSION)?0:14;
 }
 if(argc==2&&wcscmp(argv[1],L"--network")==0){HttpResponse response;if(!Get(L"https://api.github.com/repos/Velobounce/Boull/releases/latest",1024u*1024u,response)){std::cerr<<"GitHub request failed\n";return 10;}std::cout<<"GitHub HTTP status: "<<response.status<<"\n";return 0;}
 const auto a=UpdatePolicy::ParseVersion("v0.2.0"),b=UpdatePolicy::ParseVersion("1.0.0");
 if(!a||!b||!(*b>*a)||UpdatePolicy::ParseVersion("1.0.0-rc1")||UpdatePolicy::ParseVersion("1.2"))return 1;
 const std::string sha(64,'a');
 const std::string text="{\"draft\":false,\"prerelease\":false,\"tag_name\":\"v0.3.0\",\"assets\":[{\"name\":\"Boull.exe\",\"digest\":\"sha256:"+sha+"\",\"browser_download_url\":\"https://github.com/Velobounce/Boull/releases/download/v0.3.0/Boull.exe\"}]}";
 const auto parsed=UpdatePolicy::ParseRelease(text);if(!parsed||parsed->sha256!=sha||parsed->version<=*a)return 2;
 auto bad=nlohmann::json::parse(text);bad["assets"][0]["browser_download_url"]="https://example.com/Boull.exe";if(UpdatePolicy::ParseRelease(bad.dump()))return 3;
 bad=nlohmann::json::parse(text);bad["assets"][0]["digest"]="sha256:abcd";if(UpdatePolicy::ParseRelease(bad.dump()))return 4;
 bad=nlohmann::json::parse(text);bad["prerelease"]=true;if(UpdatePolicy::ParseRelease(bad.dump())||UpdatePolicy::ParseRelease("not json"))return 5;
 ByteVector bytes={'a','b','c'};if(Sha256(bytes)!="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad")return 6;
 if(!ApprovedUrl(L"https://github.com/Velobounce/Boull")||!ApprovedUrl(L"https://release-assets.githubusercontent.com/example")||ApprovedUrl(L"http://github.com/Velobounce/Boull")||ApprovedUrl(L"https://github.com.example.com/"))return 7;
 wchar_t tempDir[MAX_PATH]{},tempFile[MAX_PATH]{};
 if(!GetTempPathW(MAX_PATH,tempDir)||!GetTempFileNameW(tempDir,L"BUT",0,tempFile))return 8;
 const auto executable=SelfPath().parent_path()/L"Boull.exe";
 const bool binaryOk=CopyFileW(executable.c_str(),tempFile,FALSE)&&IsPortableExecutable(tempFile)&&BinaryVersion(tempFile)==a;
 DeleteFileW(tempFile);if(!binaryOk){std::cerr<<"Executable validation rejected a temporary download\n";return 9;}
 const auto directory=SelfPath().parent_path()/L"update fixture spaces";std::error_code ec;std::filesystem::create_directories(directory,ec);if(ec)return 15;
 const auto target=directory/L"Boull.exe",marker=directory/L"test-update-complete";
 DeleteFileW(marker.c_str());if(!CopyFileW(SelfPath().c_str(),target.c_str(),FALSE))return 16;
 std::filesystem::path candidate;if(!TempFile(directory,candidate)||!CopyFileW(SelfPath().c_str(),candidate.c_str(),FALSE))return 17;
 {KernelHandle file(CreateFileW(candidate.c_str(),FILE_APPEND_DATA,0,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr));const char suffix[]="BOULL_FIXTURE_UPDATE";DWORD written=0;if(file.h==INVALID_HANDLE_VALUE||!WriteFile(file.h,suffix,sizeof(suffix),&written,nullptr)||written!=sizeof(suffix))return 18;}
 ByteVector candidateData;if(!ReadFileBytes(candidate,candidateData))return 19;const std::string candidateHash=Sha256(candidateData);
 std::wstring cmd=Quote(target)+L" --fixture-parent "+Quote(candidate)+L" "+Widen(candidateHash);STARTUPINFOW si{};si.cb=sizeof(si);PROCESS_INFORMATION pi{};
 if(!CreateProcessW(target.c_str(),cmd.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,directory.c_str(),&si,&pi))return 20;
 CloseHandle(pi.hThread);KernelHandle process(pi.hProcess);if(WaitForSingleObject(process.h,10000)!=WAIT_OBJECT_0)return 21;
 DWORD parentResult=1;if(!GetExitCodeProcess(process.h,&parentResult)||parentResult!=0)return 22;
 for(int i=0;i<200&&GetFileAttributesW(marker.c_str())==INVALID_FILE_ATTRIBUTES;++i)Sleep(50);
 ByteVector installed;if(GetFileAttributesW(marker.c_str())==INVALID_FILE_ATTRIBUTES||!ReadFileBytes(target,installed)||Sha256(installed)!=candidateHash){std::cerr<<"Isolated self-update did not complete\n";return 23;}
 DeleteFileW(marker.c_str());DeleteFileW(target.c_str());
 std::cout<<"PASS: isolated executable replacement and relaunch\n";
 std::cout<<"PASS: release version, URL, digest and SHA-256 validation\n";return 0;
}

