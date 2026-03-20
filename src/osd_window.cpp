#include "osd_window.h"
#include <QScreen>
#include <QGuiApplication>
#include <QFont>
#include <QGraphicsDropShadowEffect>

OSDWindow::OSDWindow(QWidget *parent) : QWidget(parent), network(new QNetworkAccessManager(this)) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool | Qt::WindowTransparentForInput | Qt::WindowDoesNotAcceptFocus);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QWidget *container = new QWidget(this);
    container->setStyleSheet("background-color: #1c1c1c; border-radius: 12px; border: 1px solid #333333;");

    QHBoxLayout *hLayout = new QHBoxLayout(container);
    hLayout->setContentsMargins(15, 15, 15, 15);
    hLayout->setSpacing(15);

    albumArtLabel = new QLabel(this);
    albumArtLabel->setFixedSize(80, 80);
    albumArtLabel->setStyleSheet("border: none; border-radius: 6px; background-color: #2c2c2c; color: #1DB954; font-size: 32px;");
    albumArtLabel->setScaledContents(true);
    albumArtLabel->setAlignment(Qt::AlignCenter);
    albumArtLabel->setText("🎵");
    hLayout->addWidget(albumArtLabel);

    QVBoxLayout *textLayout = new QVBoxLayout();
    textLayout->setSpacing(2);

    trackLabel = new QLabel(this);
    trackLabel->setStyleSheet("color: #ffffff; font-weight: bold; font-size: 16px; border: none;");

    artistLabel = new QLabel(this);
    artistLabel->setStyleSheet("color: #aaaaaa; font-size: 13px; border: none;");

    volumeBar = new QProgressBar(this);
    volumeBar->setRange(0, 100);
    volumeBar->setTextVisible(false);
    volumeBar->setFixedHeight(8);
    volumeBar->setStyleSheet(
        "QProgressBar { background-color: #333333; border: none; border-radius: 4px; }"
        "QProgressBar::chunk { background-color: #1DB954; border-radius: 4px; }"
    );

    // Song Progress Area
    QVBoxLayout *progressArea = new QVBoxLayout();
    progressArea->setSpacing(2);

    songProgressBar = new QProgressBar(this);
    songProgressBar->setTextVisible(false);
    songProgressBar->setFixedHeight(4);
    songProgressBar->setStyleSheet(
        "QProgressBar { background-color: #333333; border: none; border-radius: 2px; }"
        "QProgressBar::chunk { background-color: #ffffff; border-radius: 2px; }"
    );

    timeLabel = new QLabel(this);
    timeLabel->setStyleSheet("color: #888888; font-size: 11px; font-family: monospace; border: none;");
    timeLabel->setAlignment(Qt::AlignRight);

    progressArea->addWidget(songProgressBar);
    progressArea->addWidget(timeLabel);

    textLayout->addWidget(trackLabel);
    textLayout->addWidget(artistLabel);
    textLayout->addStretch(); // Move stretch here to push progress and volume to the bottom
    textLayout->addLayout(progressArea);
    textLayout->addSpacing(2); // Small gap between progress and volume
    textLayout->addWidget(volumeBar);

    hLayout->addLayout(textLayout);
    hLayout->setStretch(1, 1);

    mainLayout->addWidget(container);

    setFixedSize(440, 110); // Exactly album art (80) + top margin (15) + bottom margin (15)

    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();
    move(screenGeometry.width() / 2 - width() / 2, screenGeometry.height() - 180);

    hideTimer = new QTimer(this);
    hideTimer->setSingleShot(true);
    connect(hideTimer, &QTimer::timeout, this, [this]() {
        this->hide();
        if (progressTimer->isActive()) progressTimer->stop();
    });

    progressTimer = new QTimer(this);
    connect(progressTimer, &QTimer::timeout, this, &OSDWindow::updateSongProgress);
}

void OSDWindow::showVolume(int volume, const QString &track, const QString &artist, const QString &albumArtUrl, int progressMs, int durationMs) {
    trackLabel->setText(track.isEmpty() ? "Loading..." : track);
    artistLabel->setText(artist.isEmpty() ? "Spotify" : artist);
    volumeBar->setValue(volume);

    currentProgressMs = progressMs;
    totalDurationMs = durationMs;
    songProgressBar->setRange(0, durationMs);
    songProgressBar->setValue(progressMs);
    timeLabel->setText(formatTime(progressMs) + " / " + formatTime(durationMs));

    if (!albumArtUrl.isEmpty() && albumArtUrl != lastArtUrl) {
        lastArtUrl = albumArtUrl;
        QNetworkRequest request(albumArtUrl);
        QNetworkReply *reply = network->get(request);
        connect(reply, &QNetworkReply::finished, [this, reply]() {
            onImageDownloaded(reply);
            // Only show after image is ready
            this->show();
            this->raise();
            hideTimer->start(3500);
            progressTimer->start(100);
        });
    } else {
        // If no new image, show immediately
        show();
        raise();
        hideTimer->start(3500);
        progressTimer->start(100);
    }
}

void OSDWindow::syncProgress(int progressMs) {
    currentProgressMs = progressMs;
    if (isVisible()) {
        songProgressBar->setValue(currentProgressMs);
        timeLabel->setText(formatTime(currentProgressMs) + " / " + formatTime(totalDurationMs));
    }
}

void OSDWindow::updateSongProgress() {
    if (totalDurationMs > 0 && currentProgressMs < totalDurationMs) {
        currentProgressMs += 100;
        songProgressBar->setValue(currentProgressMs);
        timeLabel->setText(formatTime(currentProgressMs) + " / " + formatTime(totalDurationMs));
    }
}

QString OSDWindow::formatTime(int ms) {
    int totalSeconds = ms / 1000;
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;
    return QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
}

void OSDWindow::onImageDownloaded(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray imageData = reply->readAll();
        QPixmap pixmap;
        if (pixmap.loadFromData(imageData)) {
            albumArtLabel->setText(""); // Clear fallback
            albumArtLabel->setPixmap(pixmap.scaled(70, 70, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        }
    } else {
        albumArtLabel->setText("🎵");
        qDebug() << "Image Download Error:" << reply->errorString();
    }
    reply->deleteLater();
}
