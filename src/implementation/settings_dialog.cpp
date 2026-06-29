#include "settings_dialog.h"

#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

namespace {
QWidget *createRow(const QString &labelText, QWidget *fieldWidget, QWidget *parent = nullptr) {
    QWidget *row = new QWidget(parent);
    QHBoxLayout *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    QLabel *label = new QLabel(labelText, row);
    label->setMinimumWidth(120);
    if (QLabel *textLabel = qobject_cast<QLabel *>(fieldWidget)) {
        textLabel->setWordWrap(true);
    }

    layout->addWidget(label);
    layout->addWidget(fieldWidget, 1);
    return row;
}

QLabel *createColorPreview(QWidget *parent = nullptr) {
    QLabel *preview = new QLabel(parent);
    preview->setFixedSize(28, 20);
    preview->setFrameShape(QFrame::StyledPanel);
    preview->setFrameShadow(QFrame::Sunken);
    return preview;
}
}

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("SpotifyVol Settings");
    setModal(false);
    resize(520, 470);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(12);

    QLabel *titleLabel = new QLabel("SpotifyVol Settings", this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(titleFont.pointSize() + 2);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    layout->addWidget(titleLabel);

    tabs = new QTabWidget(this);

    QWidget *spotifyTab = new QWidget(this);
    QVBoxLayout *spotifyLayout = new QVBoxLayout(spotifyTab);
    spotifyLayout->setContentsMargins(12, 12, 12, 12);
    spotifyLayout->setSpacing(12);

    QLabel *spotifyTitle = new QLabel("Spotify Connection", spotifyTab);
    spotifyTitle->setFont(titleFont);
    spotifyLayout->addWidget(spotifyTitle);

    connectionValueLabel = new QLabel(this);
    credentialsValueLabel = new QLabel(this);
    redirectUriValueLabel = new QLabel(this);
    helpTextLabel = new QLabel(this);
    helpTextLabel->setWordWrap(true);

    spotifyLayout->addWidget(createRow("Status", connectionValueLabel, spotifyTab));
    spotifyLayout->addWidget(createRow("Credentials", credentialsValueLabel, spotifyTab));
    spotifyLayout->addWidget(createRow("Redirect URI", redirectUriValueLabel, spotifyTab));
    spotifyLayout->addWidget(helpTextLabel);

    connectButton = new QPushButton(this);
    connect(connectButton, &QPushButton::clicked, this, &SettingsDialog::connectSpotifyRequested);
    spotifyLayout->addWidget(connectButton, 0, Qt::AlignLeft);
    spotifyLayout->addStretch();
    tabs->addTab(spotifyTab, "Spotify");

    #ifdef _WIN32
    QWidget *duckingTab = new QWidget(this);
    QVBoxLayout *duckingLayout = new QVBoxLayout(duckingTab);
    duckingLayout->setContentsMargins(12, 12, 12, 12);
    duckingLayout->setSpacing(12);

    audioDuckerEnabledCheck = new QCheckBox("Enable audio ducking", this);
    duckingLayout->addWidget(audioDuckerEnabledCheck);

    monitorEntireOutputCheck = new QCheckBox("Watch the whole output device instead of one app", this);
    duckingLayout->addWidget(monitorEntireOutputCheck);

    instantDuckCheck = new QCheckBox("Drop volume instantly when ducking starts", this);
    duckingLayout->addWidget(instantDuckCheck);

    monitorDeviceCombo = new QComboBox(this);
    duckingLayout->addWidget(createRow("Output device", monitorDeviceCombo, duckingTab));

    monitorProcessEdit = new QLineEdit(this);
    monitorProcessEdit->setPlaceholderText("Discord.exe");
    duckingLayout->addWidget(createRow("Monitored app", monitorProcessEdit, duckingTab));

    duckedVolumeSpin = new QSpinBox(this);
    duckedVolumeSpin->setRange(0, 100);
    duckedVolumeSpin->setSuffix("%");
    duckingLayout->addWidget(createRow("Ducked volume", duckedVolumeSpin, duckingTab));

    thresholdSpin = new QSpinBox(this);
    thresholdSpin->setRange(1, 100);
    thresholdSpin->setSuffix("%");
    duckingLayout->addWidget(createRow("Trigger level", thresholdSpin, duckingTab));

    cooldownSpin = new QSpinBox(this);
    cooldownSpin->setRange(0, 10000);
    cooldownSpin->setSuffix(" ms");
    duckingLayout->addWidget(createRow("Cooldown", cooldownSpin, duckingTab));

    releaseHoldSpin = new QSpinBox(this);
    releaseHoldSpin->setRange(0, 10000);
    releaseHoldSpin->setSuffix(" ms");
    duckingLayout->addWidget(createRow("Silence hold", releaseHoldSpin, duckingTab));

    audioDuckerStatusLabel = new QLabel(this);
    audioDuckerStatusLabel->setWordWrap(true);
    duckingLayout->addWidget(audioDuckerStatusLabel);
    duckingLayout->addStretch();
    tabs->addTab(duckingTab, "Ducking");
    #endif

    QWidget *overlayTab = new QWidget(this);
    QFormLayout *overlayLayout = new QFormLayout(overlayTab);
    overlayLayout->setContentsMargins(12, 12, 12, 12);
    overlayLayout->setSpacing(10);

    backgroundColorEdit = new QLineEdit(this);
    backgroundColorPreview = createColorPreview(this);
    borderColorEdit = new QLineEdit(this);
    borderColorPreview = createColorPreview(this);
    accentColorEdit = new QLineEdit(this);
    accentColorPreview = createColorPreview(this);
    primaryTextColorEdit = new QLineEdit(this);
    primaryTextColorPreview = createColorPreview(this);
    secondaryTextColorEdit = new QLineEdit(this);
    secondaryTextColorPreview = createColorPreview(this);
    mutedTextColorEdit = new QLineEdit(this);
    mutedTextColorPreview = createColorPreview(this);
    progressBarColorEdit = new QLineEdit(this);
    progressBarColorPreview = createColorPreview(this);
    overlayWidthSpin = new QSpinBox(this);
    overlayWidthSpin->setRange(320, 900);
    hideDurationSpin = new QSpinBox(this);
    hideDurationSpin->setRange(1000, 15000);
    hideDurationSpin->setSuffix(" ms");

    overlayLayout->addRow("Background", createColorFieldRow(backgroundColorEdit, backgroundColorPreview, overlayTab));
    overlayLayout->addRow("Border", createColorFieldRow(borderColorEdit, borderColorPreview, overlayTab));
    overlayLayout->addRow("Accent", createColorFieldRow(accentColorEdit, accentColorPreview, overlayTab));
    overlayLayout->addRow("Primary text", createColorFieldRow(primaryTextColorEdit, primaryTextColorPreview, overlayTab));
    overlayLayout->addRow("Secondary text", createColorFieldRow(secondaryTextColorEdit, secondaryTextColorPreview, overlayTab));
    overlayLayout->addRow("Muted text", createColorFieldRow(mutedTextColorEdit, mutedTextColorPreview, overlayTab));
    overlayLayout->addRow("Progress bar", createColorFieldRow(progressBarColorEdit, progressBarColorPreview, overlayTab));
    overlayLayout->addRow("Overlay width", overlayWidthSpin);
    overlayLayout->addRow("Hide delay", hideDurationSpin);
    tabs->addTab(overlayTab, "Overlay");

    QWidget *keybindsTab = new QWidget(this);
    QFormLayout *keybindsLayout = new QFormLayout(keybindsTab);
    keybindsLayout->setContentsMargins(12, 12, 12, 12);
    keybindsLayout->setSpacing(10);

    coarseStepSpin = new QSpinBox(this);
    coarseStepSpin->setRange(1, 25);
    fineStepSpin = new QSpinBox(this);
    fineStepSpin->setRange(1, 25);
    togglePlayEdit = new QLineEdit(this);
    useShiftFineAdjustCheck = new QCheckBox("Use Shift for fine adjustment", this);
    QLabel *duckingToggleHint = new QLabel("Global ducking toggle: Alt+D", this);
    duckingToggleHint->setWordWrap(true);

    keybindsLayout->addRow("Coarse step", coarseStepSpin);
    keybindsLayout->addRow("Fine step", fineStepSpin);
    keybindsLayout->addRow("Toggle play/pause (VK)", togglePlayEdit);
    keybindsLayout->addRow(QString(), useShiftFineAdjustCheck);
    keybindsLayout->addRow(QString(), duckingToggleHint);
    tabs->addTab(keybindsTab, "Keybinds");

    layout->addWidget(tabs);

    #ifdef _WIN32
    wireAudioDuckerControls();
    #endif
    wireOverlayControls();
    wireKeybindControls();

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::hide);
    layout->addWidget(buttonBox);

    refreshUi();
}

