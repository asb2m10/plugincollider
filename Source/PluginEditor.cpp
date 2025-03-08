/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginEditor.h"
#include "PluginProcessor.h"

//==============================================================================
PluginColliderAudioProcessorEditor::PluginColliderAudioProcessorEditor(
    PluginColliderAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p),
      logViewer(&(p.logger.content)) {

    addAndMakeVisible(udpPort);
    addAndMakeVisible(setUdpPortButton);

    udpPort.setBounds(10, 8, 70, 25);
    udpPort.setInputFilter(new juce::TextEditor::LengthAndCharacterRestriction(5, "0123456789"), true);
    udpPort.setText(juce::String(p.udpPort.getListenPort()), true);
    setUdpPortButton.setBounds(70, 8, 100, 25);
    setUdpPortButton.setButtonText("Set UDP Port");
    setUdpPortButton.onClick = [this] () {
        juce::String port = udpPort.getText();
        audioProcessor.setUdpPort(port);
    };

    configButton.setButtonText("Configure");
    addAndMakeVisible(configButton);
    configButton.setBounds(10, 38, 130, 25);

    configButton.onClick = [ this ] {
        settingsWindow = new juce::AlertWindow("PluginCollider settings", "", juce::AlertWindow::NoIcon);

        settingsWindow->addTextBlock("Plugin path");
        settingsWindow->addTextEditor("pluginPath", audioProcessor.pluginPath);
        settingsWindow->addTextBlock("Scsynth path");
        settingsWindow->addTextEditor("synthPath", audioProcessor.synthPath);
        settingsWindow->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey, 0, 0));
        settingsWindow->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey, 0, 0));

        settingsWindow->enterModalState(true, juce::ModalCallbackFunction::create([this](int r) {
            if (r) {
                scprintf("RESTART PLUGIN FOR SETTINGS TO TAKE EFFECT\n");
                juce::PropertiesFile *prop = audioProcessor.appProp.getUserSettings();
                prop->setValue("pluginPath", this->settingsWindow->getTextEditorContents("pluginPath"));
                prop->setValue("synthPath", this->settingsWindow->getTextEditorContents("synthPath"));
                audioProcessor.appProp.saveIfNeeded();
            }
        }), true);

    };

    // For now this is for debugging
    // addAndMakeVisible(cb1);
    cb1.setSliderStyle(juce::Slider::Rotary);
    cb1.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, true, 0, 0);
    cb1.setBounds(150, 10, 50, 50);
    juce::AudioParameterFloat *parameter = p.controlBus[0];
    cb1Attachment.reset(new juce::SliderParameterAttachment(*parameter, cb1, nullptr));

    addAndMakeVisible(logViewer);
    logViewer.setBounds(10, 75, 680, 340);

    addAndMakeVisible(rebootButton);
    rebootButton.setBounds(542, 38, 150, 25);
    rebootButton.setButtonText("Reboot server");
    rebootButton.onClick = [this] {
        audioProcessor.superCollider.reboot();
    };

    addAndMakeVisible(stats);
    stats.setBounds(212, 8, 480, 25);
    stats.setJustificationType(juce::Justification::centredRight);

    startTimer(400);
    setSize(700, 450);
}

PluginColliderAudioProcessorEditor::~PluginColliderAudioProcessorEditor() {
    stopTimer();
}

void PluginColliderAudioProcessorEditor::timerCallback() {
    if (logLines != audioProcessor.logger.content.size()) {
        logLines = audioProcessor.logger.content.size();
        logViewer.setText(audioProcessor.logger.content.joinIntoString(""));
        logViewer.moveCaretToEnd();
    }

    SCProcess::WorldStats worldStats =
        audioProcessor.superCollider.getWorldStats();
    stats.setText(juce::String::formatted(
                      "units: %i graph: %i groups: %i", worldStats.mNumUnits,
                      worldStats.mNumGraphs, worldStats.mNumGroups),
                  juce::dontSendNotification);
}

//==============================================================================
void PluginColliderAudioProcessorEditor::paint(juce::Graphics &g) {
    // (Our component is opaque, so we must completely fill the background with
    // a solid colour)
    g.fillAll(
        getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(juce::Colours::white);
    g.setFont(15.0f);
}

void PluginColliderAudioProcessorEditor::resized() {
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}
