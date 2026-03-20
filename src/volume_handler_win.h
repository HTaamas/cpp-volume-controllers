#ifndef VOLUME_HANDLER_WIN_H
#define VOLUME_HANDLER_WIN_H

#include "volume_handler.h"
#include <windows.h>

class VolumeHandlerWin : public VolumeHandler {
    Q_OBJECT
public:
    explicit VolumeHandlerWin(QObject *parent = nullptr);
    ~VolumeHandlerWin() override;

private:
    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    static HHOOK hHook;
};

#endif // VOLUME_HANDLER_WIN_H
