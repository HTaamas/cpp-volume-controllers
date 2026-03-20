#include "volume_handler_win.h"
#include <QDebug>

HHOOK VolumeHandlerWin::hHook = nullptr;

VolumeHandlerWin::VolumeHandlerWin(QObject *parent) : VolumeHandler(parent) {
    instance = this;
    hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
    if (!hHook) {
        qDebug() << "Failed to install keyboard hook!";
    }
}

VolumeHandlerWin::~VolumeHandlerWin() {
    if (hHook) {
        UnhookWindowsHookEx(hHook);
    }
}

LRESULT CALLBACK VolumeHandlerWin::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT *pKey = (KBDLLHOOKSTRUCT *)lParam;
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            if (pKey->vkCode == VK_VOLUME_UP || pKey->vkCode == VK_VOLUME_DOWN) {
                bool isShift = (GetAsyncKeyState(VK_SHIFT) & 0x8000);
                int delta = (pKey->vkCode == VK_VOLUME_UP) ? (isShift ? 1 : 5) : (isShift ? -1 : -5);
                
                emit instance->volumeChanged(delta);
                return 1; // Suppress the volume key
            }
        }
    }
    return CallNextHookEx(hHook, nCode, wParam, lParam);
}
