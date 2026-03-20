#ifndef VOLUME_HANDLER_H
#define VOLUME_HANDLER_H

#include <QObject>
#include <windows.h>

class VolumeHandler : public QObject {
    Q_OBJECT
public:
    explicit VolumeHandler(QObject *parent = nullptr);
    ~VolumeHandler() override;

signals:
    void volumeChanged(int delta);

private:
    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    static HHOOK hHook;
    static VolumeHandler *instance;
};

#endif // VOLUME_HANDLER_H
