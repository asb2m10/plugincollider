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
#include "DynamicViewPanel.h"

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

    PCTreeItem(const juce::String& name, bool containsSubItem = true) : itemName(name) {
        this->containsSubItems = containsSubItem;
    }

    bool mightContainSubItems() override { 
        return containsSubItems;
    }

    virtual void refresh() {
    }

    juce::String getUniqueName() const override {
        return itemName;
    }

    void paintItem (juce::Graphics& g, int width, int height) override {
        g.setColour(getOwnerView()->findColour(juce::Label::textColourId));
        g.setFont(height * 0.7f);
        if ( isSelected() ) {
            juce::Font font = g.getCurrentFont();
            font.setBold(true);
            g.setFont(font);
        }
        g.drawText(itemName, 4, 0, width - 4, height, juce::Justification::centredLeft, true);
    }
};

class RootItem : public PCTreeItem {
public:
    RootItem(PluginColliderAudioProcessor &p, DynamicViewPanel &panel);
};
