#include "volume_handler.h"

VolumeHandler *VolumeHandler::instance = nullptr;

VolumeHandler::VolumeHandler(QObject *parent) : QObject(parent) {
}
