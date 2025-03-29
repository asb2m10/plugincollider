/*
    PluginCollider Copyright (c) 2025 Pascal Gauthier.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#pragma once

//#include <JuceHeader.h>
#include "SCProcess.h"
#include "CommandFifo.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include "UDPPort.h"

class PluginColliderAudioProcessorEditor;


namespace IDs
{
#define DECLARE_ID(name) const juce::Identifier name (#name);
    DECLARE_ID(root)
    DECLARE_ID(udpport)
    DECLARE_ID(synths)
    DECLARE_ID(synth)
    DECLARE_ID(autoload)
    DECLARE_ID(synthName)
    DECLARE_ID(synthBlob)
    DECLARE_ID(parameters)
    DECLARE_ID(parameter)
    DECLARE_ID(pName)
    DECLARE_ID(pDefaultValue)
    DECLARE_ID(pControlBus)
    DECLARE_ID(pRangeLow)
    DECLARE_ID(pRangeHigh)
};

//==============================================================================
/**
 */
class PluginColliderAudioProcessor : public juce::AudioProcessor,
      public juce::AudioProcessorParameter::Listener, public juce::ValueTree::Listener {
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
    juce::ValueTree pluginState;

    void valueTreePropertyChanged(juce::ValueTree &treeWhosePropertyHasChanged, const juce::Identifier &property) override;

    void loadSynthDef(SynthDef *def);
    void playSynth();
    void stopSynth();

  private:
    juce::String pluginPath;
    juce::String synthPath;
    juce::ValueTree synths;

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

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginColliderAudioProcessor)
};
