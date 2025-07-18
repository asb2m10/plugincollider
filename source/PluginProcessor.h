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

#include "ExtendedParameters.h"
#include "SCProcess.h"
#include "CommandFifo.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include "UDPPort.h"
#include "PluginModel.h"
#include "NodeContainer.h"

class PluginColliderAudioProcessorEditor;
//==============================================================================
/**
 */
class PluginColliderAudioProcessor : public juce::AudioProcessor,
      public juce::AudioProcessorParameter::Listener, public juce::ValueTree::Listener {
  public:
    SCProcess superCollider;
    UDPPort udpPort;

    void resetPluginState();

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
    void valueTreeChildRemoved(juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) override;


    bool replaceSynthDef(juce::MemoryBlock &block, juce::ValueTree &target);
    bool loadSynthDefLegacy(SynthDef *def);
    int rt_playSynth();
    void stopSynth();

    void resetStaticSynth() {
        juce::ValueTree synth = pluginState.getChildWithName(IDs::synths).getChildWithName(IDs::synth);
        bool isStaticSynth = synth.getProperty(IDs::staticSynth);
        if ( synth.isValid() ) {
            command.push([this, isStaticSynth](PluginColliderAudioProcessor &proc) {
                superCollider.rt_freeGroup(kDefaultGroupId);
                if ( isStaticSynth )
                    rt_playSynth();
            });
        }
    }

    juce::MidiKeyboardState midiKeyboardState;
    CommandFifo<PluginColliderAudioProcessor> command;

    juce::AudioProcessLoadMeasurer *getLoadMeasurer() {
        return &loadMeasurer;
    }

    void recompileState();
    
    void reloadNodeContainer();

    int getFreeNodeId() {
        int nodeCount = pluginState.getProperty(IDs::nodeCount, 1000);
        pluginState.setProperty(IDs::nodeCount, nodeCount + 1, nullptr);
        return nodeCount;
    }

  private:
    juce::String pluginPath;
    juce::String synthPath;

    juce::AudioParameterFloat *gain;
    ControlBusParameter *controlBus[NUMBER_OF_CONTROL_BUSES];

    juce::AudioProcessLoadMeasurer loadMeasurer;

    bool curActivity;

    juce::ApplicationProperties appProp;

    void parameterValueChanged (int parameterIndex, float newValue) override;
    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override {
    }

    bool bindUdpPort();

    struct SynthState {
        char synthName[127];
        // While easy to use, this allocates memory on the audio thread
        std::unordered_map<int, float> precompiledMapValue;
        std::unordered_map<int, int> controlBusMap;
        int freqIdx;
        int velocityIdx;
        int gateIdx;
        bool isStaticSynth;
    };
    SynthState synthState;

    int boundedMidiVoice[127];

    std::unique_ptr<NodeContainer> container;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginColliderAudioProcessor)
};
