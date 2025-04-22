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

const juce::StringArray synthParmsToMidi( { "gate", "freq", "amp" } );


class ParameterTable : public juce::Component, public juce::TableListBoxModel {
    juce::ValueTree &vt;
public:
    ParameterTable(juce::ValueTree &vt) : vt(vt) {
        addAndMakeVisible (table);
        table.setModel(this);

        int flags = juce::TableHeaderComponent::ColumnPropertyFlags::visible;

        table.getHeader().addColumn("Argument", 1, 100, 100, 100, flags);
        table.getHeader().addColumn("Value", 2, 350, 350, 350, flags);
        table.getHeader().addColumn("Low", 3, 70, 70, 70, flags);
        table.getHeader().addColumn("High", 4, 70, 70, 70, flags);
        table.getHeader().addColumn("Control Bus", 5, 70, 70, 70, flags);
        table.getHeader().setStretchToFitActive(true);
        //table.setRowHeight(40);
    }

    int getNumColumns() {
        return 5;
    }

    int getNumRows() {
        juce::ValueTree params = vt.getChildWithName(IDs::synths).getChild(0).getChildWithName(IDs::parameters);
        if ( ! params.isValid() )
            return 0;
        return params.getNumChildren();
    }

    void paintRowBackground(juce::Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected) override {
        //if (rowIsSelected)
        //    g.fillAll (juce::Colours::lightblue);
        /*else if (rowNumber % 2)
            g.fillAll (juce::Colours::blue);*/
    }

    void paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override {
        g.setColour(getLookAndFeel().findColour (juce::ListBox::textColourId));
        g.setFont(font);

        juce::ValueTree parameter = getRowParameter(rowNumber);

        if ( parameter.isValid() ) {
            juce::Identifier targetId;
            switch (columnId) {
                case 1:
                    targetId = IDs::pName;
                    break;
                case 5:
                    targetId = IDs::pControlBus;
                    break;
            }
            g.drawText(parameter.getProperty(targetId), 2, 0, width - 4, height, juce::Justification::centredLeft, true);
        }
        g.setColour(getLookAndFeel().findColour (juce::ListBox::backgroundColourId));
        g.fillRect(width - 1, 0, 1, height);
    }

    juce::Component* refreshComponentForCell (int rowNumber, int columnId,
                                              bool isRowSelected, juce::Component* existingComponentToUpdate) override {
        switch(columnId) {
            case 2 : {
                juce::ValueTree params = getRowParameter(rowNumber);
                if ( ! params.isValid() ) {
                    return nullptr;
                }

                auto* paramSlider = static_cast<ParamSlider*>(existingComponentToUpdate);
                if ( paramSlider == nullptr ) {
                    paramSlider = new ParamSlider();
                }

                paramSlider->setEnabled(!disabledControl(params.getProperty(IDs::pName)));
                paramSlider->setRange(params.getProperty(IDs::pRangeLow), params.getProperty(IDs::pRangeHigh));
                if ( params.hasProperty(IDs::pCurrentValue) ) {
                    paramSlider->setValue(params.getProperty(IDs::pCurrentValue), juce::NotificationType::dontSendNotification);
                } else {
                    paramSlider->setValue(params.getProperty(IDs::pDefaultValue), juce::NotificationType::dontSendNotification);
                }
                paramSlider->onValueChange = [this, rowNumber, paramSlider]() {
                    juce::ValueTree parameter = getRowParameter(rowNumber);
                    parameter.setProperty(IDs::pCurrentValue, paramSlider->getValue(), nullptr);
                };

                return paramSlider;
            }

            case 3:
            case 4: {
                juce::ValueTree params = getRowParameter(rowNumber);
                if ( ! params.isValid() ) {
                    return nullptr;
                }

                auto* textEditor = static_cast<EditableTextCustomComponent*>(existingComponentToUpdate);
                if ( textEditor == nullptr ) {
                    textEditor = new EditableTextCustomComponent(*this);
                }
                textEditor->setRowAndColumn(rowNumber, columnId);

                return textEditor;
            }

            case 5: {
                juce::ValueTree params = getRowParameter(rowNumber);
                if ( ! params.isValid() ) {
                    return nullptr;
                }

                auto* comboBox = static_cast<juce::ComboBox*>(existingComponentToUpdate);
                if ( comboBox == nullptr ) {
                    comboBox = new juce::ComboBox();
                    comboBox->addItemList({"None", "This", "is", "not", "yet", "functional", "Control Bus 6", "Control Bus 7", "Control Bus 8"}, 1);
                }
                return comboBox;
            }
        }
        return nullptr;
    }

    void resized() override {
        table.setBounds(getLocalBounds());
    }

