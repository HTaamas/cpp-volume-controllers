#ifndef VOLUME_HANDLER_H
#define VOLUME_HANDLER_H

#include <QObject>

class VolumeHandler : public QObject {
    Q_OBJECT
public:
    explicit VolumeHandler(QObject *parent = nullptr);
    virtual ~VolumeHandler() = default;

signals:
    void volumeChanged(int delta);

protected:
    static VolumeHandler *instance;
};

#endif // VOLUME_HANDLER_H
