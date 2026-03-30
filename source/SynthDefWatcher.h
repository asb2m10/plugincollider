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

#pragma once

#include <juce_events/juce_events.h>
#include <juce_data_structures/juce_data_structures.h>
#include "SpecFile.h"
#include <map>

class PluginColliderAudioProcessor;

/**
 * Watches a directory for .scsyndef file changes and auto-reloads matching nodes
 * in the ValueTree, preserving control bus assignments and current parameter values.
 *
 * Addresses: https://github.com/asb2m10/plugincollider/issues/64
 */
class SynthDefWatcher : private juce::Timer {
public:
    SynthDefWatcher(PluginColliderAudioProcessor &processor);

    /** Start watching a directory. Empty path stops the watcher. */
    void start(const juce::String &path);

    /** Stop watching. */
    void stop();

private:
    PluginColliderAudioProcessor &processor;
    std::map<juce::String, juce::Time> fileTimestamps;
    juce::String currentWatchPath;

    void timerCallback() override;
    void reloadFile(const juce::File &file);
    bool updateNodesWithDef(juce::ValueTree root, const juce::String &defName,
                            juce::MemoryBlock &content,
                            const SpecList &specs);
    void updateNodeSynthDef(juce::ValueTree node, juce::MemoryBlock &content,
                            const SpecList &specs);
};
