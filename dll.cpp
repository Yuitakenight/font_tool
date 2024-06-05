#include <windows.h>
#include "detours.h"

#pragma comment(lib, "detours.lib")

LOGFONT originalLF; 

VOID __declspec(dllexport) CN()
{

}

static decltype(&SelectObject) TrueSelectObject = SelectObject;
static decltype(&MultiByteToWideChar) TrueMultiByteToWideChar = MultiByteToWideChar;
static decltype(&WideCharToMultiByte) TrueWideCharToMultiByte = WideCharToMultiByte;

DWORD WINAPI RandomDelay()
{
    DWORD delay = rand() % 1000 + 500; // 500ms ~ 1500ms 
    Sleep(delay);
    return delay;
}

int WINAPI MyMultiByteToWideChar(
    UINT CodePage,
    DWORD dwFlags,
    LPCSTR lpMultiByteStr,
    int cbMultiByte,
    LPWSTR lpWideCharStr,
    int cchWideChar)
{
    CodePage = 950;
    return TrueMultiByteToWideChar(CodePage, dwFlags, lpMultiByteStr, cbMultiByte, lpWideCharStr, cchWideChar);
}

int WINAPI MyWideCharToMultiByte(
    UINT CodePage,
    DWORD dwFlags,
    LPCWSTR lpWideCharStr,
    int cchWideChar,
    LPSTR lpMultiByteStr,
    int cbMultiByte,
    LPCSTR lpDefaultChar,
    LPBOOL lpUsedDefaultChar)
{
    CodePage = 950; // BIG5 
    return TrueWideCharToMultiByte(CodePage, dwFlags, lpWideCharStr, cchWideChar, lpMultiByteStr, cbMultiByte, lpDefaultChar, lpUsedDefaultChar);
}

HGDIOBJ WINAPI MySelectObject(HDC hdc, HGDIOBJ h)
{
    LOGFONT lf;
    if (GetObject(h, sizeof(LOGFONT), &lf))
    {
        if (lf.lfCharSet == DEFAULT_CHARSET) 
        {
            wcscpy_s(lf.lfFaceName, _countof(lf.lfFaceName), L"DFHei-GB5"); 

            HFONT hNewFont = CreateFontIndirect(&lf);
            if (hNewFont != NULL)
            {
                HGDIOBJ hOldFont = SelectObject(hdc, hNewFont); 
                
                DeleteObject(h); 

                return hOldFont; 
            }
        }
    }

    return TrueSelectObject(hdc, h); 
}

bool InstallHooks()
{
    DetourRestoreAfterWith();

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)TrueSelectObject, MySelectObject);
    DetourAttach(&(PVOID&)TrueMultiByteToWideChar, MyMultiByteToWideChar);
    DetourAttach(&(PVOID&)TrueWideCharToMultiByte, MyWideCharToMultiByte);

    if (DetourTransactionCommit() == NO_ERROR)
    {
        return true;
    }
    return false;
}

void RemoveHooks()
{
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourDetach(&(PVOID&)TrueSelectObject, MySelectObject);
    DetourDetach(&(PVOID&)TrueMultiByteToWideChar, MyMultiByteToWideChar);
    DetourDetach(&(PVOID&)TrueWideCharToMultiByte, MyWideCharToMultiByte);

    DetourTransactionCommit();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        InstallHooks();
        break;
    case DLL_PROCESS_DETACH:
        RemoveHooks();
        break;
    }
    return TRUE;
}
