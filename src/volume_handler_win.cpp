#include "volume_handler.h"
#include <QDebug>
#include <QElapsedTimer>

#ifdef _WIN32
HHOOK VolumeHandler::hHook = nullptr;
bool VolumeHandler::duckingToggleChordDown = false;

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
    const bool isShift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    const bool isCtrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    static QElapsedTimer nextTrackTimer;
    static QElapsedTimer prevTrackTimer;
    const int doubleTapTimeout = 400;

    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT *pKey = reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);
        if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
            if (pKey->vkCode == 'D') {
                duckingToggleChordDown = false;
            }
        }

        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            if (pKey->vkCode == 'D' && (GetAsyncKeyState(VK_MENU) & 0x8000) != 0) {
                if (!duckingToggleChordDown && instance) {
                    duckingToggleChordDown = true;
                    emit instance->toggleDuckingRequested();
                }
                return 1;
            }

            if (pKey->vkCode == instance->keybindSettings.mainKey.toInt(nullptr, 16)) {
                if (instance) {
                    if (isShift && isCtrl) {
                        return 0;
                    } else if (isShift) {
                        if (!nextTrackTimer.isValid() || nextTrackTimer.elapsed() > doubleTapTimeout) {
                            nextTrackTimer.start();
                        } else {
                            nextTrackTimer.invalidate();
                            emit instance->nextTrack();
                        }
                    } else if (isCtrl) {
                        if (!prevTrackTimer.isValid() || prevTrackTimer.elapsed() > doubleTapTimeout) {
                            prevTrackTimer.start();
                        } else {
                            prevTrackTimer.invalidate();
                            emit instance->prevTrack();
                        }
                    } else {
                        emit instance->toggleMusic();
                    }
                }

                return 1;
            }

            if (pKey->vkCode == VK_VOLUME_UP || pKey->vkCode == VK_VOLUME_DOWN) {
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
