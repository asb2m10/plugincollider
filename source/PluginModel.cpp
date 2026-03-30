/*
    PluginCollider Copyright (c) 2025-2026 Pascal Gauthier.

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

#include "PluginProcessor.h"

void PluginColliderAudioProcessor::getStateInformation(juce::MemoryBlock &destData) {
    juce::ValueTree controlBuses = pluginState.getChildWithName(IDs::controlbuses);
    for (int idx=0;idx<NUMBER_OF_CONTROL_BUSES;idx++) {
        juce::ValueTree cb = controlBuses.getChild(idx);
        if ( cb.isValid() ) {
            float dawValue = controlBus[idx]->convertTo0to1(controlBus[idx]->get());
            cb.setProperty(IDs::cbValue, dawValue, nullptr);
        }
    }
    std::unique_ptr<juce::XmlElement> xml(pluginState.createXml());
    copyXmlToBinary(*xml, destData);
}

void PluginColliderAudioProcessor::setStateInformation(const void *data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    juce::ValueTree tmpState = juce::ValueTree::fromXml(*xmlState);

    if ( tmpState.getProperty(IDs::version) != IDS_VERSION ) {
        resetPluginState();
    } else {
        pluginState.removeAllChildren(nullptr);
        pluginState.removeListener(this);
        pluginState = tmpState;
        pluginState.addListener(this);
    }

    for (int idx=0;idx<NUMBER_OF_CONTROL_BUSES;idx++) {
        juce::ValueTree rootbus = pluginState.getChildWithName(IDs::controlbuses);
        if ( rootbus.isValid() && rootbus.getNumChildren() > idx ) {
            juce::ValueTree cb = rootbus.getChild(idx);
            if ( cb.hasProperty(IDs::cbRange) ) {
                PluginColliderRange range(cb.getProperty(IDs::cbRange));
                controlBus[idx]->setRange(range);
            }
            if ( cb.hasProperty(IDs::cbName) ) {
                juce::String name = cb.getProperty(IDs::cbName);
                controlBus[idx]->setName(name);
            }
            if ( cb.hasProperty(IDs::cbValue) ) {
                float value = cb.getProperty(IDs::cbValue);
                controlBus[idx]->setValueNotifyingHost(value);
            }
        }
        const auto details = juce::AudioProcessorListener::ChangeDetails{}.withParameterInfoChanged(true);
        updateHostDisplay(details);
    }

    execSyncWorld([this]() {
        rt_loadSynthDef(pluginState.getChildWithName(IDs::rootnode));
    });
    reloadNodeContainer();
}

void PluginColliderAudioProcessor::resetPluginState() {
    pluginState.removeAllChildren(nullptr);
    pluginState.removeListener(this);

    juce::ValueTree srvRoot = juce::ValueTree(IDs::srvRoot);
    juce::PropertiesFile *prop = appProp.getUserSettings();
    srvRoot.setProperty(IDs::srvMaxWireBufs, prop->getIntValue("srvMaxWireBufs", 64), nullptr);
    srvRoot.setProperty(IDs::srvRealTimeMemorySize, prop->getIntValue("srvRealTimeMemorySize", 8192), nullptr);
    srvRoot.setProperty(IDs::srvNumBuffers, prop->getIntValue("srvNumBuffers", 1024), nullptr);
    srvRoot.setProperty(IDs::srvMaxLogins, prop->getIntValue("srvMaxLogins", 32), nullptr);
    srvRoot.setProperty(IDs::srvAlwaysSyncNodes, true, nullptr);

#ifdef WIN32
    juce::String pluginPath = prop->getValue("pluginPath", "C:\\Program Files\\SuperCollider\\plugins");
#elif __APPLE__
    juce::String pluginPath = prop->getValue("pluginPath", "/Applications/SuperCollider.app/Contents/Resources/plugins");
#else
    juce::String pluginPath = prop->getValue("pluginPath", "/usr/lib/SuperCollider/plugins");
#endif
    srvRoot.setProperty(IDs::srvPluginPath, pluginPath, nullptr);

    juce::String synthDefPath = prop->getValue("synthPath", "");
    srvRoot.setProperty(IDs::srvSynthDefPath, synthDefPath, nullptr);
    srvRoot.setProperty(IDs::srvAutoReloadSynthDefs, prop->getBoolValue("autoReloadSynthDefs", false), nullptr);

    pluginState.addChild(srvRoot, -1, nullptr);

    juce::ValueTree controlBusses = juce::ValueTree(IDs::controlbuses);
    for(int i=0;i<NUMBER_OF_CONTROL_BUSES;i++) {
        juce::ValueTree controlBus = juce::ValueTree(IDs::controlbus);
        controlBus.setProperty(IDs::cbName, juce::String("Control Bus ") + juce::String(i), nullptr);
        controlBus.setProperty(IDs::cbIdx, i, nullptr);
        controlBus.setProperty(IDs::cbRange, "0 1 0.001", nullptr);
        controlBusses.addChild(controlBus, i, nullptr);
    }
    pluginState.addChild(juce::ValueTree(IDs::rootnode), 0, nullptr);
    pluginState.addChild(controlBusses, 0, nullptr);

    juce::ValueTree scratchpad = juce::ValueTree(IDs::scratchpad);
    scratchpad.setProperty(IDs::spCode, "", nullptr);
    pluginState.addChild(scratchpad, -1, nullptr);

    pluginState.setProperty(IDs::nodeCount , 1000, nullptr);
    pluginState.setProperty(IDs::version, IDS_VERSION, nullptr);
    pluginState.addListener(this);
}

juce::ValueTree PluginColliderAudioProcessor::createMidiNoteNodeVT() {
    juce::ValueTree newSynth = juce::ValueTree(IDs::notenode);
    newSynth.setProperty(IDs::nodename, "Midi Note Node", nullptr);
    newSynth.setProperty(IDs::nodeid, getFreeNodeId(), nullptr);
    newSynth.setProperty(IDs::synthNoteRange, "0 127", nullptr);
    newSynth.setProperty(IDs::synthMono, false, nullptr);
    return newSynth;
}

juce::ValueTree PluginColliderAudioProcessor::createFxNodeVT() {
    juce::ValueTree newSynth = juce::ValueTree(IDs::fxnode);
    newSynth.setProperty(IDs::nodename, "FX Node", nullptr);
    newSynth.setProperty(IDs::nodeid, getFreeNodeId(), nullptr);
    return newSynth;
}

juce::ValueTree PluginColliderAudioProcessor::createGroupNodeVT() {
    juce::ValueTree newNode = juce::ValueTree(IDs::groupnode);
    newNode.setProperty(IDs::nodename, "Group", nullptr);
    newNode.setProperty(IDs::nodeid, getFreeNodeId(), nullptr);
    return newNode;
}
