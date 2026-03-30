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

static std::map<juce::String, juce::String> loadSpecFile(const juce::File &scsyndef) {
    std::map<juce::String, juce::String> specs;

    // Try .txarcmeta (SC's metadata archive format)
    auto metaFile = scsyndef.withFileExtension("txarcmeta");
    if (!metaFile.existsAsFile()) {
        // Fall back to .spec (simple text format)
        auto specFile = scsyndef.withFileExtension("spec");
        if (!specFile.existsAsFile())
            return specs;
        auto lines = juce::StringArray::fromLines(specFile.loadFileAsString());
        for (auto &line : lines) {
            line = line.trim();
            if (line.isEmpty() || line.startsWith("#"))
                continue;
            auto tokens = juce::StringArray::fromTokens(line, " \t", "");
            if (tokens.size() >= 4)
                specs[tokens[0]] = tokens[1] + " " + tokens[2] + " " + tokens[3];
        }
        return specs;
    }

    // Parse .txarcmeta: extract ControlSpec minval/maxval/step keyed by param name
    auto content = metaFile.loadFileAsString();

    // 1. Find param name -> object index mapping from the array line like:
    //    'stutter',  o[4],  'div',  o[7],  ...  'ffreq',  o[9],
    std::map<int, juce::String> indexToName;
    int searchPos = 0;
    while (true) {
        int quoteStart = content.indexOf(searchPos, "'");
        if (quoteStart < 0) break;
        int quoteEnd = content.indexOf(quoteStart + 1, "'");
        if (quoteEnd < 0) break;
        auto paramName = content.substring(quoteStart + 1, quoteEnd);
        // Look for o[N] after the param name
        int oRef = content.indexOf(quoteEnd, "o[");
        if (oRef < 0) break;
        // Make sure we don't jump past the next param name
        int nextQuote = content.indexOf(quoteEnd + 1, "'");
        if (nextQuote >= 0 && oRef > nextQuote) {
            searchPos = quoteEnd + 1;
            continue;
        }
        int oBracketEnd = content.indexOf(oRef, "]");
        if (oBracketEnd < 0) break;
        int objIdx = content.substring(oRef + 2, oBracketEnd).getIntValue();
        if (paramName != "specs" && paramName != "spec")
            indexToName[objIdx] = paramName;
        searchPos = oBracketEnd + 1;
    }

    // 2. Extract ControlSpec entries: "N, [ minval: X, maxval: Y, ... step: Z, ..."
    searchPos = 0;
    while (true) {
        int csPos = content.indexOf(searchPos, "// ControlSpec");
        if (csPos < 0) break;
        // Find the object index on the next line: "N, ["
        int lineStart = content.indexOf(csPos, "\n") + 1;
        auto idxToken = content.substring(lineStart, content.indexOf(lineStart, ",")).trim();
        int objIdx = idxToken.getIntValue();

        // Extract minval, maxval, step
        int blockStart = content.indexOf(lineStart, "[");
        if (blockStart < 0) break;
        // Find matching ] — skip nested o[N] references
        int blockEnd = blockStart + 1;
        int depth = 1;
        while (blockEnd < content.length() && depth > 0) {
            if (content[blockEnd] == '[') depth++;
            else if (content[blockEnd] == ']') depth--;
            blockEnd++;
        }
        if (depth != 0) break;
        blockEnd--; // point at the ]
        auto block = content.substring(blockStart, blockEnd + 1);

        auto extractValue = [&block](const juce::String &key) -> juce::String {
            int pos = block.indexOf(key + ":");
            if (pos < 0) return "";
            int valStart = pos + key.length() + 1;
            int valEnd = block.indexOf(valStart, ",");
            if (valEnd < 0) valEnd = block.indexOf(valStart, "]");
            return block.substring(valStart, valEnd).trim();
        };

        auto minval = extractValue("minval");
        auto maxval = extractValue("maxval");
        auto step = extractValue("step");

        if (minval.isNotEmpty() && maxval.isNotEmpty() && step.isNotEmpty()) {
            auto nameIt = indexToName.find(objIdx);
            if (nameIt != indexToName.end()) {
                specs[nameIt->second] = minval + " " + maxval + " " + step;
            }
        }
        searchPos = blockEnd + 1;
    }

    return specs;
}

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

                auto specs = loadSpecFile(scfile);
                if (!this->processor.replaceSynthDef(content, vtSynth, specs)) {
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

                    // Assign directly — copy param name/range to the control bus
                    juce::ValueTree cbVt = this->vtControlBus.getChild(idx);
                    cbVt.setProperty(IDs::cbName, parameter.getProperty(IDs::pName), nullptr);
                    cbVt.setProperty(IDs::cbRange, parameter.getProperty(IDs::pRange), nullptr);

                    float value;
                    if ( parameter.hasProperty(IDs::pCurrentValue) )
                        value = parameter.getProperty(IDs::pCurrentValue);
                    else
                        value = parameter.getProperty(IDs::pDefaultValue);

                    this->processor.setControlBusValue(idx, value);
                    parameter.setProperty(IDs::pControlBus, idx, nullptr);
                    refresh();
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

