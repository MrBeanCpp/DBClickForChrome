// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include<Windows.h>

#pragma data_seg(".SHARE")//共享数据段
HWND hWndServer = NULL;
HWND hWndTarget = NULL;
HHOOK hMouse = NULL;
#pragma data_seg()
#pragma comment(linker, "/section:.SHARE,rws")

HINSTANCE hInstance;
UINT UWM_MOUSEHOOK;
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            hInstance = hModule;
            UWM_MOUSEHOOK = RegisterWindowMessage(L"Chrome_WH_MOUSE");
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}


extern "C" _declspec(dllexport)
int test(int a) {
    return a + 1;
}

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && hWndServer) {
        MOUSEHOOKSTRUCT* data = (MOUSEHOOKSTRUCT*)lParam;
        PostMessage(hWndServer, UWM_MOUSEHOOK, wParam, (LPARAM)data->hwnd);//PostMessage不能发送指针
    }

    return CallNextHookEx(hMouse, nCode, wParam, lParam);
}

extern "C" _declspec(dllexport)
bool setMouseHook(HWND hWnd, HWND target, DWORD* errorCode)
{
    if (hWndServer && hWndServer != hWnd) {
        *errorCode = 985211;
        return false;
    }
    hMouse = SetWindowsHookEx(WH_MOUSE, (HOOKPROC)MouseProc, hInstance, GetWindowThreadProcessId(hWndTarget = target, NULL));
    if (hMouse) {
        hWndServer = hWnd;
        return true;
    }
    *errorCode = GetLastError();
    return false;
}

extern "C" _declspec(dllexport)
bool clearHook(void)
{
	hWndServer = NULL;
	hMouse = NULL;
    return UnhookWindowsHookEx(hMouse);//若hook已被自动卸载 也会返回false
}