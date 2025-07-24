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
#include "PluginProcessor.h"

void PluginColliderAudioProcessor::getStateInformation(juce::MemoryBlock &destData) {
    std::unique_ptr<juce::XmlElement> xml(pluginState.createXml());
    copyXmlToBinary(*xml, destData);
}

void PluginColliderAudioProcessor::setStateInformation(const void *data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    juce::ValueTree tmpState = juce::ValueTree::fromXml(*xmlState);

    resetPluginState();


    return;

    if ( tmpState.getProperty(IDs::version) != IDS_VERSION ) {
        resetPluginState();
        //recompileState();
    } else {
        pluginState = tmpState;
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
        }
        const auto details = juce::AudioProcessorListener::ChangeDetails{}.withParameterInfoChanged(true);
        updateHostDisplay(details);
    }

    if ( superCollider.isRunning() ) {
        command.push([&] (PluginColliderAudioProcessor &proc) {
            proc.rt_loadSynthDef(pluginState.getChildWithName(IDs::rootnode));
        });

        reloadNodeContainer();
    }
}

void PluginColliderAudioProcessor::resetPluginState() {
    pluginState.removeListener(this);
    pluginState.removeAllChildren(nullptr);

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
    juce::ValueTree controlBus = juce::ValueTree(IDs::controlbus);

    juce::ValueTree scratchpad = juce::ValueTree(IDs::scratchpad);
    scratchpad.setProperty(IDs::spCode, "", nullptr);
    pluginState.addChild(scratchpad, -1, nullptr);

    pluginState.setProperty(IDs::nodeCount , 1000, nullptr);
    pluginState.setProperty(IDs::version, IDS_VERSION, nullptr);
    pluginState.addListener(this);
}

