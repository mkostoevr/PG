#define _WIN32_WINNT 0x0400
#pragma comment(lib, "user32")

#include <windows.h>
#include <stdio.h>

HHOOK hKeyboardHook;

char *generate_password(const char *seed) {
    char buf[256] = { 0 };
    sprintf(buf, "%s%d", seed, seed[0]);
    char *output = malloc(strlen(buf));
    strcpy(output, buf);
    return output;
}

LRESULT CALLBACK KeyboardEvent(int nCode, WPARAM wParam, LPARAM lParam) {
    if  ((nCode == HC_ACTION) && ((wParam == WM_SYSKEYDOWN) || (wParam == WM_KEYDOWN))) {
        KBDLLHOOKSTRUCT hooked_key = *((KBDLLHOOKSTRUCT*)lParam);

        DWORD dwMsg = 1;
        dwMsg += hooked_key.scanCode << 16;
        dwMsg += hooked_key.flags << 24;

        int key = hooked_key.vkCode;

        DWORD SHIFT_key = GetAsyncKeyState(VK_SHIFT);
        DWORD CTRL_key = GetAsyncKeyState(VK_CONTROL);
        DWORD ALT_key = GetAsyncKeyState(VK_MENU);
        if (CTRL_key && ALT_key && key == 'P') {
            if (!OpenClipboard(NULL)) {
                goto RETURN_DEFAULT;
            }

            HANDLE hData = GetClipboardData(CF_TEXT);
            if (hData == NULL) {
                goto RELEASE_CLIPBOARD;
            }

            char *pszText = GlobalLock(hData);
            if (pszText == NULL) {
                goto RELEASE_CLIPBOARD;
            }

            {
                const char *output = generate_password(pszText);
                if (output == NULL) {
                    goto UNLOCK_CLIPBOARD_DATA;
                }
                const size_t len = lstrlenA(output) + 1;

                HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
                if (hMem == NULL) {
                    goto UNLOCK_CLIPBOARD_DATA;
                }

                char *lock = GlobalLock(hMem);
                if (lock == NULL) {
                    goto UNLOCK_CLIPBOARD_DATA;
                }
                strcpy(lock, output);
                GlobalUnlock(hMem);
                EmptyClipboard();
                SetClipboardData(CF_TEXT, hMem);
            }

UNLOCK_CLIPBOARD_DATA:
            GlobalUnlock(hData);

RELEASE_CLIPBOARD:
            CloseClipboard();

            SHIFT_key = 0;
            CTRL_key = 0;
            ALT_key = 0;
        }
    }
RETURN_DEFAULT:
    return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

DWORD WINAPI my_HotKey(LPVOID lpParm) {
    (void)lpParm;

    HINSTANCE hInstance = GetModuleHandle(NULL);
    if (!hInstance) {
        return 1;
    }

    hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)KeyboardEvent, hInstance, 0);
    {
        MSG message;
        while (GetMessage(&message,NULL,0,0)) {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
    }
    UnhookWindowsHookEx(hKeyboardHook);
    return 0;
}

int main() {
    HANDLE hThread;
    DWORD dwThread;

    hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)my_HotKey, NULL, 0, &dwThread);
    ShowWindow(FindWindowA("ConsoleWindowClass", NULL), 0);
    if (hThread) {
        return WaitForSingleObject(hThread, INFINITE);
    }
    return 1;
}
