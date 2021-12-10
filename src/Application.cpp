#include <iostream>
#include <chrono>
#include <thread>

#include <windows.h>
#include <strsafe.h>
#include <shellapi.h>

#include "keyboardExt.rc"

#define MAX_LOADSTRING 100
#define	WM_USER_SHELLICON WM_USER + 1

static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WindowProc(HWND   hwnd, UINT   uMsg, WPARAM wParam, LPARAM lParam);

void sleep(int timems);


HINSTANCE hInstance = GetModuleHandle(0);
HWND hWnd;
HHOOK hHook;

HMENU hPopMenu;
HICON hIconEnable;
HICON hIconDisable;
NOTIFYICONDATAA iconEnable;
NOTIFYICONDATAA iconDisable;

bool isEnable = true;
bool keys[256];

int main(int argc, char* argv[]){

    // Register the window class.
    const char CLASS_NAME[]  ="KeyExt";

    WNDCLASS wc = {0};

    wc.style		 = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    hWnd = CreateWindowExA(
        0,
        CLASS_NAME,
        CLASS_NAME,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    // Load the icon for high DPI.
    hIconEnable = LoadIconA(hInstance, MAKEINTRESOURCE(IDI_ENABLE));
    if(hIconEnable == NULL){
        std::cout << "Error loading icon" << std::endl;
        ExitProcess(1);
    }

    hIconDisable = LoadIconA(hInstance, MAKEINTRESOURCE(IDI_DISABLE));
    if(hIconDisable == NULL){
        std::cout << "Error loading icon" << std::endl;
        ExitProcess(1);
    }
    //load enable icon to sys tray
    iconEnable = {};

    iconEnable.uVersion = NOTIFYICON_VERSION_4;
    iconEnable.cbSize = sizeof(iconEnable);
    iconEnable.hWnd = hWnd;
    iconEnable.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    iconEnable.uCallbackMessage = WM_USER_SHELLICON; 
    iconEnable.hIcon = hIconEnable;

    StringCchCopy(iconEnable.szTip, ARRAYSIZE(iconEnable.szTip), CLASS_NAME);

    //load enable icon to sys tray
    iconDisable = {};

    iconDisable.uVersion = NOTIFYICON_VERSION_4;
    iconDisable.cbSize = sizeof(iconDisable);
    iconDisable.hWnd = hWnd;
    iconDisable.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    iconDisable.uCallbackMessage = WM_USER_SHELLICON; 
    iconDisable.hIcon = hIconDisable;

    StringCchCopy(iconDisable.szTip, ARRAYSIZE(iconDisable.szTip), CLASS_NAME);

    // free icon handles
    DestroyIcon(hIconEnable);
    DestroyIcon(hIconDisable);

    Shell_NotifyIconA(NIM_ADD, &iconEnable);

    hHook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);

    MSG msg = {0};

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(hHook);
    Shell_NotifyIcon(NIM_DELETE,&iconEnable);
    return 0;
}

void sleep(int timems){
    std::this_thread::sleep_for(std::chrono::milliseconds(timems));
}

void exitCleanup(){
    UnhookWindowsHookEx(hHook);
    Shell_NotifyIcon(NIM_DELETE,&iconEnable);
    ExitProcess(0);
}

void enableExt(){
    Shell_NotifyIconA(NIM_MODIFY, &iconEnable);
    isEnable = true;
}

