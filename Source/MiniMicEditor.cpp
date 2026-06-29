// SPDX-License-Identifier: GPL-3.0-or-later

#include "MiniMicEditor.h"

#include <cmath>

#if JUCE_WINDOWS
#include <mmdeviceapi.h>
#include <propidl.h>
#endif

namespace
{
constexpr int serverPort = 10998;
constexpr auto serverHost = "aoo.sonobus.net";

juce::Colour backgroundColour() { return juce::Colour::fromRGB (15, 20, 29); }
juce::Colour panelColour()      { return juce::Colour::fromRGB (25, 32, 44); }
juce::Colour accentColour()     { return juce::Colour::fromRGB (72, 190, 160); }
juce::Colour warningColour()    { return juce::Colour::fromRGB (244, 184, 76); }
juce::Colour errorColour()      { return juce::Colour::fromRGB (240, 96, 105); }

juce::String getDefaultWindowsPlaybackDeviceName()
{
#if JUCE_WINDOWS
    static constexpr PROPERTYKEY deviceFriendlyNameKey {
        { 0xa45c254e, 0xdf1c, 0x4efd, { 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0 } },
        14
    };

    juce::String deviceName;
    const auto comInitResult = CoInitializeEx (nullptr, COINIT_APARTMENTTHREADED);
    const auto shouldUninitialiseCom = SUCCEEDED (comInitResult);

    IMMDeviceEnumerator* enumerator = nullptr;
    if (SUCCEEDED (CoCreateInstance (__uuidof (MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                     __uuidof (IMMDeviceEnumerator),
                                     reinterpret_cast<void**> (&enumerator)))
        && enumerator != nullptr)
    {
        IMMDevice* defaultDevice = nullptr;
        if (SUCCEEDED (enumerator->GetDefaultAudioEndpoint (eRender, eConsole, &defaultDevice))
            && defaultDevice != nullptr)
        {
            IPropertyStore* propertyStore = nullptr;
            if (SUCCEEDED (defaultDevice->OpenPropertyStore (STGM_READ, &propertyStore))
                && propertyStore != nullptr)
            {
                PROPVARIANT value;
                PropVariantInit (&value);

                if (SUCCEEDED (propertyStore->GetValue (deviceFriendlyNameKey, &value))
                    && value.vt == VT_LPWSTR
                    && value.pwszVal != nullptr)
                    deviceName = juce::String (value.pwszVal);

                PropVariantClear (&value);
                propertyStore->Release();
            }

            defaultDevice->Release();
        }

        enumerator->Release();
    }

    if (shouldUninitialiseCom)
        CoUninitialize();

    return deviceName;
#else
    return {};
#endif
}
}

MiniMicEditor::MiniMicEditor (SonobusAudioProcessor& processorToUse)
    : AudioProcessorEditor (&processorToUse), processor (processorToUse)
{
    processor.addClientListener (this);
    peerName = makePeerName();

    titleLabel.setText ("MiniMic Link", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (28.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    titleLabel.setJustificationType (juce::Justification::centred);

    subtitleLabel.setText ("Remote microphone untuk cloud gaming", juce::dontSendNotification);
    subtitleLabel.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
    subtitleLabel.setJustificationType (juce::Justification::centred);

    senderButton.setRadioGroupId (4101);
    receiverButton.setRadioGroupId (4101);
    senderButton.setToggleState (true, juce::dontSendNotification);
    senderButton.onClick = [this] { setRole (Role::sender); };
    receiverButton.onClick = [this] { setRole (Role::receiver); };
#if JUCE_IOS || JUCE_ANDROID
    receiverButton.setButtonText ("Receiver (Windows)");
    receiverButton.setEnabled (false);
#endif

    groupLabel.setText ("Group ID", juce::dontSendNotification);
    passwordLabel.setText ("Password", juce::dontSendNotification);
    deviceLabel.setText ("Microphone sender", juce::dontSendNotification);
    for (auto* label : { &groupLabel, &passwordLabel, &deviceLabel })
        label->setColour (juce::Label::textColourId, juce::Colours::white);

    groupEditor.setInputRestrictions (32, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-");
    groupEditor.setTextToShowWhenEmpty ("Contoh: GAME-7K2P", juce::Colours::grey);
    passwordEditor.setInputRestrictions (64);
    passwordEditor.setPasswordCharacter (0x2022);
    passwordEditor.setTextToShowWhenEmpty ("Wajib diisi", juce::Colours::grey);

    generateGroupButton.onClick = [this] { generateGroupId(); };
    generatePasswordButton.onClick = [this] { generatePassword(); };
    refreshDevicesButton.onClick = [this] { refreshAudioDevices(); };
    deviceCombo.onChange = [this] { applySelectedAudioDevice(); };
    connectButton.onClick = [this] { connectOrDisconnect(); };
    privacyMuteButton.onClick = [this]
    {
        applyRolePolicy();
        updateConnectionControls();
        updateAudioMeter();
    };

    connectButton.setColour (juce::TextButton::buttonColourId, accentColour());
    connectButton.setColour (juce::TextButton::textColourOffId, backgroundColour());

    deviceHintLabel.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
    deviceHintLabel.setFont (juce::Font (13.0f));
    deviceHintLabel.setMinimumHorizontalScale (0.7f);

    statusLabel.setJustificationType (juce::Justification::centred);
    statusLabel.setFont (juce::Font (16.0f, juce::Font::bold));
    meterLabel.setText ("Level mic dikirim", juce::dontSendNotification);
    meterLabel.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
    meterLabel.setFont (juce::Font (13.0f));
    meterLabel.setJustificationType (juce::Justification::centredLeft);
    serverLabel.setText ("Server: aoo.sonobus.net:10998", juce::dontSendNotification);
    serverLabel.setJustificationType (juce::Justification::centred);
    serverLabel.setColour (juce::Label::textColourId, juce::Colours::grey);
    securityLabel.setText ("Prototype SonoBus: audio P2P belum terenkripsi end-to-end", juce::dontSendNotification);
    securityLabel.setJustificationType (juce::Justification::centred);
    securityLabel.setColour (juce::Label::textColourId, warningColour());
    securityLabel.setFont (juce::Font (12.0f));

    for (auto* component : std::initializer_list<juce::Component*> {
             &titleLabel, &subtitleLabel, &senderButton, &receiverButton,
             &groupLabel, &groupEditor, &generateGroupButton,
             &passwordLabel, &passwordEditor, &generatePasswordButton,
             &deviceLabel, &deviceCombo, &refreshDevicesButton, &deviceHintLabel,
             &connectButton, &privacyMuteButton, &meterLabel,
             &statusLabel, &serverLabel, &securityLabel })
        addAndMakeVisible (component);

    generateGroupId();
    generatePassword();

    const auto envGroup = juce::SystemStats::getEnvironmentVariable ("MINIMIC_GROUP_ID", {}).trim();
    if (envGroup.isNotEmpty())
        groupEditor.setText (envGroup, juce::dontSendNotification);

    const auto envPassword = juce::SystemStats::getEnvironmentVariable ("MINIMIC_PASSWORD", {});
    if (envPassword.isNotEmpty())
        passwordEditor.setText (envPassword, juce::dontSendNotification);

    applyRolePolicy();
    updateRoleControls();
    setConnectionState (ConnectionState::disconnected);
    setSize (560, 660);
    startTimerHz (10);
}

MiniMicEditor::~MiniMicEditor()
{
    stopTimer();
    processor.removeClientListener (this);
}

void MiniMicEditor::paint (juce::Graphics& g)
{
    g.fillAll (backgroundColour());
    g.setColour (panelColour());
    g.fillRoundedRectangle (18.0f, 90.0f, static_cast<float> (getWidth() - 36),
                            static_cast<float> (getHeight() - 174), 12.0f);

    if (! audioMeterBounds.isEmpty())
    {
        auto bounds = audioMeterBounds.toFloat().reduced (1.0f);
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillRoundedRectangle (bounds, 7.0f);

        const auto barArea = bounds.reduced (5.0f, 5.0f);
        const auto fillArea = barArea.withWidth (barArea.getWidth() * audioMeterLevel);
        auto colour = audioMeterMuted ? juce::Colours::grey : accentColour();
        if (! audioMeterMuted && audioMeterLevel > 0.78f)
            colour = warningColour();
        if (! audioMeterMuted && audioMeterLevel > 0.94f)
            colour = errorColour();

        if (fillArea.getWidth() > 1.0f)
        {
            juce::ColourGradient gradient (accentColour(), fillArea.getX(), fillArea.getY(),
                                           warningColour(), fillArea.getRight(), fillArea.getY(), false);
            gradient.addColour (0.85, warningColour());
            gradient.addColour (1.0, errorColour());
            g.setGradientFill (gradient);
            g.fillRoundedRectangle (fillArea, 5.0f);
        }

        g.setColour (juce::Colours::white.withAlpha (0.10f));
        for (int i = 1; i < 10; ++i)
        {
            const auto x = barArea.getX() + (barArea.getWidth() * static_cast<float> (i) / 10.0f);
            g.drawVerticalLine (juce::roundToInt (x), barArea.getY(), barArea.getBottom());
        }

        g.setColour ((audioMeterMuted ? juce::Colours::lightgrey : colour).withAlpha (0.9f));
        g.drawRoundedRectangle (bounds, 7.0f, 1.3f);
    }
}

void MiniMicEditor::resized()
{
    auto area = getLocalBounds().reduced (28);
    titleLabel.setBounds (area.removeFromTop (36));
    subtitleLabel.setBounds (area.removeFromTop (25));
    area.removeFromTop (20);

    auto roleRow = area.removeFromTop (36);
    const auto roleWidth = roleRow.getWidth() / 2;
    senderButton.setBounds (roleRow.removeFromLeft (roleWidth).reduced (28, 0));
    receiverButton.setBounds (roleRow.reduced (28, 0));
    area.removeFromTop (16);

    auto layoutField = [&area] (juce::Label& label, juce::Component& field, juce::Component& action)
    {
        label.setBounds (area.removeFromTop (22));
        auto row = area.removeFromTop (38);
        action.setBounds (row.removeFromRight (142));
        row.removeFromRight (10);
        field.setBounds (row);
        area.removeFromTop (12);
    };

    layoutField (groupLabel, groupEditor, generateGroupButton);
    layoutField (passwordLabel, passwordEditor, generatePasswordButton);
    layoutField (deviceLabel, deviceCombo, refreshDevicesButton);
    deviceHintLabel.setBounds (area.removeFromTop (34));
    area.removeFromTop (8);

    auto actionRow = area.removeFromTop (46);
    connectButton.setBounds (actionRow.removeFromLeft ((actionRow.getWidth() * 2) / 3).reduced (0, 2));
    actionRow.removeFromLeft (10);
    privacyMuteButton.setBounds (actionRow);
    area.removeFromTop (12);
    meterLabel.setBounds (area.removeFromTop (20));
    audioMeterBounds = area.removeFromTop (26);
    area.removeFromTop (10);
    statusLabel.setBounds (area.removeFromTop (34));

    auto footer = getLocalBounds().removeFromBottom (66).reduced (24, 4);
    serverLabel.setBounds (footer.removeFromTop (24));
    securityLabel.setBounds (footer.removeFromTop (22));
}

void MiniMicEditor::audioDeviceManagerReady()
{
    refreshAudioDevices();
}

void MiniMicEditor::timerCallback()
{
    updateStatusForPeerCount();
    updateAudioMeter();
}

void MiniMicEditor::setRole (Role newRole)
{
    if (role == newRole)
        return;

    if (connectionState != ConnectionState::disconnected && connectionState != ConnectionState::error)
        disconnect();

    role = newRole;
    privacyMuteButton.setToggleState (false, juce::dontSendNotification);
    applyRolePolicy();
    updateRoleControls();
    refreshAudioDevices();
    setConnectionState (ConnectionState::disconnected);
}

void MiniMicEditor::applyRolePolicy()
{
    const auto privacyMuted = privacyMuteButton.getToggleState();
    const auto sendMuted = role == Role::receiver || privacyMuted;
    const auto receiveMuted = role == Role::sender || privacyMuted;

    auto& state = processor.getValueTreeState();
    if (auto* parameter = state.getParameter (SonobusAudioProcessor::paramMainSendMute))
        parameter->setValueNotifyingHost (sendMuted ? 1.0f : 0.0f);
    if (auto* parameter = state.getParameter (SonobusAudioProcessor::paramMainRecvMute))
        parameter->setValueNotifyingHost (receiveMuted ? 1.0f : 0.0f);
    if (auto* parameter = state.getParameter (SonobusAudioProcessor::paramDry))
        parameter->setValueNotifyingHost (0.0f);
}

void MiniMicEditor::refreshAudioDevices()
{
    if (! getAudioDeviceManager)
        return;

    auto* manager = getAudioDeviceManager();
    if (manager == nullptr)
        return;

    auto* currentType = static_cast<juce::AudioIODeviceType*> (nullptr);
    for (auto* type : manager->getAvailableDeviceTypes())
    {
        if (type->getTypeName() == manager->getCurrentAudioDeviceType())
        {
            currentType = type;
            break;
        }
    }

    if (currentType == nullptr)
    {
        setConnectionState (ConnectionState::error, "Driver audio Windows tidak ditemukan");
        return;
    }

    currentType->scanForDevices();
    inputDeviceNames = currentType->getDeviceNames (true);
    outputDeviceNames = currentType->getDeviceNames (false);
    const auto setup = manager->getAudioDeviceSetup();
    const auto& names = role == Role::sender ? inputDeviceNames : outputDeviceNames;
    const auto selectedName = role == Role::sender ? setup.inputDeviceName : setup.outputDeviceName;

    updatingDevices = true;
    deviceCombo.clear (juce::dontSendNotification);
    for (int index = 0; index < names.size(); ++index)
        deviceCombo.addItem (names[index], index + 1);

    auto selectedIndex = names.indexOf (selectedName);
    if (role == Role::receiver)
    {
        for (int index = 0; index < names.size(); ++index)
        {
            if (names[index].containsIgnoreCase ("CABLE Input"))
            {
                selectedIndex = index;
                break;
            }
        }
    }

    if (selectedIndex >= 0)
        deviceCombo.setSelectedItemIndex (selectedIndex, juce::dontSendNotification);
    else if (! names.isEmpty())
        deviceCombo.setSelectedItemIndex (0, juce::dontSendNotification);
    updatingDevices = false;

    if (role == Role::receiver && receiverUsesCableInput() && selectedName != deviceCombo.getText())
        applySelectedAudioDevice();

    updateRoleControls();
}

void MiniMicEditor::applySelectedAudioDevice()
{
    if (updatingDevices || ! getAudioDeviceManager || deviceCombo.getSelectedItemIndex() < 0)
        return;

    auto* manager = getAudioDeviceManager();
    if (manager == nullptr)
        return;

    auto setup = manager->getAudioDeviceSetup();
    if (role == Role::sender)
        setup.inputDeviceName = deviceCombo.getText();
    else
        setup.outputDeviceName = deviceCombo.getText();

    if (setup.sampleRate <= 0.0)
        setup.sampleRate = 48000.0;

    const auto error = manager->setAudioDeviceSetup (setup, true);
    if (error.isNotEmpty())
        setConnectionState (ConnectionState::error, "Audio device: " + error);
    else if (connectionState == ConnectionState::error || connectionState == ConnectionState::disconnected)
        setConnectionState (ConnectionState::disconnected);

    updateRoleControls();
}

bool MiniMicEditor::validateConnectionInputs (juce::String& errorMessage) const
{
    if (groupEditor.getText().trim().isEmpty())
        errorMessage = "Group ID wajib diisi";
    else if (passwordEditor.getText().isEmpty())
        errorMessage = "Password wajib diisi";
    else if (deviceCombo.getSelectedItemIndex() < 0)
        errorMessage = role == Role::sender ? "Pilih microphone sender" : "CABLE Input tidak ditemukan";
    else if (role == Role::receiver && ! receiverUsesCableInput())
        errorMessage = "Receiver wajib memakai CABLE Input agar suara tidak bocor ke speaker";
    else if (role == Role::receiver)
    {
        juce::String playbackDeviceName;
        if (receiverDefaultPlaybackUsesCableInput (playbackDeviceName))
            errorMessage = "Default speaker Windows masih VB-CABLE. Pilih speaker/headphone asli dulu.";
    }

    return errorMessage.isEmpty();
}

bool MiniMicEditor::receiverUsesCableInput() const
{
    return deviceCombo.getText().containsIgnoreCase ("CABLE Input");
}

bool MiniMicEditor::receiverDefaultPlaybackUsesCableInput (juce::String& playbackDeviceName) const
{
    playbackDeviceName = getDefaultWindowsPlaybackDeviceName();
    return playbackDeviceName.containsIgnoreCase ("CABLE Input")
        || playbackDeviceName.containsIgnoreCase ("VB-Audio Virtual Cable");
}

void MiniMicEditor::connectOrDisconnect()
{
    if (connectionState == ConnectionState::disconnected || connectionState == ConnectionState::error)
        beginConnect();
    else
        disconnect();
}

void MiniMicEditor::beginConnect()
{
    juce::String error;
    if (! validateConnectionInputs (error))
    {
        setConnectionState (ConnectionState::error, error);
        return;
    }

    pendingGroup = groupEditor.getText().trim().toUpperCase();
    pendingPassword = passwordEditor.getText();
    peerName = makePeerName();
    processor.setAutoconnectToGroupPeers (true);
    applyRolePolicy();
    connectedPeerName.clear();

    setConnectionState (ConnectionState::connectingServer, "Menghubungkan ke server SonoBus...");
    if (processor.isConnectedToServer())
    {
        setConnectionState (ConnectionState::joiningGroup, "Masuk ke grup...");
        if (! processor.joinServerGroup (pendingGroup, pendingPassword, false))
            setConnectionState (ConnectionState::error, "Gagal mengirim permintaan grup");
    }
    else if (! processor.connectToServer (serverHost, serverPort, peerName, {}))
    {
        setConnectionState (ConnectionState::error, "Server SonoBus tidak dapat dihubungi");
    }
}

void MiniMicEditor::disconnect()
{
    processor.disconnectFromServer();
    lastPeerCount = -1;
    connectedPeerName.clear();
    setConnectionState (ConnectionState::disconnected);
}

void MiniMicEditor::setConnectionState (ConnectionState newState, const juce::String& detail)
{
    connectionState = newState;
    juce::String text = detail;
    auto colour = juce::Colours::lightgrey;

    if (text.isEmpty())
    {
        switch (newState)
        {
            case ConnectionState::disconnected:     text = "Belum terhubung"; break;
            case ConnectionState::connectingServer: text = "Menghubungkan ke server..."; break;
            case ConnectionState::joiningGroup:     text = "Masuk ke grup..."; break;
            case ConnectionState::waitingForPeer:   text = "Terhubung — menunggu pasangan"; break;
            case ConnectionState::connected:        text = "Terhubung"; break;
            case ConnectionState::error:            text = "Terjadi kesalahan"; break;
        }
    }

    if (newState == ConnectionState::connected || newState == ConnectionState::waitingForPeer)
        colour = accentColour();
    else if (newState == ConnectionState::error)
        colour = errorColour();
    else if (newState == ConnectionState::connectingServer || newState == ConnectionState::joiningGroup)
        colour = warningColour();

    statusLabel.setText (text, juce::dontSendNotification);
    statusLabel.setColour (juce::Label::textColourId, colour);
    updateConnectionControls();
}

void MiniMicEditor::generateGroupId()
{
    groupEditor.setText ("GAME-" + makeRandomCode (6, false), juce::dontSendNotification);
}

void MiniMicEditor::generatePassword()
{
    passwordEditor.setText (makeRandomCode (12, true), juce::dontSendNotification);
}

juce::String MiniMicEditor::makeRandomCode (int length, bool withSeparator) const
{
    static constexpr auto alphabet = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
    constexpr int alphabetSize = 32;
    auto& random = juce::Random::getSystemRandom();
    juce::String result;

    for (int index = 0; index < length; ++index)
    {
        if (withSeparator && index > 0 && index % 4 == 0)
            result << '-';
        result << alphabet[random.nextInt (alphabetSize)];
    }

    return result;
}

juce::String MiniMicEditor::makePeerName() const
{
    const auto roleName = role == Role::sender ? "sender" : "receiver";
#if JUCE_ANDROID || JUCE_IOS
    const auto rawDeviceName = juce::SystemStats::getDeviceDescription();
#else
    const auto rawDeviceName = juce::SystemStats::getComputerName().isNotEmpty()
                                   ? juce::SystemStats::getComputerName()
                                   : juce::SystemStats::getDeviceDescription();
#endif
    const auto deviceName = makeSafePeerNamePart (rawDeviceName);
    return juce::String (roleName) + "-" + deviceName;
}

juce::String MiniMicEditor::makeSafePeerNamePart (juce::String text)
{
    text = text.trim();
    if (text.isEmpty())
        text = juce::SystemStats::getDeviceDescription().trim();
    if (text.isEmpty())
        text = "device";

    juce::String safe;
    for (int index = 0; index < text.length(); ++index)
    {
        const auto c = text[index];
        const auto isDigit = c >= '0' && c <= '9';
        const auto isLower = c >= 'a' && c <= 'z';
        const auto isUpper = c >= 'A' && c <= 'Z';
        if (isDigit || isLower || isUpper)
            safe << c;
        else if (c == '-' || c == '_')
            safe << c;
    }

    safe = safe.trimCharactersAtStart ("-_").trimCharactersAtEnd ("-_");
    if (safe.isEmpty())
        safe = "device";

    return safe.substring (0, 18).toLowerCase();
}

void MiniMicEditor::updateRoleControls()
{
    const auto isSender = role == Role::sender;
    juce::String receiverHint;
    auto receiverHintIsError = false;

    if (! isSender)
    {
        if (! receiverUsesCableInput())
        {
            receiverHint = "Install/pilih VB-CABLE Input sebelum Connect.";
            receiverHintIsError = true;
        }
        else
        {
            juce::String playbackDeviceName;
            if (receiverDefaultPlaybackUsesCableInput (playbackDeviceName))
            {
                receiverHint = "Bahaya: default speaker Windows masih VB-CABLE. Pilih speaker/headphone asli.";
                receiverHintIsError = true;
            }
            else
            {
                receiverHint = "Aman: audio diarahkan ke CABLE Input, bukan speaker.";
            }
        }
    }

    senderButton.setToggleState (isSender, juce::dontSendNotification);
    receiverButton.setToggleState (! isSender, juce::dontSendNotification);
    deviceLabel.setText (isSender ? "Microphone sender" : "Output receiver (wajib CABLE Input)", juce::dontSendNotification);
    deviceHintLabel.setText (isSender
                                 ? "Audio balik diblokir; microphone hanya dikirim ke receiver."
                                 : receiverHint,
                             juce::dontSendNotification);
    deviceHintLabel.setColour (juce::Label::textColourId,
                               (! isSender && receiverHintIsError) ? errorColour() : juce::Colours::lightgrey);
    privacyMuteButton.setButtonText (isSender ? "Mute Mic" : "Privacy Mute");
    meterLabel.setText (isSender ? "Level mic dikirim" : "Level masuk ke CABLE", juce::dontSendNotification);
    updateAudioMeter();
}

void MiniMicEditor::updateConnectionControls()
{
    const auto disconnected = connectionState == ConnectionState::disconnected || connectionState == ConnectionState::error;
    connectButton.setButtonText (disconnected ? "Connect" : "Disconnect");
    groupEditor.setEnabled (disconnected);
    passwordEditor.setEnabled (disconnected);
    generateGroupButton.setEnabled (disconnected);
    generatePasswordButton.setEnabled (disconnected);
    senderButton.setEnabled (disconnected);
    receiverButton.setEnabled (disconnected);
    deviceCombo.setEnabled (disconnected);
    refreshDevicesButton.setEnabled (disconnected);
}

void MiniMicEditor::updateStatusForPeerCount()
{
    if (connectionState != ConnectionState::waitingForPeer && connectionState != ConnectionState::connected)
        return;

    const auto peers = processor.getNumberRemotePeers();
    if (peers == lastPeerCount)
        return;

    lastPeerCount = peers;
    if (peers > 0)
    {
        if (connectedPeerName.isNotEmpty())
            setConnectionState (ConnectionState::connected, "Terhubung ke " + connectedPeerName);
        else
            setConnectionState (ConnectionState::connected, "Terhubung — " + juce::String (peers) + " peer aktif");
    }
    else
    {
        connectedPeerName.clear();
        setConnectionState (ConnectionState::waitingForPeer);
    }
}

foleys::LevelMeterSource* MiniMicEditor::getActiveMeterSource()
{
    if (role == Role::sender)
        return &processor.getSendMeterSource();

    return &processor.getOutputMeterSource();
}

float MiniMicEditor::getMeterLevel (foleys::LevelMeterSource* source)
{
    if (source == nullptr)
        return 0.0f;

    source->decayIfNeeded();

    auto level = 0.0f;
    const auto numChannels = source->getNumChannels();
    for (int channel = 0; channel < numChannels; ++channel)
        level = juce::jmax (level, source->getRMSLevel (channel));

    if (level <= 0.00002f)
        return 0.0f;

    const auto db = 20.0f * std::log10 (juce::jmax (level, 0.000001f));
    return juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 48.0f);
}

void MiniMicEditor::updateAudioMeter()
{
    const auto isMuted = privacyMuteButton.getToggleState();
    const auto meterLevel = isMuted ? 0.0f : getMeterLevel (getActiveMeterSource());

    const auto changed = std::abs (audioMeterLevel - meterLevel) > 0.004f || audioMeterMuted != isMuted;
    audioMeterMuted = isMuted;
    audioMeterLevel = meterLevel;
    if (changed && ! audioMeterBounds.isEmpty())
        repaint (audioMeterBounds.expanded (2));

    const auto hasSignal = meterLevel > 0.08f;
    const auto baseText = ::juce::String (role == Role::sender ? "Level mic dikirim" : "Level masuk ke CABLE");
    const auto stateText = ::juce::String (isMuted ? "mute" : (hasSignal ? "ada sinyal" : "senyap"));
    meterLabel.setText (baseText + " — " + stateText, juce::dontSendNotification);
}

void MiniMicEditor::aooClientConnected (SonobusAudioProcessor*, bool success, const juce::String& errorMessage)
{
    auto safe = juce::Component::SafePointer<MiniMicEditor> (this);
    juce::MessageManager::callAsync ([safe, success, errorMessage]
    {
        if (safe == nullptr)
            return;
        if (! success)
        {
            safe->setConnectionState (ConnectionState::error, "Server: " + errorMessage);
            return;
        }

        safe->setConnectionState (ConnectionState::joiningGroup, "Masuk ke grup...");
        if (! safe->processor.joinServerGroup (safe->pendingGroup, safe->pendingPassword, false))
            safe->setConnectionState (ConnectionState::error, "Gagal mengirim permintaan grup");
    });
}

void MiniMicEditor::aooClientDisconnected (SonobusAudioProcessor*, bool, const juce::String& errorMessage)
{
    auto safe = juce::Component::SafePointer<MiniMicEditor> (this);
    juce::MessageManager::callAsync ([safe, errorMessage]
    {
        if (safe != nullptr)
        {
            if (! errorMessage.isEmpty())
                safe->connectedPeerName.clear();
            safe->setConnectionState (errorMessage.isEmpty() ? ConnectionState::disconnected : ConnectionState::error,
                                      errorMessage);
        }
    });
}

void MiniMicEditor::aooClientGroupJoined (SonobusAudioProcessor*, bool success, const juce::String&, const juce::String& errorMessage)
{
    auto safe = juce::Component::SafePointer<MiniMicEditor> (this);
    juce::MessageManager::callAsync ([safe, success, errorMessage]
    {
        if (safe == nullptr)
            return;
        safe->lastPeerCount = -1;
        safe->setConnectionState (success ? ConnectionState::waitingForPeer : ConnectionState::error,
                                  success ? juce::String() : "Group/password: " + errorMessage);
    });
}

void MiniMicEditor::aooClientPeerJoined (SonobusAudioProcessor*, const juce::String&, const juce::String& user)
{
    auto safe = juce::Component::SafePointer<MiniMicEditor> (this);
    juce::MessageManager::callAsync ([safe, user]
    {
        if (safe != nullptr)
        {
            safe->connectedPeerName = user;
            safe->lastPeerCount = -1;
            safe->setConnectionState (ConnectionState::connected, "Terhubung ke " + user);
        }
    });
}

void MiniMicEditor::aooClientPeerLeft (SonobusAudioProcessor*, const juce::String&, const juce::String&)
{
    auto safe = juce::Component::SafePointer<MiniMicEditor> (this);
    juce::MessageManager::callAsync ([safe]
    {
        if (safe != nullptr)
        {
            safe->lastPeerCount = -1;
            safe->connectedPeerName.clear();
            safe->updateStatusForPeerCount();
        }
    });
}

void MiniMicEditor::aooClientError (SonobusAudioProcessor*, const juce::String& errorMessage)
{
    auto safe = juce::Component::SafePointer<MiniMicEditor> (this);
    juce::MessageManager::callAsync ([safe, errorMessage]
    {
        if (safe != nullptr)
            safe->setConnectionState (ConnectionState::error, errorMessage);
    });
}
