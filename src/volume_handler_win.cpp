#include "volume_handler.h"
#include <QDebug>

#ifdef _WIN32
HHOOK VolumeHandler::hHook = nullptr;

VolumeHandler::VolumeHandler(QObject *parent) : QObject(parent) {
    instance = this;
    keybindSettings = AppSettings::loadKeybindSettings();

    hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
    if (!hHook) {
        qDebug() << "Failed to install keyboard hook!";
    }
}

VolumeHandler::~VolumeHandler() {
    if (hHook) {
        UnhookWindowsHookEx(hHook);
        hHook = nullptr;
    }

    if (instance == this) {
        instance = nullptr;
    }
}

LRESULT CALLBACK VolumeHandler::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT *pKey = reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            if (pKey->vkCode == VK_VOLUME_UP || pKey->vkCode == VK_VOLUME_DOWN) {
                const bool isShift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
                const bool useFineStep = instance && instance->keybindSettings.useShiftForFineAdjust && isShift;
                const int step = instance ? (useFineStep ? instance->keybindSettings.fineStep : instance->keybindSettings.coarseStep) : 5;
                const int delta = (pKey->vkCode == VK_VOLUME_UP) ? step : -step;

                if (instance) {
                    emit instance->volumeChanged(delta);
                }
                return 1;
            }
        }
    }

    return CallNextHookEx(hHook, nCode, wParam, lParam);
}
#endif
