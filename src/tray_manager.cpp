#include "tray_manager.h"
#include <QIcon>
#include <QPixmap>
#include <QPainter>
#include <QApplication>

TrayManager::TrayManager(QObject *parent) : QObject(parent) {
    QPixmap pixmap(64, 64);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setBrush(QColor("#1DB954"));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(10, 10, 44, 44);
    
    trayIcon = new QSystemTrayIcon(QIcon(pixmap), this);
    QMenu *menu = new QMenu();
    
    trackAction = menu->addAction("Current Playing: None");
    trackAction->setEnabled(false);
    
    QAction *quitAction = menu->addAction("Quit");
    connect(quitAction, &QAction::triggered, QApplication::quit);
    
    trayIcon->setContextMenu(menu);
    trayIcon->show();
}

void TrayManager::updateTrackInfo(const QString &track, const QString &artist) {
    trackAction->setText("Current Playing: " + track + " - " + artist);
}
