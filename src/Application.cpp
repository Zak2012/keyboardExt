// Include standard libraries
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <bitset>
#include <ctime>
#include <fstream>
#include <algorithm>
// #include <condition_variable>
// #include <mutex>

#undef WINVER
#define WINVER NTDDI_WIN10_19H1

#undef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_WIN10

#include <windows.h>
#include <Ntddvdeo.h>
// #include <Initguid.h>
#include <powrprof.h>
#include <shellapi.h>
#include <shlobj.h>
#include <knownfolders.h>

static LRESULT LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

static HHOOK LLKeyboardHook;

static std::bitset<256> keys;

static std::vector<GUID> Power(3);

static HANDLE MonitorHndl;

static std::string UserScreenshotFolder;

//https://davidxl.blogspot.com/2010/07/controla-el-nivel-de-backlight-de-una.html
int8_t GetBrightness()
{
    int Bytes = 0;
    DISPLAY_BRIGHTNESS DisplayBrightness;

    DeviceIoControl(
        (HANDLE) MonitorHndl,                // handle to device
        IOCTL_VIDEO_QUERY_DISPLAY_BRIGHTNESS,  // dwIoControlCode
        NULL,                            // lpInBuffer
        0,                               // nInBufferSize
        (LPVOID) &DisplayBrightness,            // output buffer
        (DWORD) sizeof(DisplayBrightness),          // size of output buffer
        (LPDWORD) &Bytes,       // number of bytes returned
        NULL      // OVERLAPPED structure
    );
    return DisplayBrightness.ucDCBrightness;
}

void SetBrightness(uint8_t Level)
{
    int Bytes = 0;
    DISPLAY_BRIGHTNESS DisplayBrightness = {2, Level, Level};
    DeviceIoControl(
        (HANDLE) MonitorHndl,                // handle to device
        IOCTL_VIDEO_SET_DISPLAY_BRIGHTNESS,  // dwIoControlCode
        &DisplayBrightness,                            // lpInBuffer
        sizeof(DisplayBrightness),                               // nInBufferSize
        NULL,            // output buffer
        0,          // size of output buffer
        (LPDWORD) &Bytes,       // number of bytes returned
        NULL      // OVERLAPPED structure
    );
}

void Cleanup()
{
    UnhookWindowsHookEx(LLKeyboardHook);
}

int main(int iArgCnt, char ** argv)
{
    MonitorHndl =  CreateFileA(
        "\\\\.\\LCD", 
        GENERIC_READ | GENERIC_WRITE, 
        FILE_SHARE_READ | FILE_SHARE_WRITE, 
        NULL, 
        OPEN_EXISTING, 
        0, //FILE_FLAG_OVERLAPPED , 
        NULL
    );

    if (MonitorHndl == INVALID_HANDLE_VALUE)
    {
        std::cout << "error:" << GetLastError() << "\n";
        return 1;
    }

    GUID buffer;
    DWORD bufferSize = sizeof(buffer);
    int index = 0;


    while (PowerEnumerate(NULL, NULL, NULL, ACCESS_SCHEME, index, (UCHAR*)&buffer, &bufferSize) == ERROR_SUCCESS)
    {
        index++;
        char GetName[1024];
        uint32_t GetNameBufferSize = sizeof(GetName);

        // Balanced = B, \0, a, \0, l ...
        PowerReadFriendlyName(
                                NULL, 
                                &buffer, 
                                &NO_SUBGROUP_GUID, 
                                NULL, 
                                (uint8_t *)GetName, 
                                (DWORD *)&GetNameBufferSize);
        
        if (GetName[0] == 'P')
        {
            Power[0] = buffer;
            // std::cout << GetName << "\n";
        }
        else if (GetName[0] == 'B')
        {
            Power[1] = buffer;
            // std::cout << GetName << "\n";
        }
        else if (GetName[0] == 'H')
        {
            Power[2] = buffer;
            // std::cout << GetName << "\n";
        }
    }

    LLKeyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
    MSG msg = {0};

    std::atexit(Cleanup);

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        // if (ScreenshotCounter > 0)
        // {
        //     for (unsigned int i = 0; i < Threads.size(); i++)
        //     {
        //         if (Threads[i].IsReady())
        //         {
        //             std::cout << "Running on thread " << i << "\n";
        //             Threads[i].Data((void *)"");
        //             Threads[i].Exec();
        //             ScreenshotCounter--;
        //             break;
        //         }
        //     }
        // }
    }

    return 0;
}

bool HoldKey(unsigned int vKey){
    INPUT inputs[1] = {};
    ZeroMemory(inputs, sizeof(inputs));
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = vKey;
    return SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
}

bool PressKey(unsigned int vKey){
    INPUT inputs[2] = {};
    ZeroMemory(inputs, sizeof(inputs));
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = vKey;
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = vKey;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
    return SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));

}

bool ReleaseKey(unsigned int vKey){
    INPUT inputs[1] = {};
    ZeroMemory(inputs, sizeof(inputs));
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = vKey;
    inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;
    return SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
}

static LRESULT LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    KBDLLHOOKSTRUCT *LLKbd = (KBDLLHOOKSTRUCT *)lParam;
    // int key = LLKeyboard->vkCode;
    if (nCode == HC_ACTION)
    {
        if (wParam == WM_KEYUP)
        {
            
            keys[LLKbd->vkCode] = false;
            switch (LLKbd->vkCode)
            {
                case VK_F1:
                    return 1;
                case VK_F2:
                    ReleaseKey(VK_VOLUME_DOWN);
                    return 1;
                case VK_F3:
                    ReleaseKey(VK_VOLUME_UP);
                    return 1;
                case VK_F5:
                    return 1;
                case VK_F7:
                    return 1;
                case VK_F8:
                    return 1;
                case VK_F9:
                    return 1;
                case VK_F10:
                    return 1;
                case VK_F11:
                    return 1;
            }
        }
        if (wParam == WM_KEYDOWN)
        {
            bool Lock = LOWORD(GetKeyState(VK_SCROLL));
            bool KeyState = keys[LLKbd->vkCode];
            keys[LLKbd->vkCode] = true;
            // std::cout << LLKbd->vkCode << "\n";
            if (!Lock)
            {

                switch (LLKbd->vkCode)
                {
                    case VK_F1:
                        if (!KeyState)
                        {
                            PressKey(VK_VOLUME_MUTE);
                        }
                        return 1;
                    case VK_F2:
                        HoldKey(VK_VOLUME_DOWN);
                        return 1;
                    case VK_F3:
                        HoldKey(VK_VOLUME_UP);
                        return 1;
                    case VK_F5:
                        if (!KeyState)
                        {
                            PressKey(VK_MEDIA_PLAY_PAUSE);
                        }
                        return 1;
                    case VK_F7:
                            SetBrightness(std::max(0,GetBrightness() - 2));
                        return 1;
                    case VK_F8:
                            SetBrightness(std::min(100,GetBrightness() + 2));
                        return 1;
                    case VK_F9:
                        if (!KeyState)
                        {
                            PowerSetActiveScheme(NULL, &(Power[0]));
                        }
                        return 1;
                    case VK_F10:
                        if (!KeyState)
                        {
                            PowerSetActiveScheme(NULL, &(Power[1]));
                        }
                        return 1;
                    case VK_F11:
                        if (!KeyState)
                        {
                            PowerSetActiveScheme(NULL, &(Power[2]));
                        }
                        return 1;
                }
            }
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}