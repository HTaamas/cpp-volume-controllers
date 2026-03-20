#ifndef VOLUME_HANDLER_FACTORY_H
#define VOLUME_HANDLER_FACTORY_H

#ifdef Q_OS_MAC
#include "volume_handler_mac.h"
typedef VolumeHandlerMac VolumeHandlerImpl;
#elif defined(Q_OS_WIN)
#include "volume_handler_win.h"
typedef VolumeHandlerWin VolumeHandlerImpl;
#else
#include "volume_handler.h"
typedef VolumeHandler VolumeHandlerImpl;
#endif

#endif // VOLUME_HANDLER_FACTORY_H
