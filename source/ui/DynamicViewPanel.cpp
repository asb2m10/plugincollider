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

#include "DynamicViewPanel.h"
#include "PanelControlBus.h"
#include "PanelSynthDef.h"
#include "PanelScratchpad.h"

void DynamicViewPanel::setEditableItem(juce::ValueTree item, PluginColliderAudioProcessor &processor) {
    if ( currentItem == item ) {
        return;
    }

    currentItem = item;
    juce::Identifier type = item.getType();

    if ( component != nullptr ) {
        removeChildComponent(component.get());
        component.reset();
    }

    if ( type == IDs::scratchpad ) {
        ScratchpadPanel *scratchpad = new ScratchpadPanel(item, processor);
        component.reset(scratchpad);
        addAndMakeVisible(scratchpad);
        resized();
    }

    if ( type == IDs::controlbuses ) {
        PanelControlBus *cbPanel = new PanelControlBus(item);
        component.reset(cbPanel);
        addAndMakeVisible(cbPanel);
        resized();
    }

    if ( type == IDs::fxnode ) {
        PanelSynthDefFx *panel = new PanelSynthDefFx(item, processor);
        component.reset(panel);
        addAndMakeVisible(panel);
        resized();
    }

    if ( type == IDs::notenode ) {
        PanelSynthDefMidi *panel = new PanelSynthDefMidi(item, processor);
        component.reset(panel);
        addAndMakeVisible(panel);
        resized();
    }
}

void DynamicViewPanel::valueTreeChildRemoved(juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) {
}
