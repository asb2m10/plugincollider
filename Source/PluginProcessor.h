/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

//#include <JuceHeader.h>
#include "SCProcess.h"
#include "CommandFifo.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include "UDPPort.h"

class PluginColliderAudioProcessorEditor;

// Dirty cheap logger
class SuperLogger : public juce::Logger {
  public:
    juce::StringArray content;
    void printf(const char *fmt, ...);
    void logMessage(const juce::String &message) override {
        if (content.size() > 4096)
            content.removeRange(0, 2048);
        content.add(message);
    }
};

namespace IDs
{
#define DECLARE_ID(name) const juce::Identifier name (#name);
    DECLARE_ID(ROOT)
    DECLARE_ID(udpPort)
};

//==============================================================================
/**
 */
class PluginColliderAudioProcessor : public juce::AudioProcessor, public juce::AudioProcessorParameter::Listener {
  public:
    SCProcess superCollider;
    UDPPort udpPort;

    //==============================================================================
    PluginColliderAudioProcessor();
    ~PluginColliderAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout &layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

    //==============================================================================
    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String &newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock &destData) override;
    void setStateInformation(const void *data, int sizeInBytes) override;

    bool getActivityMonitor();

    SuperLogger logger;
    bool setUdpPort(juce::String value);

    friend PluginColliderAudioProcessorEditor;

  private:
    juce::String pluginPath;
    juce::String synthPath;

    juce::AudioParameterFloat *gain;
    juce::AudioParameterFloat *controlBus[32];
    CommandFifo<PluginColliderAudioProcessor> command;

    bool curActivity;

    juce::ApplicationProperties appProp;

    void parameterValueChanged (int parameterIndex, float newValue) override {
        command.push([this, parameterIndex, newValue](PluginColliderAudioProcessor &proc) {
            this->superCollider.setControlBusValue(parameterIndex-1, newValue);
        });
    }

    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override {
    }

    bool bindUdpPort();
    juce::ValueTree pluginState;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginColliderAudioProcessor)
};
