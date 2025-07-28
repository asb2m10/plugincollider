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

#include "PanelCommon.h"

class GroupPanel : public juce::Component {
    PluginColliderAudioProcessor &processor;
    juce::Label label;
    juce::ValueTree vt;
    juce::TextButton addGroup;
    juce::TextButton addFx;
    juce::TextButton addMidiNote;

public:
    GroupPanel(const juce::ValueTree &groupVt, PluginColliderAudioProcessor &processor) : vt(groupVt), processor(processor) {
        if ( groupVt.hasType(IDs::rootnode) )
            label.setText("Group: Root Node", juce::dontSendNotification);
        else
            label.setText("Group: " + juce::String(vt[IDs::nodeid].toString()), juce::dontSendNotification);

        addAndMakeVisible(label);
        addAndMakeVisible(addGroup);
        addAndMakeVisible(addFx);
        addAndMakeVisible(addMidiNote);

        addGroup.setButtonText("Add Group");
        addMidiNote.setButtonText("Add Midi Synth...");
        addFx.setButtonText("Add Fx Synth...");

        addGroup.onClick = [this] {
            vt.addChild(this->processor.createGroupNodeVT(), -1, nullptr);
        };

        addFx.onClick = [this] {
            vt.addChild(this->processor.createFxNodeVT(), -1, nullptr);
        };

        addMidiNote.onClick = [this] {
            vt.addChild(this->processor.createMidiNoteNodeVT(), -1, nullptr);
        };
    }

    void resized() override {
        auto bounds = getBounds();
        bounds.removeFromTop(10);
        label.setBounds(bounds.removeFromTop(20));
        bounds.removeFromTop(5);

        auto buttonBounds = bounds.removeFromTop(25);
        buttonBounds.setWidth(200);
        addGroup.setBounds(buttonBounds);

        bounds.removeFromTop(5);
        buttonBounds = bounds.removeFromTop(25);
        buttonBounds.setWidth(200);
        addMidiNote.setBounds(buttonBounds);

        bounds.removeFromTop(5);
        buttonBounds = bounds.removeFromTop(25);
        buttonBounds.setWidth(200);
        addFx.setBounds(buttonBounds);
    }
};