#ifndef H_CPP_UTILS_H
#define H_CPP_UTILS_H
#include <string>

inline std::string wstringToString(const std::wstring& wstr)
{
    if (wstr.empty()) return {};

    const int size = WideCharToMultiByte(
        CP_UTF8, 0,
        wstr.data(), (int)wstr.size(),
        nullptr, 0,
        nullptr, nullptr
    );

    std::string result(size, 0);
    WideCharToMultiByte(
        CP_UTF8, 0,
        wstr.data(), (int)wstr.size(),
        result.data(), size,
        nullptr, nullptr
    );

    return result;
}

inline std::wstring stringToWString(const std::string& str)
{
    const int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    std::wstring wstr(size_needed, 0);

    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size_needed);

    wstr.pop_back(); // Remove null terminator added by MultiByteToWideChar
    return wstr;
}

inline std::string HrToString(HRESULT hr)
{
    char s_str[64] = {};
    sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
    return std::string(s_str);
}

#endif