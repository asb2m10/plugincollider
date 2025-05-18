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

#include "CommonPanelView.h"

const juce::StringArray synthParmsToMidi( { "gate", "freq", "amp" } );

class SynthDefTable : public VTTableList {

    juce::ValueTree getRowParameter(int rowNumber) {
        return vt.getChild(rowNumber);
    }

    bool disabledControl(juce::String name) {
        if ( synthParmsToMidi.contains(name, false) ) {
            if ( ! rootTree.getProperty(IDs::staticSynth) )
                return true;
        }
        return false;
    }

public:
    juce::ValueTree rootTree;

    SynthDefTable() {
        addColumn(IDs::pName, "Argument", 70);
        addColumn(IDs::pCurrentValue, "Value", 200);
        addColumn(IDs::pLow, "Low", 70);
        addColumn(IDs::pHigh, "High", 70);
        addColumn(IDs::pControlBus, "Control Bus", 70);
    }

    void setContent(juce::ValueTree vt) override {
        rootTree = vt;
        VTTableList::setContent(vt.getChildWithName(IDs::parameters));
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
                paramSlider->setRange(params.getProperty(IDs::pLow), params.getProperty(IDs::pHigh));
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
};

class SynthDefPanel : public juce::Component {
    PluginColliderAudioProcessor &processor;
    juce::Label synthname;
    //ParameterTable parmModel;
    SynthDefTable table;
    juce::ToggleButton staticSynth;
    std::unique_ptr<juce::FileChooser> scsynthChooser;
    juce::TextButton loaddef;
    juce::ValueTree vt;
public:
    SynthDefPanel(juce::ValueTree vt, PluginColliderAudioProcessor &processor) :  vt(vt), processor(processor) {
        addAndMakeVisible(loaddef);
        loaddef.setButtonText("Load");

        addAndMakeVisible(synthname);
        synthname.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(staticSynth);
        staticSynth.setButtonText("FX mode");

        addAndMakeVisible(table);

        staticSynth.onClick = [this] {
            if ( table.rootTree.isValid() ) {
                table.rootTree.setProperty(IDs::staticSynth, staticSynth.getToggleState(), nullptr);
                refresh();
            }
        };

        loaddef.onClick = [this] () {
            scsynthChooser = std::make_unique<juce::FileChooser> ("Please select the moose you want to load...",
                                                juce::File(), "*.scsyndef");
            auto folderChooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
            scsynthChooser->launchAsync (folderChooserFlags, [this] (const juce::FileChooser& chooser) {
                juce::File scfile(chooser.getResult());
                if ( !scfile.exists() )
                    return;

                std::unique_ptr<SynthDef> def;
                def.reset(SynthDef::fromFile(scfile));

                if ( def != nullptr ) {
                    if ( !this->processor.loadSynthDef(def.get()) ) {
                        auto opts = juce::MessageBoxOptions().withTitle("Error").withMessage("SuperCollider refused to load the SynthDef").withButton("OK");
                        juce::AlertWindow::showAsync(opts, [](int res) {});
                        return;
                    }
                    this->table.setContent(this->vt.getChildWithName(IDs::synth));
                    this->refresh();
                } else {
                    auto opts = juce::MessageBoxOptions().withTitle("Error").withMessage("Unable to read Synthdef file").withButton("OK");
                    juce::AlertWindow::showAsync(opts, [](int res) {});
                }
            });
        };

        table.setContent(vt.getChildWithName(IDs::synth));
        refresh();
    }

    void refresh() {
        juce::String synthName = table.rootTree.getProperty(IDs::synthName);
        if ( synthName == "" )
            synthName = "No synthDef loaded";
        table.refresh();
        synthname.setText(juce::String("Synth: ") + synthName, juce::NotificationType::dontSendNotification);
        staticSynth.setToggleState(table.rootTree.getProperty(IDs::staticSynth), juce::NotificationType::dontSendNotification);
    }

    void resized() override {
        auto bounds = getBounds();

        synthname.setBounds(200, 5, bounds.getWidth() - 200, 25);
        loaddef.setBounds(0, 5, 50, 25);
        staticSynth.setBounds(60, 5, 200, 25);
        table.setBounds(0, 40, bounds.getWidth(), bounds.getHeight() - 40);
    }
};

class ControlBusTable : public VTTableList {
public:
    ControlBusTable(juce::ValueTree vt) {
        addColumn(IDs::cbName, "Name", 200);
        addColumn(IDs::cbLow, "Low", 70);
        addColumn(IDs::cbHigh, "High", 70);
        addColumn(IDs::cbStep, "Step", 70);

        setContent(vt);
    }

    juce::Component* refreshComponentForCell (int rowNumber, int columnId,
                                            bool isRowSelected, juce::Component* existingComponentToUpdate) override {
        juce::Identifier targetId = columnIds[columnId - 1];
        auto* textEditor = static_cast<EditableTextCustomComponent*>(existingComponentToUpdate);
        if ( textEditor == nullptr ) {
            textEditor = new EditableTextCustomComponent(*this);
        }
        textEditor->setRowAndColumn(rowNumber, columnId);
        return textEditor;
    }
};

class ControlBusPanel : public juce::Component {
    juce::ValueTree &vt;
    juce::Label label;
    ControlBusTable table;
public:
    ControlBusPanel(juce::ValueTree vt) : vt(vt), table(vt) {
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