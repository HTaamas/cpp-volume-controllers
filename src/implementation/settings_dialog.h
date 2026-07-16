#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QDialog>
#include "app_settings.h"

class QLabel;
class QPushButton;
class QCheckBox;
class QLineEdit;
class QSpinBox;
class QTabWidget;

class QPlainTextEdit;

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);

    void setAuthenticated(bool authenticated);
    void setRateLimitStatusText(const QString &text);
    void setCurrentPollingIntervalText(const QString &text);
    void setTimeTillNextPollText(const QString &text);
    void appendLog(const QString &text);
    void setOverlaySettings(const OverlaySettings &settings);
    OverlaySettings overlaySettings() const;
    void setKeybindSettings(const KeybindSettings &settings);
    KeybindSettings keybindSettings() const;

    // Show the device-flow verification URL and user code while auth is pending.
    void showAuthorizationPrompt(const QString &url, const QString &code);

signals:
    void connectSpotifyRequested();
    void overlaySettingsChanged();
    void keybindSettingsChanged();

private:
    void refreshUi();
    void wireOverlayControls();
    void wireKeybindControls();
    QWidget *createColorFieldRow(QLineEdit *edit, QLabel *preview, QWidget *parent = nullptr);
    void updateColorPreview(QLineEdit *edit, QLabel *preview);

    QLabel *connectionValueLabel;
    QLabel *rateLimitValueLabel;
    QLabel *currentPollingIntervalLabel;
    QLabel *timeTillNextPollLabel;
    QLabel *helpTextLabel;
    QPushButton *connectButton;
    QTabWidget *tabs;
    QLineEdit *backgroundColorEdit;
    QLabel *backgroundColorPreview;
    QLineEdit *borderColorEdit;
    QLabel *borderColorPreview;
    QLineEdit *accentColorEdit;
    QLabel *accentColorPreview;
    QLineEdit *primaryTextColorEdit;
    QLabel *primaryTextColorPreview;
    QLineEdit *secondaryTextColorEdit;
    QLabel *secondaryTextColorPreview;
    QLineEdit *mutedTextColorEdit;
    QLabel *mutedTextColorPreview;
    QLineEdit *progressBarColorEdit;
    QLabel *progressBarColorPreview;
    QSpinBox *overlayWidthSpin;
    QSpinBox *hideDurationSpin;
    QSpinBox *coarseStepSpin;
    QSpinBox *fineStepSpin;
    QCheckBox *useShiftFineAdjustCheck;
    QLineEdit *mainKeyEdit;

    bool authenticated = false;
    QString rateLimitStatusText = "Not rate limited";
    QString currentPollingIntervalText = "N/A";
    QString timeTillNextPollText = "N/A";

    QPlainTextEdit *logViewer = nullptr;
};

#endif // SETTINGS_DIALOG_H
