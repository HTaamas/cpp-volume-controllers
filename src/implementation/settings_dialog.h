#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QDialog>
#ifdef _WIN32
#include "audio_ducker.h"
#endif
#include "app_settings.h"

class QLabel;
class QPushButton;
class QCheckBox;
class QLineEdit;
class QSpinBox;
class QTabWidget;
class QComboBox;

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);

    void setCredentialsConfigured(bool configured);
    void setAuthenticated(bool authenticated);
    void setRedirectUri(const QString &redirectUri);
    #ifdef _WIN32
    void setAudioDuckerSettings(const AudioDuckerSettings &settings);
    AudioDuckerSettings audioDuckerSettings() const;
    void setAudioDuckerStatusText(const QString &statusText);
    void setAvailableAudioDevices(const QVector<AudioOutputDeviceOption> &devices);
    #endif
    void setOverlaySettings(const OverlaySettings &settings);
    OverlaySettings overlaySettings() const;
    void setKeybindSettings(const KeybindSettings &settings);
    KeybindSettings keybindSettings() const;

signals:
    void connectSpotifyRequested();
    #ifdef _WIN32
    void audioDuckerSettingsChanged();
    #endif
    void overlaySettingsChanged();
    void keybindSettingsChanged();

private:
    void refreshUi();
    #ifdef _WIN32
    void wireAudioDuckerControls();
    #endif
    void wireOverlayControls();
    void wireKeybindControls();
    QWidget *createColorFieldRow(QLineEdit *edit, QLabel *preview, QWidget *parent = nullptr);
    void updateColorPreview(QLineEdit *edit, QLabel *preview);

    QLabel *connectionValueLabel;
    QLabel *credentialsValueLabel;
    QLabel *redirectUriValueLabel;
    QLabel *helpTextLabel;
    #ifdef _WIN32
    QLabel *audioDuckerStatusLabel;
    #endif
    QPushButton *connectButton;
    QTabWidget *tabs;
    #ifdef _WIN32
    QCheckBox *audioDuckerEnabledCheck;
    QCheckBox *monitorEntireOutputCheck;
    QCheckBox *instantDuckCheck;
    QComboBox *monitorDeviceCombo;
    QLineEdit *monitorProcessEdit;
    QSpinBox *duckedVolumeSpin;
    QSpinBox *thresholdSpin;
    QSpinBox *cooldownSpin;
    QSpinBox *releaseHoldSpin;
    #endif
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
    QLineEdit *togglePlayEdit;

    bool credentialsConfigured = false;
    bool authenticated = false;
};

#endif // SETTINGS_DIALOG_H
