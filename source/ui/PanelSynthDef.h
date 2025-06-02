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
    juce::ValueTree getRowParameter(int rowNumber) {
        return vt.getChild(rowNumber);
    }

    bool disabledControl(juce::String name) {
        if ( synthParmsToMidi.contains(name, false) ) {
            if ( ! vt.getProperty(IDs::staticSynth) )
                return true;
        }
        return false;
    }
public:
    juce::ValueTree vtControlBuses;

    SynthDefTable() {
        addColumn(IDs::pName, "Argument", 70);
        addColumn(IDs::pCurrentValue, "Value", 200);
        addColumn(IDs::pRange, "Low", 200);
        addColumn(IDs::pControlBus, "Control Bus", 70);
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

                paramSlider->setEnabled(!disabledControl(params.getProperty(IDs::pName)));
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
                if ( static_cast<int>(params.getProperty(IDs::pControlBus)) != -1 ) {
                    rangeEditor->setEnabled(true);
                } else {
                    rangeEditor->setEnabled(false);
                }

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
                return cbSelector;
            }
        }
        return nullptr;
    }
};

class PanelSynthDef : public juce::Component {
    PluginColliderAudioProcessor &processor;
    juce::Label synthname;
    SynthDefTable table;
    juce::ToggleButton staticSynth;
    std::unique_ptr<juce::FileChooser> scsynthChooser;
    juce::TextButton loaddef;
    juce::ValueTree vt;
public:
    PanelSynthDef(juce::ValueTree vt, PluginColliderAudioProcessor &processor) :  vt(vt), processor(processor) {
        addAndMakeVisible(loaddef);
        loaddef.setButtonText("Load");

        addAndMakeVisible(synthname);
        synthname.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(staticSynth);
        staticSynth.setButtonText("FX mode");

        addAndMakeVisible(table);

        staticSynth.onClick = [this] {
            if ( this->vt.isValid() ) {
                this->vt.setProperty(IDs::staticSynth, staticSynth.getToggleState(), nullptr);
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
                    this->vt = this->processor.pluginState.getChildWithName(IDs::synths).getChildWithName(IDs::synth);

                    this->table.setContent(this->vt.getChildWithName(IDs::parameters));
                    this->refresh();
                } else {
                    auto opts = juce::MessageBoxOptions().withTitle("Error").withMessage("Unable to read Synthdef file").withButton("OK");
                    juce::AlertWindow::showAsync(opts, [](int res) {});
                }
            });
        };

        table.setContent(vt.getChildWithName(IDs::parameters));
        refresh();
    }

    void refresh() {
        juce::String synthName = vt.getProperty(IDs::synthName);
        if ( synthName == "" )
            synthName = "No synthDef loaded";
        table.refresh();
        synthname.setText(juce::String("Synth: ") + synthName, juce::NotificationType::dontSendNotification);
        staticSynth.setToggleState(vt.getProperty(IDs::staticSynth), juce::NotificationType::dontSendNotification);
    }

    void resized() override {
        auto bounds = getBounds();

        synthname.setBounds(200, 5, bounds.getWidth() - 200, 25);
        loaddef.setBounds(0, 5, 50, 25);
        staticSynth.setBounds(60, 5, 200, 25);
        table.setBounds(0, 40, bounds.getWidth(), bounds.getHeight() - 40);
    }
};
