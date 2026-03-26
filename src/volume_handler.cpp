#include "volume_handler.h"
#include <QDebug>

#ifdef _WIN32
HHOOK VolumeHandler::hHook = nullptr;
#endif
VolumeHandler *VolumeHandler::instance = nullptr;

VolumeHandler::VolumeHandler(QObject *parent) : QObject(parent) {
    instance = this;

#ifdef _WIN32
    hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
    if (!hHook) {
        qDebug() << "Failed to install keyboard hook!";
    }
#else
    qDebug() << "Global volume key interception is disabled on this platform.";
#endif
}

VolumeHandler::~VolumeHandler() {
#ifdef _WIN32
    if (hHook) {
        UnhookWindowsHookEx(hHook);
        hHook = nullptr;
    }
#endif

    if (instance == this) {
        instance = nullptr;
    }
}

#ifdef _WIN32
LRESULT CALLBACK VolumeHandler::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT *pKey = reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            if (pKey->vkCode == VK_VOLUME_UP || pKey->vkCode == VK_VOLUME_DOWN) {
                const bool isShift = (GetAsyncKeyState(VK_SHIFT) & 0x8000);
                const int delta = (pKey->vkCode == VK_VOLUME_UP) ? (isShift ? 1 : 5) : (isShift ? -1 : -5);

                if (instance) {
                    emit instance->volumeChanged(delta);
                }
                return 1; // Suppress the volume key
            }
        }
    }

    return CallNextHookEx(hHook, nCode, wParam, lParam);
}
#endif