    void refresh() {
        table.updateContent();
    }

    juce::String getText(const int columnNumber, const int rowNumber) {
        juce::ValueTree vt = getRowParameter(rowNumber);
        if ( vt.isValid() ) {
            switch (columnNumber) {
                case 3:
                    return vt.getProperty(IDs::pRangeLow);
                case 4:
                    return vt.getProperty(IDs::pRangeHigh);
            }
        }
    }

    void setText(const int columnNumber, const int rowNumber, const juce::String& newText) {
        juce::ValueTree vt = getRowParameter(rowNumber);

        int value = newText.getIntValue();

        if ( vt.isValid() ) {
            switch (columnNumber) {
                case 3:
                    vt.setProperty(IDs::pRangeLow, value, nullptr);
                    refresh();
                    break;
                case 4:
                    vt.setProperty(IDs::pRangeHigh, value, nullptr);
                    refresh();
                    break;
            }
        }
    }

private:
    juce::TableListBox table;
    juce::Font font { juce::FontOptions { 14.0f } };

    juce::ValueTree getRowParameter(int rowNumber) {
        return vt.getChildWithName(IDs::synths).getChild(0).getChildWithName(IDs::parameters).getChild(rowNumber);
    }

    bool disabledControl(juce::String name) {
        if ( synthParmsToMidi.contains(name, false) ) {
            if ( ! vt.getChildWithName(IDs::synths).getChild(0).getProperty(IDs::staticSynth) )
                return true;
        }
        return false;
    }

    class ParamSlider : public juce::Slider {
    public:
        ParamSlider() {
            setSliderStyle(juce::Slider::LinearBar);
        }
    };

    //==============================================================================
    // This is a custom Label component, which we use for the table's editable text columns.
    class EditableTextCustomComponent final : public juce::Label  {
    public:
        EditableTextCustomComponent (ParameterTable& td)  : owner (td)  {
            // double click to edit the label text; single click handled below
            setEditable (false, true, false);
        }

        void mouseDown (const juce::MouseEvent& event) override {
            // single click on the label should simply select the row
            owner.table.selectRowsBasedOnModifierKeys(row, event.mods, false);
            juce::Label::mouseDown(event);
        }

        void textWasEdited() override {
            owner.setText(columnId, row, getText());
        }

        // Our demo code will call this when we may need to update our contents
        void setRowAndColumn(const int newRow, const int newColumn) {
            row = newRow;
            columnId = newColumn;
            setText(owner.getText(columnId, row), juce::NotificationType::dontSendNotification);
        }

        void paint (juce::Graphics& g) override {
            auto& lf = getLookAndFeel();
            if (! dynamic_cast<juce::LookAndFeel_V4*> (&lf))
                lf.setColour (textColourId, juce::Colours::black);
            Label::paint (g);
        }

    private:
        ParameterTable& owner;
        int row, columnId;
        juce::Colour textColour;
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterTable)
};

class SynthDefPanel : public juce::Component {
    juce::ValueTree &vt;
    juce::Label synthname;
    ParameterTable parmModel;
    juce::ToggleButton staticSynth;
public:
    juce::TextButton loaddef;

    SynthDefPanel(juce::ValueTree &vt) : vt(vt), parmModel(vt) {
        addAndMakeVisible(loaddef);
        loaddef.setButtonText("Load");

        addAndMakeVisible(synthname);
        synthname.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(staticSynth);
        staticSynth.setButtonText("FX mode");

        addAndMakeVisible(parmModel);

        staticSynth.onClick = [this] {
            juce::ValueTree synth = this->vt.getChildWithName(IDs::synths).getChildWithName(IDs::synth);
            if ( synth.isValid() ) {
                synth.setProperty(IDs::staticSynth, staticSynth.getToggleState(), nullptr);
                refresh();
            }
        };

        refresh();
    }

    void refresh() {
        juce::ValueTree synth = vt.getChildWithName(IDs::synths).getChildWithName(IDs::synth);
        juce::String synthName = synth.getProperty(IDs::synthName);
        if ( synthName == "" )
            synthName = "No synthDef loaded";
        parmModel.refresh();
        synthname.setText(juce::String("Synth: ") + synthName, juce::NotificationType::dontSendNotification);
        staticSynth.setToggleState(synth.getProperty(IDs::staticSynth), juce::NotificationType::dontSendNotification);
    }

    void resized() override {
        auto bounds = getBounds();

        synthname.setBounds(200, 5, bounds.getWidth() - 200, 25);
        loaddef.setBounds(0, 5, 50, 25);
        staticSynth.setBounds(60, 5, 200, 25);
        parmModel.setBounds(0, 40, bounds.getWidth(), bounds.getHeight() - 40);
    }
};
