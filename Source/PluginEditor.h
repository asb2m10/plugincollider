/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

//#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "LogComponent.h"

//==============================================================================
/**
*/
class PluginColliderAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                            public juce::Button::Listener,
                                            public juce::Timer
{
public:
    PluginColliderAudioProcessorEditor (PluginColliderAudioProcessor&);
    ~PluginColliderAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    void buttonClicked (juce::Button* button) override;

    virtual void timerCallback() override;
    
private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    PluginColliderAudioProcessor& audioProcessor;
    juce::TextButton freeAll;
    
    juce::Label portNumberLabel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginColliderAudioProcessorEditor)
};
