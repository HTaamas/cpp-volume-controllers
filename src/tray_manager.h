#ifndef TRAY_MANAGER_H
#define TRAY_MANAGER_H

#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>

class TrayManager : public QObject {
    Q_OBJECT
public:
    explicit TrayManager(QObject *parent = nullptr);
    void updateTrackInfo(const QString &track, const QString &artist);

signals:
    void quitRequested();

private:
    QSystemTrayIcon *trayIcon;
    QAction *trackAction;
};

#endif // TRAY_MANAGER_H
