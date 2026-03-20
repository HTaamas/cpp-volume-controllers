#ifndef OSD_WINDOW_H
#define OSD_WINDOW_H

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPixmap>

class ScrollingLabel;

class OSDWindow : public QWidget {
    Q_OBJECT
public:
    explicit OSDWindow(QWidget *parent = nullptr);
    void showVolume(int volume, const QString &track, const QString &artist, const QString &albumArtUrl = "", int progressMs = 0, int durationMs = 0);
    void syncProgress(int progressMs);

private slots:
    void onImageDownloaded(QNetworkReply *reply);
    void updateSongProgress();

private:
    ScrollingLabel *trackLabel;
    ScrollingLabel *artistLabel;
    QLabel *albumArtLabel;
    QLabel *timeLabel;
    QLabel *volumeLabel;
    QProgressBar *volumeBar;
    QProgressBar *songProgressBar;
    QTimer *hideTimer;
    QTimer *progressTimer;
    QNetworkAccessManager *network;
    QString lastArtUrl;
    int currentProgressMs = 0;
    int totalDurationMs = 0;
    QString formatTime(int ms);
};

#endif // OSD_WINDOW_H
