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

const juce::StringArray synthParmsToMidi( { "gate", "freq", "amp" } );

class SynthDefTable : public VTTableList {
    juce::ValueTree vtControlBuses;
    juce::Value staticSynth;

    juce::ValueTree getRowParameter(int rowNumber) {
        return vt.getChild(rowNumber);
    }

    /**
     * Check if the control should be disabled if it is a control bus or if it is a parameter that is a midi event.
     */
    bool disabledControl(juce::ValueTree &params) {
        int cbIdx = params.getProperty(IDs::pControlBus, -1);
        if ( cbIdx != -1 )
            return true;
        juce::String name = params.getProperty(IDs::pName);
        if ( synthParmsToMidi.contains(name, false) )
            if ( ! staticSynth.getValue() )
                return true;
        return false;
    }
public:
    std::function<void(int)> onControlBusAssign;

    SynthDefTable() {
        addColumn(IDs::pName, "Argument", 70);
        addColumn(IDs::pCurrentValue, "Value", 200);
        addColumn(IDs::pRange, "Range", 200);
        addColumn(IDs::pControlBus, "Control Bus", 70);
    }

    void setSynthContent(juce::ValueTree vtSynth, juce::ValueTree vtControlBuses) {
        staticSynth = vtSynth.getPropertyAsValue(IDs::staticSynth, nullptr);
        this->vtControlBuses = vtControlBuses;
        setContent(vtSynth.getChildWithName(IDs::parameters));
    }

    juce::Component* refreshComponentForCell (int rowNumber, int columnId,
                                              bool isRowSelected, juce::Component* existingComponentToUpdate) override {
        switch(columnId) {
            case 2 : {
                juce::ValueTree params = getRowParameter(rowNumber);
                if ( ! params.isValid() ) {
                    return nullptr;
                }

                auto* paramSlider = static_cast<SliderRangeAware*>(existingComponentToUpdate);
                if ( paramSlider == nullptr ) {
                    paramSlider = new SliderRangeAware();
                    paramSlider->setSliderStyle(juce::Slider::LinearBar);
                    paramSlider->setRangeProperty(params, IDs::pRange);
                }

                paramSlider->setEnabled(!disabledControl(params));
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

            case 3: {
                juce::ValueTree params = getRowParameter(rowNumber);
                if ( ! params.isValid() ) {
                    return nullptr;
                }
                auto* rangeEditor = static_cast<RangeEditor*>(existingComponentToUpdate);
                if ( rangeEditor == nullptr ) {
                    rangeEditor = new RangeEditor();
                }
                rangeEditor->assignValueTree(params, IDs::pRange);
                rangeEditor->setEnabled(!disabledControl(params));
                return rangeEditor;
            }

            case 4 : {
                juce::ValueTree params = getRowParameter(rowNumber);
                if ( ! params.isValid() ) {
                    return nullptr;
                }

                auto* cbSelector = static_cast<juce::TextButton*>(existingComponentToUpdate);
                if ( cbSelector == nullptr ) {
                    cbSelector = new juce::TextButton();
                }
                int cbIdx = params.getProperty(IDs::pControlBus, -1);
                if ( cbIdx == -1 ) {
                    cbSelector->setButtonText("Assign...");
                } else {
                    juce::ValueTree cbConf = vtControlBuses.getChild(cbIdx);
                    if ( cbConf.isValid() ) {
                        cbSelector->setButtonText(cbConf.getProperty(IDs::cbName));
                    }
                }

                if ( onControlBusAssign != nullptr ) {
                    cbSelector->onClick = [this, rowNumber]() {
                        juce::ValueTree parameter = getRowParameter(rowNumber);
                        if ( parameter.isValid() ) {
                            onControlBusAssign(rowNumber);
                        }
                    };
                }

                return cbSelector;
            }
        }
        return nullptr;
    }
};

class PanelSynthDef : public juce::Component {
    PluginColliderAudioProcessor &processor;
    juce::Label synthname;
    SynthDefTable synthDefTable;
    juce::ToggleButton staticSynth;
    std::unique_ptr<juce::FileChooser> scsynthChooser;
    juce::TextButton loaddef;
    juce::ValueTree vtSynth;
    juce::ValueTree vtControlBus;
public:
    PanelSynthDef(juce::ValueTree vt, PluginColliderAudioProcessor &processor) :  vtSynth(vt), processor(processor) {
        vtControlBus = this->processor.pluginState.getChildWithName(IDs::controlbuses);

        addAndMakeVisible(loaddef);
        loaddef.setButtonText("Load");

        addAndMakeVisible(synthname);
        synthname.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(staticSynth);
        staticSynth.setButtonText("FX mode");

        addAndMakeVisible(synthDefTable);

        staticSynth.onClick = [this] {
            if ( this->vtSynth.isValid() ) {
                this->vtSynth.setProperty(IDs::staticSynth, staticSynth.getToggleState(), nullptr);
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

                    // This is te be replaced once PluginCollider supports multiple synths; and the synth won't be
                    // loaded on the panel
                    this->vtSynth = this->processor.pluginState.getChildWithName(IDs::synths).getChildWithName(IDs::synth);
                    this->synthDefTable.setSynthContent(this->vtSynth, this->vtControlBus);
                    this->refresh();
                } else {
                    auto opts = juce::MessageBoxOptions().withTitle("Error").withMessage("Unable to read Synthdef file").withButton("OK");
                    juce::AlertWindow::showAsync(opts, [](int res) {});
                }
            });
        };

