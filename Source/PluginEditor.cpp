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
    string listen = "Listening on: ";
    listen += std::to_string(audioProcessor.superCollider.portNum);
    portNumberLabel.setText(listen, juce::dontSendNotification);
    portNumberLabel.setBounds(10, 18, 130, 25);
    addAndMakeVisible(portNumberLabel);

    addAndMakeVisible(logViewer);
    logViewer.setBounds(10, 48, 680, 340);

    addAndMakeVisible(stats);
    stats.setBounds(200, 18, 480, 25);
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
