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

class ControlBusTable : public VTTableList {
public:
    ControlBusTable(juce::ValueTree vt) {
        addColumn(IDs::cbName, "Name", 100);
        addColumn(IDs::cbRange, "Range", 400);
        setContent(vt);
    }

    juce::Component* refreshComponentForCell(int rowNumber, int columnId,
                                            bool isRowSelected, juce::Component* existingComponentToUpdate) override {
        if ( columnId == 1 ) {
            juce::Identifier targetId = columnIds[columnId - 1];
            auto* textEditor = static_cast<EditableTextCustomComponent*>(existingComponentToUpdate);
            if ( textEditor == nullptr ) {
                textEditor = new EditableTextCustomComponent(*this);
            }
            textEditor->setRowAndColumn(rowNumber, columnId);
            return textEditor;
        }

        if ( columnId == 2 ) {
            auto* rangeEditor = static_cast<RangeEditor*>(existingComponentToUpdate);
            if ( rangeEditor == nullptr ) {
                rangeEditor = new RangeEditor();
            }
            rangeEditor->assignValueTree(vt.getChild(rowNumber), IDs::cbRange);
            return rangeEditor;
        }
    }
};

class PanelControlBus : public juce::Component {
    juce::Label label;
    ControlBusTable table;
public:
    PanelControlBus(juce::ValueTree vt) : table(vt) {
        addAndMakeVisible(label);
        label.setText("Control Bus", juce::dontSendNotification);
        addAndMakeVisible(table);
        table.setBounds(getLocalBounds());
    }

    void resized() override {
        auto bounds = getBounds();
        bounds.removeFromTop(10);
        label.setBounds(bounds.removeFromTop(20));
        bounds.removeFromTop(5);
        table.setBounds(bounds);
    }
};
