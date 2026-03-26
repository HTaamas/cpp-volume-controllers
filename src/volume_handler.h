#ifndef VOLUME_HANDLER_H
#define VOLUME_HANDLER_H

#include <QObject>

#ifdef _WIN32
#include <windows.h>
#endif

class VolumeHandler : public QObject {
    Q_OBJECT
public:
    explicit VolumeHandler(QObject *parent = nullptr);
    ~VolumeHandler() override;

signals:
    void volumeChanged(int delta);

private:
#ifdef _WIN32
    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    static HHOOK hHook;
#endif
    static VolumeHandler *instance;
};

#endif // VOLUME_HANDLER_H
