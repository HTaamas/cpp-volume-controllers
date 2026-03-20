#ifndef VOLUME_HANDLER_MAC_H
#define VOLUME_HANDLER_MAC_H

#include "volume_handler.h"
#include <CoreGraphics/CoreGraphics.h>

class VolumeHandlerMac : public VolumeHandler {
    Q_OBJECT
public:
    explicit VolumeHandlerMac(QObject *parent = nullptr);
    ~VolumeHandlerMac() override;

private:
    static CGEventRef eventCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon);
    CFMachPortRef eventTap = nullptr;
};

#endif // VOLUME_HANDLER_MAC_H
