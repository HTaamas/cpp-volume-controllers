#include "osd_window.h"
#include <QScreen>
#include <QGuiApplication>
#include <QFont>
#include <QGraphicsDropShadowEffect>
#include <QFontMetrics>
#include <QEasingCurve>
#include <QPainter>

class ScrollingLabel : public QWidget {
public:
    explicit ScrollingLabel(QWidget *parent = nullptr)
        : QWidget(parent), label(new QLabel(this)), timer(new QTimer(this)) {
        label->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        label->move(0, 0);

        timer->setInterval(30);
        connect(timer, &QTimer::timeout, this, [this]() { tick(); });
    }

    void setText(const QString &text) {
        fullText = text;
        restart();
    }

    void setLabelStyleSheet(const QString &style) {
        label->setStyleSheet(style);
    }

protected:
    void resizeEvent(QResizeEvent *event) override {
        QWidget::resizeEvent(event);
        restart();
    }

private:
    void restart() {
        offsetPx = 0;
        pauseTicks = 20;

        const QFontMetrics fm(label->font());
        const int textWidth = fm.horizontalAdvance(fullText);
        const int gapWidth = fm.horizontalAdvance(QStringLiteral("     "));

        if (textWidth <= width()) {
            timer->stop();
            label->setText(fullText);
            label->setFixedSize(width(), height());
            label->move(0, 0);
            loopWidth = 0;
            return;
        }

        const QString repeated = fullText + QStringLiteral("     ") + fullText;
        label->setText(repeated);
        const int repeatedWidth = fm.horizontalAdvance(repeated);
        label->setFixedSize(repeatedWidth + 8, height());
        label->move(0, 0);

        loopWidth = textWidth + gapWidth;
        timer->start();
    }

    void tick() {
        if (loopWidth <= 0) {
            return;
        }

        if (pauseTicks > 0) {
            --pauseTicks;
            return;
        }

        offsetPx += 1;
        if (offsetPx >= loopWidth) {
            offsetPx = 0;
            pauseTicks = 20;
        }

        label->move(-offsetPx, 0);
    }

    QLabel *label;
    QTimer *timer;
    QString fullText;
    int offsetPx = 0;
    int loopWidth = 0;
    int pauseTicks = 0;
};

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

    pauseOverlay = new QLabel(albumArtLabel);
    pauseOverlay->setFixedSize(albumArtLabel->size());
    pauseOverlay->setAlignment(Qt::AlignCenter);
    QPixmap pauseIcon(20, 20);
    pauseIcon.fill(Qt::transparent);
    {
        QPainter painter(&pauseIcon);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 255, 255, 235));
        painter.drawRoundedRect(QRectF(4, 3, 4, 14), 1.5, 1.5);
        painter.drawRoundedRect(QRectF(12, 3, 4, 14), 1.5, 1.5);
    }
    pauseOverlay->setPixmap(pauseIcon);
    pauseOverlay->setStyleSheet("background-color: rgba(0, 0, 0, 110); border-radius: 6px; border: none;");
    pauseOverlay->move(0, 0);

    pauseOverlayEffect = new QGraphicsOpacityEffect(pauseOverlay);
    pauseOverlayEffect->setOpacity(0.0);
    pauseOverlay->setGraphicsEffect(pauseOverlayEffect);

    pauseOverlayFade = new QPropertyAnimation(pauseOverlayEffect, "opacity", this);
    pauseOverlayFade->setDuration(200);
    pauseOverlayFade->setEasingCurve(QEasingCurve::InOutQuad);

    pauseOverlay->show();
    hLayout->addWidget(albumArtLabel);

    QVBoxLayout *textLayout = new QVBoxLayout();
    textLayout->setSpacing(2);

    trackLabel = new ScrollingLabel(this);
    trackLabel->setLabelStyleSheet("color: #ffffff; font-weight: bold; font-size: 16px; border: none;");
    trackLabel->setFixedHeight(24);

    artistLabel = new ScrollingLabel(this);
    artistLabel->setLabelStyleSheet("color: #aaaaaa; font-size: 13px; border: none;");
    artistLabel->setFixedHeight(20);

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

void OSDWindow::showVolume(int volume, const QString &track, const QString &artist, const QString &albumArtUrl, int progressMs, int durationMs, bool isPlaying) {
    trackLabel->setText(track.isEmpty() ? "Loading..." : track);
    artistLabel->setText(artist.isEmpty() ? "Spotify" : artist);
    volumeBar->setValue(volume);

    currentProgressMs = progressMs;
    totalDurationMs = durationMs;
    isPlayingNow = isPlaying;
    setPausedOverlayVisible(!isPlayingNow);
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

void OSDWindow::syncProgress(int progressMs, bool isPlaying) {
    currentProgressMs = progressMs;
    isPlayingNow = isPlaying;
    setPausedOverlayVisible(!isPlayingNow);
    if (isVisible()) {
        songProgressBar->setValue(currentProgressMs);
        timeLabel->setText(formatTime(currentProgressMs) + " / " + formatTime(totalDurationMs));
    }
}

void OSDWindow::setPausedOverlayVisible(bool visible) {
    pauseOverlayFade->stop();
    pauseOverlayFade->setStartValue(pauseOverlayEffect->opacity());
    pauseOverlayFade->setEndValue(visible ? 1.0 : 0.0);
    pauseOverlayFade->start();
}

void OSDWindow::updateSongProgress() {
    if (isPlayingNow && totalDurationMs > 0 && currentProgressMs < totalDurationMs) {
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