void SettingsDialog::setCredentialsConfigured(bool configured) {
    credentialsConfigured = configured;
    refreshUi();
}

void SettingsDialog::setAuthenticated(bool isAuthenticated) {
    authenticated = isAuthenticated;
    refreshUi();
}

void SettingsDialog::setRedirectUri(const QString &redirectUri) {
    redirectUriValueLabel->setText(redirectUri);
}

#ifdef _WIN32
void SettingsDialog::setAudioDuckerSettings(const AudioDuckerSettings &settings) {
    audioDuckerEnabledCheck->setChecked(settings.enabled);
    monitorEntireOutputCheck->setChecked(settings.monitorEntireOutput);
    instantDuckCheck->setChecked(settings.instantDuck);
    const int deviceIndex = monitorDeviceCombo->findData(settings.monitorDeviceId);
    if (deviceIndex >= 0) {
        monitorDeviceCombo->setCurrentIndex(deviceIndex);
    }
    monitorProcessEdit->setText(settings.monitorProcessName);
    duckedVolumeSpin->setValue(settings.duckedVolume);
    thresholdSpin->setValue(settings.thresholdPercent);
    cooldownSpin->setValue(settings.cooldownMs);
    releaseHoldSpin->setValue(settings.releaseHoldMs);
    monitorProcessEdit->setEnabled(!settings.monitorEntireOutput);
}

