#include "volume_handler.h"
#include <QDebug>

HHOOK VolumeHandler::hHook = nullptr;
VolumeHandler *VolumeHandler::instance = nullptr;

VolumeHandler::VolumeHandler(QObject *parent) : QObject(parent) {
	instance = this;
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
}

LRESULT CALLBACK VolumeHandler::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode == HC_ACTION) {
		KBDLLHOOKSTRUCT *pKey = (KBDLLHOOKSTRUCT *)lParam;
		if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
			if (pKey->vkCode == VK_VOLUME_UP || pKey->vkCode == VK_VOLUME_DOWN) {
				bool isShift = (GetAsyncKeyState(VK_SHIFT) & 0x8000);
				int delta = (pKey->vkCode == VK_VOLUME_UP) ? (isShift ? 1 : 5) : (isShift ? -1 : -5);

				if (instance) {
					emit instance->volumeChanged(delta);
				}
				return 1; // Suppress the volume key
			}
		}
	}

	return CallNextHookEx(hHook, nCode, wParam, lParam);
}
