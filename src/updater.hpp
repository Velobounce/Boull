#pragma once
#include <windows.h>
#include <string>
enum class UpdateCheck { NoUpdate, Started, Failed };
UpdateCheck CheckForUpdate(std::wstring& error);
int ApplyUpdateCommand(int argc,wchar_t** argv);
void CleanupUpdateCommand(int argc,wchar_t** argv);
