// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "SonobusPluginProcessor.h"

#include <functional>

class MiniMicEditor final : public ::juce::AudioProcessorEditor,
                            private ::juce::Timer,
                            private SonobusAudioProcessor::ClientListener
{
public:
    explicit MiniMicEditor (SonobusAudioProcessor& processorToUse);
    ~MiniMicEditor() override;

    void paint (::juce::Graphics& g) override;
    void resized() override;

    std::function<::juce::AudioDeviceManager*()> getAudioDeviceManager;
    void audioDeviceManagerReady();

private:
    enum class Role
    {
        sender,
        receiver
    };

    enum class ConnectionState
    {
        disconnected,
        connectingServer,
        joiningGroup,
        waitingForPeer,
        connected,
        error
    };

    void timerCallback() override;
    void setRole (Role newRole);
    void applyRolePolicy();
    void refreshAudioDevices();
    void applySelectedAudioDevice();
    bool validateConnectionInputs (::juce::String& errorMessage) const;
    bool receiverUsesCableInput() const;
    bool receiverDefaultPlaybackUsesCableInput (::juce::String& playbackDeviceName) const;
    void connectOrDisconnect();
    void beginConnect();
    void disconnect();
    void setConnectionState (ConnectionState newState, const ::juce::String& detail = {});
    void generateGroupId();
    void generatePassword();
    ::juce::String makeRandomCode (int length, bool withSeparator) const;
    ::juce::String makePeerName() const;
    void updateRoleControls();
    void updateConnectionControls();
    void updateStatusForPeerCount();
    void updateAudioMeter();
    ::foleys::LevelMeterSource* getActiveMeterSource();
    static float getMeterLevel (::foleys::LevelMeterSource* source);
    static ::juce::String makeSafePeerNamePart (::juce::String text);

    void aooClientConnected (SonobusAudioProcessor*, bool success, const ::juce::String& errorMessage) override;
    void aooClientDisconnected (SonobusAudioProcessor*, bool success, const ::juce::String& errorMessage) override;
    void aooClientGroupJoined (SonobusAudioProcessor*, bool success, const ::juce::String& group, const ::juce::String& errorMessage) override;
    void aooClientPeerJoined (SonobusAudioProcessor*, const ::juce::String& group, const ::juce::String& user) override;
    void aooClientPeerLeft (SonobusAudioProcessor*, const ::juce::String& group, const ::juce::String& user) override;
    void aooClientError (SonobusAudioProcessor*, const ::juce::String& errorMessage) override;

    SonobusAudioProcessor& processor;
    Role role { Role::sender };
    ConnectionState connectionState { ConnectionState::disconnected };

    ::juce::Label titleLabel;
    ::juce::Label subtitleLabel;
    ::juce::ToggleButton senderButton { "Sender" };
    ::juce::ToggleButton receiverButton { "Receiver" };

    ::juce::Label groupLabel;
    ::juce::TextEditor groupEditor;
    ::juce::TextButton generateGroupButton { "Generate ID" };
    ::juce::Label passwordLabel;
    ::juce::TextEditor passwordEditor;
    ::juce::TextButton generatePasswordButton { "Generate Password" };

    ::juce::Label deviceLabel;
    ::juce::ComboBox deviceCombo;
    ::juce::TextButton refreshDevicesButton { "Refresh" };
    ::juce::Label deviceHintLabel;

    ::juce::TextButton connectButton { "Connect" };
    ::juce::ToggleButton privacyMuteButton { "Mute Mic" };
    ::juce::Label meterLabel;
    ::juce::Rectangle<int> audioMeterBounds;
    float audioMeterLevel { 0.0f };
    bool audioMeterMuted { false };
    ::juce::Label statusLabel;
    ::juce::Label serverLabel;
    ::juce::Label securityLabel;

    ::juce::StringArray inputDeviceNames;
    ::juce::StringArray outputDeviceNames;
    ::juce::String pendingGroup;
    ::juce::String pendingPassword;
    ::juce::String peerName;
    ::juce::String connectedPeerName;
    int lastPeerCount { -1 };
    bool updatingDevices { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MiniMicEditor)
};
