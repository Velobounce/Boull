#pragma once
#include <string>
struct EmbeddedRuntimeResult { std::wstring path; std::wstring error; };
EmbeddedRuntimeResult ExtractEmbeddedRuntime();