AudioDuckerSettings SettingsDialog::audioDuckerSettings() const {
    AudioDuckerSettings settings;
    settings.enabled = audioDuckerEnabledCheck->isChecked();
    settings.monitorEntireOutput = monitorEntireOutputCheck->isChecked();
    settings.instantDuck = instantDuckCheck->isChecked();
    settings.monitorDeviceId = monitorDeviceCombo->currentData().toString();
    settings.monitorDeviceName = monitorDeviceCombo->currentText();
    settings.monitorProcessName = monitorProcessEdit->text().trimmed();
    settings.duckedVolume = duckedVolumeSpin->value();
    settings.thresholdPercent = thresholdSpin->value();
    settings.cooldownMs = cooldownSpin->value();
    settings.releaseHoldMs = releaseHoldSpin->value();
    return settings;
}

void SettingsDialog::setAudioDuckerStatusText(const QString &statusText) {
    audioDuckerStatusLabel->setText(statusText);
}

void SettingsDialog::setAvailableAudioDevices(const QVector<AudioOutputDeviceOption> &devices) {
    const QString selectedId = monitorDeviceCombo->currentData().toString();
    monitorDeviceCombo->clear();
    monitorDeviceCombo->addItem("Default system output", "");
    for (const AudioOutputDeviceOption &device : devices) {
        monitorDeviceCombo->addItem(device.name, device.id);
    }

    const int selectedIndex = monitorDeviceCombo->findData(selectedId);
    if (selectedIndex >= 0) {
        monitorDeviceCombo->setCurrentIndex(selectedIndex);
    }
}
#endif

void SettingsDialog::setOverlaySettings(const OverlaySettings &settings) {
    backgroundColorEdit->setText(settings.backgroundColor);
    updateColorPreview(backgroundColorEdit, backgroundColorPreview);
    borderColorEdit->setText(settings.borderColor);
    updateColorPreview(borderColorEdit, borderColorPreview);
    accentColorEdit->setText(settings.accentColor);
    updateColorPreview(accentColorEdit, accentColorPreview);
    primaryTextColorEdit->setText(settings.primaryTextColor);
    updateColorPreview(primaryTextColorEdit, primaryTextColorPreview);
    secondaryTextColorEdit->setText(settings.secondaryTextColor);
    updateColorPreview(secondaryTextColorEdit, secondaryTextColorPreview);
    mutedTextColorEdit->setText(settings.mutedTextColor);
    updateColorPreview(mutedTextColorEdit, mutedTextColorPreview);
    progressBarColorEdit->setText(settings.progressBarColor);
    updateColorPreview(progressBarColorEdit, progressBarColorPreview);
    overlayWidthSpin->setValue(settings.overlayWidth);
    hideDurationSpin->setValue(settings.hideDurationMs);
}

OverlaySettings SettingsDialog::overlaySettings() const {
    OverlaySettings settings;
    settings.backgroundColor = backgroundColorEdit->text().trimmed();
    settings.borderColor = borderColorEdit->text().trimmed();
    settings.accentColor = accentColorEdit->text().trimmed();
    settings.primaryTextColor = primaryTextColorEdit->text().trimmed();
    settings.secondaryTextColor = secondaryTextColorEdit->text().trimmed();
    settings.mutedTextColor = mutedTextColorEdit->text().trimmed();
    settings.progressBarColor = progressBarColorEdit->text().trimmed();
    settings.overlayWidth = overlayWidthSpin->value();
    settings.hideDurationMs = hideDurationSpin->value();
    return settings;
}

void SettingsDialog::setKeybindSettings(const KeybindSettings &settings) {
    coarseStepSpin->setValue(settings.coarseStep);
    fineStepSpin->setValue(settings.fineStep);
    togglePlayEdit->setText(settings.togglePlay);
    useShiftFineAdjustCheck->setChecked(settings.useShiftForFineAdjust);
}

KeybindSettings SettingsDialog::keybindSettings() const {
    KeybindSettings settings;
    settings.coarseStep = coarseStepSpin->value();
    settings.fineStep = fineStepSpin->value();
    settings.togglePlay = togglePlayEdit->text().trimmed();
    settings.useShiftForFineAdjust = useShiftFineAdjustCheck->isChecked();
    return settings;
}

