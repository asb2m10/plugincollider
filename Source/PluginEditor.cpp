/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
PluginColliderAudioProcessorEditor::PluginColliderAudioProcessorEditor (PluginColliderAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{    
    string listen = "Listening on: ";
    listen += std::to_string(audioProcessor.superCollider.portNum);
    portNumberLabel.setText(listen, juce::dontSendNotification);
    portNumberLabel.setBounds (10, 18, 130, 25);
    addAndMakeVisible(portNumberLabel);
    
    //startTimer(100);
    
    setSize (700, 400);
}

PluginColliderAudioProcessorEditor::~PluginColliderAudioProcessorEditor()
{
    stopTimer();
}

void PluginColliderAudioProcessorEditor::timerCallback()
{
    
}

//==============================================================================
void PluginColliderAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void PluginColliderAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}
