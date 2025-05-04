#pragma once

#include "PluginProcessor.h"


/**
 * PluginCollider TreeViewItem base class.
 * 
 * \brief Base class for tree view items in the PluginCollider editor.
 */
class PCTreeItem : public juce::TreeViewItem {
protected:
    bool containsSubItems = true;
    juce::String itemName;
public:
    PCTreeItem() {
    }

    PCTreeItem(const juce::String& name) : itemName(name) {
    }

    ~PCTreeItem() override {
        clearSubItems();
    }

    bool mightContainSubItems() override { 
        return containsSubItems;
    }

    virtual void refresh() {
    }

    void paintItem (juce::Graphics& g, int width, int height) {
        g.setColour(getOwnerView()->findColour(juce::Label::textColourId));
        g.setFont(height * 0.7f);
        g.drawText(itemName, 4, 0, width - 4, height, juce::Justification::centredLeft, true);
    }
};

class RootItem : public PCTreeItem {
public:
    RootItem(PluginColliderAudioProcessor &p);
};