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

    configButton.setButtonText("Configure");
    addAndMakeVisible(configButton);
    configButton.setBounds(10, 18, 130, 25);

    configButton.onClick = [ this ] {
        settingsWindow = new juce::AlertWindow("PluginCollider settings", "", juce::AlertWindow::NoIcon);

        settingsWindow->addTextBlock("Prefred UDP port");
        settingsWindow->addTextEditor("udpPort", juce::String(audioProcessor.udpPort));
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
                prop->setValue("udpPort", this->settingsWindow->getTextEditorContents("udpPort"));
                prop->setValue("pluginPath", this->settingsWindow->getTextEditorContents("pluginPath"));
                prop->setValue("synthPath", this->settingsWindow->getTextEditorContents("synthPath"));
                audioProcessor.appProp.saveIfNeeded();
            }
        }), true);

    };

    addAndMakeVisible(logViewer);
    logViewer.setBounds(10, 48, 680, 340);

    addAndMakeVisible(stats);
    stats.setBounds(212, 18, 480, 25);
    stats.setJustificationType(juce::Justification::centredRight);

    startTimer(400);

    setSize(700, 400);
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
                      "udp port: %i units: %i graph: %i groups: %i", audioProcessor.superCollider.portNum, worldStats.mNumUnits,
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
