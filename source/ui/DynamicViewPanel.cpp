#pragma once

#include "DynamicViewPanel.h"
#include "SynthDefPanel.h"

void DynamicViewPanel::setEditableItem(juce::ValueTree item) {
    if ( currentItem == item ) {
        return;
    }

    currentItem = item;
    juce::Identifier type = item.getType();

    if ( component != nullptr ) {
        removeChildComponent(component.get());
        component.reset();
    }

    if ( type == IDs::controlbuses) {
        ControlBusPanel *label = new ControlBusPanel(item);
        component.reset(label);
        addAndMakeVisible(label);
        resized();
    }

    if ( type == IDs::synth ) {
        juce::Label *label = new juce::Label("synthName", item.getProperty(IDs::synthName));
        component.reset(label);
        addAndMakeVisible(label);
    }
}

void DynamicViewPanel::valueTreeChildRemoved(juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) {
}
