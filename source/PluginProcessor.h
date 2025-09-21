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
    void processBlockBypassed(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
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
    juce::ValueTree createMidiNoteNodeVT();
    juce::ValueTree createFxNodeVT();
    juce::ValueTree createGroupNodeVT();

    bool getActivityMonitor();

    SuperLogger logger;
    bool setUdpPort(juce::String value);

    friend PluginColliderAudioProcessorEditor;
    juce::ValueTree pluginState;
    
    void valueTreePropertyChanged(juce::ValueTree &treeWhosePropertyHasChanged, const juce::Identifier &property) override;
    void valueTreeChildRemoved(juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) override;
    void valueTreeChildAdded(juce::ValueTree&, juce::ValueTree&) override;
    void valueTreeChildOrderChanged(juce::ValueTree&, int, int) override;

    /**
     * Replace the synthdef in the plugin state with the one in the memory block.
     */
    bool replaceSynthDef(juce::MemoryBlock &block, juce::ValueTree &target);

    /**
     * Load the synthdef from the plugin state into the supercollider world ; usually when the server is booted.
     */
    void rt_loadSynthDef(juce::ValueTree root);

    juce::MidiKeyboardState midiKeyboardState;

    juce::AudioProcessLoadMeasurer *getLoadMeasurer() {
        return &loadMeasurer;
    }

    void reloadNodeContainer();

    int getFreeNodeId() {
        int nodeCount = pluginState.getProperty(IDs::nodeCount, 1000);
        pluginState.setProperty(IDs::nodeCount, nodeCount + 1, nullptr);
        return nodeCount;
    }

    // Reaper doesnt signal the plugin that it is suspended, we need to check the timestamp of the
    // last audio proc call to detect that the audioProc is not run anymore
    bool isAudioProcSuspended();

    /**
     * Executes a function on the audio thread. Might not get executed or delayed if the plugin
     * is bypassed. Avoid using this for synchronous (request/response) operations
     */
    template <typename Item>
    bool execOnAudioThread(Item&& item) noexcept {
        if ( isAudioProcSuspended() )
            return false;

        command.push(std::forward<Item>(item));
        return true;
    }

    /**
     * Executes a function by locking the SuperCollider global lock. The block might not
     * get executed if the supercollider world is not running.
     */
    bool execSyncWorld(std::function<void()> func);

    void rebootServer();

  private:
    CommandFifo<PluginColliderAudioProcessor> command;
    juce::String pluginPath;
    juce::String synthDefPath;
    juce::AudioParameterFloat *gain;
    ControlBusParameter *controlBus[NUMBER_OF_CONTROL_BUSES];

    juce::AudioProcessLoadMeasurer loadMeasurer;

    bool curActivity;

    juce::ApplicationProperties appProp;

    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {
    }

    /**
     * Tells if the event is a base event for reloading nodes.
     * @param parentTree the parent tree of the event
     */
    bool isNodeReloadBaseEvent(juce::ValueTree& parentTree);

    bool bindUdpPort();

    std::unique_ptr<NodeContainer> container;

    double lastProcThreshold;
    double lastProcRun;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginColliderAudioProcessor)
};
