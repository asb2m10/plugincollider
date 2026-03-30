/*
    PluginCollider Copyright (c) 2026 Pascal Gauthier.

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

#include "SynthDefWatcher.h"
#include "PluginProcessor.h"
#include "SpecFile.h"
#include "NodeContainer.h"

SynthDefWatcher::SynthDefWatcher(PluginColliderAudioProcessor &processor)
    : processor(processor) {}

void SynthDefWatcher::start(const juce::String &path) {
    stopTimer();
    fileTimestamps.clear();
    currentWatchPath = path;
    if (path.isNotEmpty())
        startTimer(1000);
}

void SynthDefWatcher::stop() {
    stopTimer();
    fileTimestamps.clear();
    currentWatchPath.clear();
}

void SynthDefWatcher::timerCallback() {
    if (currentWatchPath.isEmpty())
        return;

    juce::File dir(currentWatchPath);
    if (!dir.isDirectory())
        return;

    for (const auto &entry : juce::RangedDirectoryIterator(dir, false, "*.scsyndef")) {
        auto file = entry.getFile();
        auto modTime = file.getLastModificationTime();
        auto key = file.getFullPathName();

        auto it = fileTimestamps.find(key);
        if (it == fileTimestamps.end()) {
            // First scan: just record timestamp, don't reload
            fileTimestamps[key] = modTime;
        } else if (modTime > it->second) {
            fileTimestamps[key] = modTime;
            reloadFile(file);
        }
    }
}

void SynthDefWatcher::reloadFile(const juce::File &file) {
    juce::MemoryBlock content;
    if (!file.loadFileAsData(content))
        return;

    try {
        SynthDef synthDef(content);
        auto defName = synthDef.getName();
        auto specs = loadSpecFile(file);

        // Disable ValueTree listener to batch all changes
        processor.pluginState.removeListener(&processor);

        bool anyUpdated = updateNodesWithDef(
            processor.pluginState.getChildWithName(IDs::rootnode),
            defName, content, specs);

        // Re-enable listener
        processor.pluginState.addListener(&processor);

        if (anyUpdated) {
            processor.reloadNodeContainer();
            const auto details = juce::AudioProcessorListener::ChangeDetails{}.withParameterInfoChanged(true);
            processor.updateHostDisplay(details);
            processor.logger.scprintf("Auto-reloaded SynthDef: %s\n", defName.toRawUTF8());
        }
    } catch (const InvalidSynthDef &) {
        // File may be mid-write or corrupt — ignore, next poll will retry
    }
}

bool SynthDefWatcher::updateNodesWithDef(juce::ValueTree root, const juce::String &defName,
                                         juce::MemoryBlock &content,
                                         const SpecList &specs) {
    bool updated = false;
    for (int i = 0; i < root.getNumChildren(); i++) {
        auto child = root.getChild(i);
        if (child.hasType(IDs::groupnode)) {
            if (updateNodesWithDef(child, defName, content, specs))
                updated = true;
        } else if (child.hasType(IDs::fxnode) || child.hasType(IDs::notenode)) {
            if (child.getProperty(IDs::synthName).toString() == defName) {
                updateNodeSynthDef(child, content, specs);
                updated = true;
            }
        }
    }
    return updated;
}

void SynthDefWatcher::updateNodeSynthDef(juce::ValueTree node, juce::MemoryBlock &content,
                                         const SpecList &specs) {
    // Save existing parameter state keyed by name
    struct ParamState {
        int controlBus;
        juce::var currentValue;
    };
    std::map<juce::String, ParamState> savedParams;
    auto oldParams = node.getChildWithName(IDs::parameters);
    for (int i = 0; i < oldParams.getNumChildren(); i++) {
        auto p = oldParams.getChild(i);
        savedParams[p[IDs::pName].toString()] = {
            static_cast<int>(p.getProperty(IDs::pControlBus, -1)),
            p.getProperty(IDs::pCurrentValue)
        };
    }

    // Replace the SynthDef — no specs passed, so no auto-assign
    if (!processor.replaceSynthDef(content, node))
        return;

    // Restore saved state for params that still exist by name;
    // always use spec range when available (for both old and new params)
    auto newParams = node.getChildWithName(IDs::parameters);
    for (int i = 0; i < newParams.getNumChildren(); i++) {
        auto p = newParams.getChild(i);
        juce::String pName = p[IDs::pName].toString();

        // Apply spec range if available (overrides guessed range)
        auto specRange = findSpec(specs, pName);
        if (specRange.isNotEmpty())
            p.setProperty(IDs::pRange, specRange, nullptr);

        // Restore control bus and current value for existing params
        auto it = savedParams.find(pName);
        if (it != savedParams.end()) {
            int cbIdx = it->second.controlBus;
            p.setProperty(IDs::pControlBus, cbIdx, nullptr);
            if (!it->second.currentValue.isVoid())
                p.setProperty(IDs::pCurrentValue, it->second.currentValue, nullptr);

            // Sync control bus range with updated param range
            // (listener is disabled, so update ValueTree AND parameter object directly)
            if (cbIdx >= 0 && cbIdx < NUMBER_OF_CONTROL_BUSES) {
                auto controlBuses = processor.pluginState.getChildWithName(IDs::controlbuses);
                controlBuses.getChild(cbIdx).setProperty(IDs::cbRange, p.getProperty(IDs::pRange), nullptr);
                PluginColliderRange range(p.getProperty(IDs::pRange));
                processor.controlBus[cbIdx]->setRange(range);
            }
        }
    }
}
