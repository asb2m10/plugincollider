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
#include "PluginModel.h"

const juce::StringArray synthParmsToMidi( { "gate", "freq", "amp" } );

class SynthDefTable : public VTTableList {
    juce::StringArray filterParameter;
    juce::ValueTree vtControlBuses;

    juce::ValueTree getRowParameter(int rowNumber) {
        return vt.getChild(rowNumber);
    }

    /**
     * Check if the control should be disabled if it is a control bus or if it is a parameter that is a midi event.
     */
    bool disabledControl(juce::ValueTree &param) {
        int cbIdx = param.getProperty(IDs::pControlBus, -1);
        if ( cbIdx != -1 )
            return true;
        juce::String name = param.getProperty(IDs::pName);
        if ( filterParameter.contains(name, false) )
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

    void setParametersToFilter(const juce::StringArray names) {
        filterParameter = names;
    }

    void setSynthContent(juce::ValueTree vtSynth, juce::ValueTree vtControlBuses) {
        this->vtControlBuses = vtControlBuses;
        setContent(vtSynth.getChildWithName(IDs::parameters));
    }

    juce::Component* refreshComponentForCell(int rowNumber, int columnId,
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

class PanelSynthDefFx : public juce::Component {
protected:
    PluginColliderAudioProcessor &processor;
    juce::Label synthname;
    SynthDefTable synthDefTable;
    std::unique_ptr<juce::FileChooser> scsynthChooser;
    juce::TextButton loaddef;
    juce::ValueTree vtSynth;
    juce::ValueTree vtControlBus;
public:
    PanelSynthDefFx(juce::ValueTree vt, PluginColliderAudioProcessor &processor) :  vtSynth(vt), processor(processor) {
        vtControlBus = this->processor.pluginState.getChildWithName(IDs::controlbuses);

        addAndMakeVisible(loaddef);
        loaddef.setButtonText("Load");

        addAndMakeVisible(synthname);
        synthname.setJustificationType(juce::Justification::centredRight);

        addAndMakeVisible(synthDefTable);

        loaddef.onClick = [this] () {
            scsynthChooser = std::make_unique<juce::FileChooser> ("Please select the SynthDef you want to load...",
                                                juce::File(), "*.scsyndef;*.scd");
            auto folderChooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
            scsynthChooser->launchAsync (folderChooserFlags, [this] (const juce::FileChooser& chooser) {
                juce::File scfile(chooser.getResult());
                if ( !scfile.exists() )
                    return;

                juce::MemoryBlock content;
                if (!scfile.loadFileAsData(content))
                    return;

                if (!this->processor.replaceSynthDef(content, vtSynth)) {
                    auto opts = juce::MessageBoxOptions().withTitle("Error").withMessage("SuperCollider refused to load the SynthDef").withButton("OK");
                    juce::AlertWindow::showAsync(opts, [](int res) {});
                    return;
                }

                synthDefTable.setSynthContent(vtSynth, vtControlBus);
                refresh();
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
                juce::String name = juce::String(i+1) + ": " + cb.getProperty(IDs::cbName).toString();
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
    }

    void resized() override {
        auto bounds = getLocalBounds();
        bounds.removeFromTop(8);
        auto top = bounds.removeFromTop(25);
        bounds.removeFromTop(8);
        loaddef.setBounds(top.removeFromLeft(50));
        top.removeFromLeft(10);
        synthname.setBounds(top.removeFromRight(200));
        synthDefTable.setBounds(bounds);
    }
};

class PanelSynthDefMidi : public PanelSynthDefFx {
    juce::Label labelRange;
    juce::Label labelDash;
    juce::TextEditor lowNote;
    juce::TextEditor highNote;
    juce::ToggleButton mono;
public:    
    PanelSynthDefMidi(juce::ValueTree vt, PluginColliderAudioProcessor &processor) : PanelSynthDefFx(vt, processor) {
        synthDefTable.setParametersToFilter(synthParmsToMidi);

        addAndMakeVisible(labelRange);
        addAndMakeVisible(labelDash);
        addAndMakeVisible(lowNote);
        addAndMakeVisible(highNote);
        //addAndMakeVisible(mono);

        labelRange.setText("Note Range", juce::dontSendNotification);
        labelDash.setText(" - ", juce::dontSendNotification);
        mono.setButtonText("Monophonic");
        lowNote.setInputFilter(new juce::TextEditor::LengthAndCharacterRestriction(3, "0123456789"), true);
        highNote.setInputFilter(new juce::TextEditor::LengthAndCharacterRestriction(3, "0123456789"), true);

        juce::String range = vtSynth[IDs::synthNoteRange];
        juce::StringArray token;
        token.addTokens(range, " ");

        lowNote.setText(token[0], juce::dontSendNotification);
        highNote.setText(token[1], juce::dontSendNotification);
        bool isMonoSynth= vtSynth[IDs::synthMono];
        mono.setToggleState(isMonoSynth, juce::dontSendNotification);

        lowNote.onTextChange = [this]() {
            int low = lowNote.getText().getIntValue();
            juce::String range = vtSynth[IDs::synthNoteRange];
            juce::StringArray token;
            token.addTokens(range, " ");
            int currentHigh = token[1].getIntValue();
            if ( low >= currentHigh ) {
                low = currentHigh - 1;
            }
            if ( low < 0 ) {
                low = 0;
            }
            lowNote.setText(juce::String(low), juce::dontSendNotification);
            vtSynth.setProperty(IDs::synthNoteRange, juce::String(low) + " " + token[1], nullptr);
        };

        highNote.onTextChange = [this]() {
            int high = highNote.getText().getIntValue();
            juce::String range = vtSynth[IDs::synthNoteRange];
            juce::StringArray token;
            token.addTokens(range, " ");
            int currentLow = token[0].getIntValue();
            if ( high <= currentLow ) {
                high = currentLow + 1;
            }
            if ( high > 127 ) {
                high = 127;
            }
            highNote.setText(juce::String(high), juce::dontSendNotification);
            vtSynth.setProperty(IDs::synthNoteRange, token[0] + " " + juce::String(high), nullptr);
        };

        mono.onStateChange = [this]() {
            vtSynth.setProperty(IDs::synthMono, mono.getToggleState(), nullptr);
        };
    }

    void resized() override {
        auto bounds = getLocalBounds();
        bounds.removeFromTop(8);
        auto top = bounds.removeFromTop(25);
        bounds.removeFromTop(8);
        loaddef.setBounds(top.removeFromLeft(50));
        top.removeFromLeft(2);
        labelRange.setBounds(top.removeFromLeft(80));
        lowNote.setBounds(top.removeFromLeft(30));
        labelDash.setBounds(top.removeFromLeft(15));
        highNote.setBounds(top.removeFromLeft(30));
        top.removeFromLeft(10);
        mono.setBounds(top.removeFromLeft(130));
        synthname.setBounds(top.removeFromRight(200));
        synthDefTable.setBounds(bounds);
    }    
};