#ifdef _WIN32
void SettingsDialog::wireAudioDuckerControls() {
    connect(audioDuckerEnabledCheck, &QCheckBox::toggled, this, &SettingsDialog::audioDuckerSettingsChanged);
    connect(monitorEntireOutputCheck, &QCheckBox::toggled, this, [this](bool checked) {
        monitorProcessEdit->setEnabled(!checked);
        emit audioDuckerSettingsChanged();
    });
    connect(instantDuckCheck, &QCheckBox::toggled, this, &SettingsDialog::audioDuckerSettingsChanged);
    connect(monitorDeviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { emit audioDuckerSettingsChanged(); });
    connect(monitorProcessEdit, &QLineEdit::textChanged, this, &SettingsDialog::audioDuckerSettingsChanged);
    connect(duckedVolumeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) { emit audioDuckerSettingsChanged(); });
    connect(thresholdSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) { emit audioDuckerSettingsChanged(); });
    connect(cooldownSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) { emit audioDuckerSettingsChanged(); });
    connect(releaseHoldSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) { emit audioDuckerSettingsChanged(); });
}
#endif

void SettingsDialog::wireOverlayControls() {
    connect(backgroundColorEdit, &QLineEdit::textChanged, this, [this](const QString &) { updateColorPreview(backgroundColorEdit, backgroundColorPreview); emit overlaySettingsChanged(); });
    connect(borderColorEdit, &QLineEdit::textChanged, this, [this](const QString &) { updateColorPreview(borderColorEdit, borderColorPreview); emit overlaySettingsChanged(); });
    connect(accentColorEdit, &QLineEdit::textChanged, this, [this](const QString &) { updateColorPreview(accentColorEdit, accentColorPreview); emit overlaySettingsChanged(); });
    connect(primaryTextColorEdit, &QLineEdit::textChanged, this, [this](const QString &) { updateColorPreview(primaryTextColorEdit, primaryTextColorPreview); emit overlaySettingsChanged(); });
    connect(secondaryTextColorEdit, &QLineEdit::textChanged, this, [this](const QString &) { updateColorPreview(secondaryTextColorEdit, secondaryTextColorPreview); emit overlaySettingsChanged(); });
    connect(mutedTextColorEdit, &QLineEdit::textChanged, this, [this](const QString &) { updateColorPreview(mutedTextColorEdit, mutedTextColorPreview); emit overlaySettingsChanged(); });
    connect(progressBarColorEdit, &QLineEdit::textChanged, this, [this](const QString &) { updateColorPreview(progressBarColorEdit, progressBarColorPreview); emit overlaySettingsChanged(); });
    connect(overlayWidthSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) { emit overlaySettingsChanged(); });
    connect(hideDurationSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) { emit overlaySettingsChanged(); });
}

void SettingsDialog::wireKeybindControls() {
    connect(coarseStepSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) { emit keybindSettingsChanged(); });
    connect(fineStepSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) { emit keybindSettingsChanged(); });
    connect(togglePlayEdit, &QLineEdit::textChanged, this, &SettingsDialog::keybindSettingsChanged);
    connect(useShiftFineAdjustCheck, &QCheckBox::toggled, this, &SettingsDialog::keybindSettingsChanged);
}

QWidget *SettingsDialog::createColorFieldRow(QLineEdit *edit, QLabel *preview, QWidget *parent) {
    QWidget *row = new QWidget(parent);
    QHBoxLayout *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    layout->addWidget(edit, 1);
    layout->addWidget(preview, 0);
    return row;
}

void SettingsDialog::updateColorPreview(QLineEdit *edit, QLabel *preview) {
    const QColor color(edit->text().trimmed());
    if (color.isValid()) {
        preview->setStyleSheet(QString("background-color: %1; border: 1px solid #555555; border-radius: 4px;").arg(color.name()));
    } else {
        preview->setStyleSheet("background-color: #202020; border: 1px solid #aa5555; border-radius: 4px;");
    }
}

void SettingsDialog::refreshUi() {
    credentialsValueLabel->setText(credentialsConfigured ? "Configured" : "Missing");
    connectionValueLabel->setText(authenticated ? "Connected" : "Not connected");

    if (!credentialsConfigured) {
        connectButton->setEnabled(false);
        connectButton->setText("Spotify credentials required");
        helpTextLabel->setText("Spotify client credentials are missing from the build configuration, so authorization cannot start yet.");
        return;
    }

    connectButton->setEnabled(true);
    connectButton->setText(authenticated ? "Reconnect Spotify" : "Connect Spotify");
    helpTextLabel->setText(authenticated
        ? "Your Spotify session is available. You can reconnect if you want to refresh authorization manually."
        : "Open the Spotify authorization flow in your browser to link this app to your account.");
}
