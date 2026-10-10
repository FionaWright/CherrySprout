#ifndef H_HELPER_H
#define H_HELPER_H

#ifdef CHERRY_PRINT_ENABLED
#   define CherryPrint(str) std::cout << str << std::endl
#   define CherryPrintW(str) std::wcout << str << std::endl
#else
#   define CherryPrint(str)
#   define CherryPrintW(str)
#endif

#if defined(_MSVC_LANG)
    #define CHERRY_CXX_VERSION _MSVC_LANG
#else
    #define CHERRY_CXX_VERSION __cplusplus
#endif

#include "CppUtils.h"

inline void CherryAssert(const bool expr, const char* message = nullptr)
{
#ifdef CHERRY_ASSERT_ENABLED
    if (!expr)
    {
        if (message)
        {
            CherryPrint("Assertion failed: '" << message << "'");
            throw std::exception((std::string("Assertion failed: '") + message + "'\n").c_str());
        }
        else
        {
            CherryPrint("Assertion failed!");
            throw std::exception(std::string("Assertion failed!").c_str());
        }
    }
#else

#if CHERRY_CXX_VERSION >= 202302L && defined(__has_cpp_attribute) && __has_cpp_attribute(assume) >= 202207L
    [[assume(expr)]];
#elif defined(_MSC_VER)
    __assume(expr);
#endif

#endif
}

class HrException final : public std::runtime_error
{
public:
    explicit HrException(HRESULT hr) : std::runtime_error(HrToString(hr)), m_hr(hr) {}
    [[nodiscard]] HRESULT Error() const { return m_hr; }
private:
    const HRESULT m_hr;
};

inline void V(const HRESULT hr)
{
    if (FAILED(hr))
    {
        CherryPrint("[ERROR]: " + HrToString(hr));
        throw HrException(hr);
    }
}

#if !defined(NDEBUG) || defined(DBG)
inline void SetResourceName(ID3D12Object* pObject, LPCWSTR name)
{
    V(pObject->SetName(name));
}
inline void SetNameIndexed(ID3D12Object* pObject, LPCWSTR name, UINT index)
{
    WCHAR fullName[50];
    if (swprintf_s(fullName, L"%s[%u]", name, index) > 0)
    {
        V(pObject->SetName(fullName));
    }
}
#else
inline void SetName(ID3D12Object*, LPCWSTR)
{
}
inline void SetNameIndexed(ID3D12Object*, LPCWSTR, UINT)
{
}
#endif

// Naming helper for ComPtr<T>.
// Assigns the name of the variable as the name of the object.
// The indexed variant will include the index in the name of the object.
#define NAME_D3D12_OBJECT(x) SetResourceName((x).Get(), L"" #x)
#define NAME_D3D12_OBJECT_INDEXED(x, n) SetNameIndexed((x)[n].Get(), L"" #x, n)

inline UINT CalculateConstantBufferByteSize(UINT byteSize)
{
    // Constant buffer size is required to be aligned.
    return (byteSize + (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1)) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);
}

// Resets all elements in a ComPtr array.
template<class T>
void ResetComPtrArray(T* comPtrArray)
{
    for (auto &i : *comPtrArray)
    {
        i.Reset();
    }
}

// Resets all elements in a unique_ptr array.
template<class T>
void ResetUniquePtrArray(T* uniquePtrArray)
{
    for (auto &i : *uniquePtrArray)
    {
        i.reset();
    }
}

#endif