void disableExt(){
    Shell_NotifyIconA(NIM_MODIFY, &iconDisable);
   isEnable = false;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
    POINT lpClickPoint;
    switch (uMsg)
    {
    case WM_USER_SHELLICON: 

        // systray msg callback 
        switch(LOWORD(lParam)) 
        {
        case WM_LBUTTONDOWN:
            unsigned int uFlag = MF_BYPOSITION|MF_STRING;
            GetCursorPos(&lpClickPoint);
            hPopMenu = CreatePopupMenu();
            if(isEnable)
                InsertMenuA(hPopMenu,0xFFFFFFFF,uFlag,IDM_DISABLE,"Disable (win + esc)");
            else
                InsertMenuA(hPopMenu,0xFFFFFFFF,uFlag,IDM_ENABLE,"Enable (win + esc)");


            InsertMenuA(hPopMenu,0xFFFFFFFF,MF_SEPARATOR,IDM_SEP,NULL);
            InsertMenuA(hPopMenu,0xFFFFFFFF,uFlag,IDM_QUIT,"Quit (win + del)");

            SetForegroundWindow(hWnd);
            TrackPopupMenu(hPopMenu,TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_BOTTOMALIGN,lpClickPoint.x, lpClickPoint.y,0,hWnd,NULL);
            return true; 
        }
        break;

    case WM_COMMAND:
        // Parse the menu selections:
        switch (LOWORD(wParam))
        {
        case IDM_QUIT:
            exitCleanup();
            break;
        case IDM_ENABLE:
            enableExt();
            break;
        case IDM_DISABLE:
            disableExt();
            break;
        default:
            return DefWindowProcA(hWnd, uMsg, wParam, lParam);
        }
        break;

    case WM_CLOSE:
        exitCleanup();
        break;

    default:
        return DefWindowProcA(hWnd, uMsg, wParam, lParam);
    }
}

bool pressKey(unsigned int vKey){
    INPUT inputs[1] = {};
    ZeroMemory(inputs, sizeof(inputs));
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = vKey;
    return SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));

}

bool releaseKey(unsigned int vKey){
    INPUT inputs[1] = {};
    ZeroMemory(inputs, sizeof(inputs));
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = vKey;
    inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;
    return SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
}

static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    int key = ((LPKBDLLHOOKSTRUCT)lParam)->vkCode;
    switch (wParam)
    {
    case WM_KEYDOWN:
        keys[key] = true;
        break;
    case WM_KEYUP:
        keys[key] = false;
        break;
    }
    if ((wParam == WM_KEYDOWN || wParam == WM_KEYUP) && lParam != NULL)
    {
        INPUT inputs[2] = {};
        ZeroMemory(inputs, sizeof(inputs));
        bool winKey = (keys[VK_LWIN] || keys[VK_RWIN]);
        if(winKey && keys[VK_ESCAPE])
        {
            isEnable ? disableExt() : enableExt();
            return 1;
        }
        else if(winKey && keys[VK_DELETE])
        {
            exitCleanup();
            return 1;
        }
        else if(keys[VK_F1] && (!isEnable != !winKey)){
            keys[VK_F1] = false;
            pressKey(VK_VOLUME_MUTE);
            releaseKey(VK_VOLUME_MUTE);
            return 1;
        }
        else if(keys[VK_F2] && (!isEnable != !winKey)){
            keys[VK_F2] = false;
            pressKey(VK_VOLUME_DOWN);
            releaseKey(VK_VOLUME_DOWN);
            return 1;
        }
        else if(keys[VK_F3] && (!isEnable != !winKey)){
            keys[VK_F3] = false;
            pressKey(VK_VOLUME_UP);
            releaseKey(VK_VOLUME_DOWN);
            return 1;
        }
        else if(keys[VK_F4] && (!isEnable != !winKey)){
            keys[VK_F4] = false;
            pressKey(VK_MEDIA_PREV_TRACK);
            releaseKey(VK_MEDIA_PREV_TRACK);
            return 1;
        }
        else if(keys[VK_F5] && (!isEnable != !winKey)){
            keys[VK_F5] = false;
            pressKey(VK_MEDIA_PLAY_PAUSE);
            releaseKey(VK_MEDIA_PLAY_PAUSE);
            return 1;
        }
        else if(keys[VK_F6] && (!isEnable != !winKey)){
            keys[VK_F6] = false;
            pressKey(VK_MEDIA_NEXT_TRACK);
            releaseKey(VK_MEDIA_NEXT_TRACK);
            return 1;
        }
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}
