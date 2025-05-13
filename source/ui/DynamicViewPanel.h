
#pragma once
#include "PluginProcessor.h"

class DynamicViewPanel : public juce::Component, public juce::ValueTree::Listener {
    std::unique_ptr<juce::Component> component;
    juce::ValueTree currentItem;
public:
    void setEditableItem(juce::ValueTree item);
    void clearEditableItem() {
        if (component != nullptr) {
            removeChildComponent(component.get());
            component.reset();
            currentItem = juce::ValueTree();
        }
    }

    //void valueTreeChildAdded(juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenAdded) override;
    void valueTreeChildRemoved (juce::ValueTree& parentTree,
        juce::ValueTree& childWhichHasBeenRemoved,
        int indexFromWhichChildWasRemoved) override;

    void resized() override {
        if (component != nullptr) {
            component->setBounds(0, 0, getWidth(), getHeight());
        }
    }
};