        synthDefTable.onControlBusAssign = [this](int rowNumber) {
            juce::PopupMenu menu;

            menu.addItem("Unassign", true, false, [this, rowNumber] {
                juce::ValueTree parameter = this->vtSynth.getChildWithName(IDs::parameters).getChild(rowNumber);
                parameter.setProperty(IDs::pControlBus, -1, nullptr);
                refresh();
            });
            for (int i = 0; i < vtControlBus.getNumChildren(); i++) {
                juce::ValueTree cb = vtControlBus.getChild(i);
                juce::String name = cb.getProperty(IDs::cbName);
                int idx = cb.getProperty(IDs::cbIdx);
                menu.addItem(name, true, false, [this, idx, rowNumber, name] {
                    juce::ValueTree parameter = this->vtSynth.getChildWithName(IDs::parameters).getChild(rowNumber);
                    juce::String msg = juce::String("Assign parameters value '") + parameter.getProperty(IDs::pName).toString()
                        + "' to control bus '" + name + "' ?";
                    auto msgbox = juce::MessageBoxOptions::makeOptionsYesNoCancel(
                        juce::MessageBoxIconType::QuestionIcon, "Confirmation", msg);
                    juce::NativeMessageBox::showAsync(msgbox, [this, idx, name, rowNumber](int result) {
                        if ( result == 2 )
                            return;

                        // We copy the value of the parameter to the control bus
                        juce::ValueTree parameter = this->vtSynth.getChildWithName(IDs::parameters).getChild(rowNumber);
                        if ( result == 0 ) {
                            juce::ValueTree cbVt = this->vtControlBus.getChild(idx);
                            cbVt.setProperty(IDs::cbName, parameter.getProperty(IDs::pName), nullptr);
                            cbVt.setProperty(IDs::cbRange, parameter.getProperty(IDs::pRange), nullptr);
                        }
                        parameter.setProperty(IDs::pControlBus, idx, nullptr);
                        refresh();
                     });
                });
            }
            menu.showMenuAsync(juce::PopupMenu::Options());
        };

        synthDefTable.setSynthContent(vtSynth, vtControlBus);
        refresh();
    }

    void refresh() {
        juce::String synthName = vtSynth.getProperty(IDs::synthName);
        if ( synthName == "" )
            synthName = "No synthDef loaded";
        synthDefTable.refresh();
        synthname.setText(juce::String("Synth: ") + synthName, juce::NotificationType::dontSendNotification);
        staticSynth.setToggleState(vtSynth.getProperty(IDs::staticSynth), juce::NotificationType::dontSendNotification);
    }

    void resized() override {
        auto bounds = getBounds();

        synthname.setBounds(200, 5, bounds.getWidth() - 200, 25);
        loaddef.setBounds(0, 5, 50, 25);
        staticSynth.setBounds(60, 5, 200, 25);
        synthDefTable.setBounds(0, 40, bounds.getWidth(), bounds.getHeight() - 40);
    }
};
