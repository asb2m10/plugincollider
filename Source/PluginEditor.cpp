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
    //addAndMakeVisible(freeAll);
    freeAll.setButtonText("g_freeAll");
    freeAll.setBounds(10,10,200,50);
    freeAll.addListener(this);
    setSize (400, 300);
}

PluginColliderAudioProcessorEditor::~PluginColliderAudioProcessorEditor()
{
}

//==============================================================================
void PluginColliderAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    g.setColour (juce::Colours::white);
    g.setFont (15.0f);

    string listen = "Listening on: ";
    listen += std::to_string(audioProcessor.superCollider->portNum);

    g.drawFittedText (listen, getLocalBounds(), juce::Justification::centred, 1);
}

void PluginColliderAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}

void PluginColliderAudioProcessorEditor::buttonClicked (juce::Button* button) {
    audioProcessor.superCollider->freeAll();
}
