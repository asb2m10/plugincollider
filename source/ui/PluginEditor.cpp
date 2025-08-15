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

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginEditor.h"
#include "ext/value_tree_debugger.h"
#include "TreeViewItems.h"
#include "SC_Version.hpp"

class MidiKeyboardWindow: public juce::DocumentWindow {
public:
    MidiKeyboardWindow(juce::MidiKeyboardState &state) : juce::DocumentWindow("Midi Keyboard", juce::Colours::lightgrey, juce::DocumentWindow::allButtons) {
        auto keyboardComponent = new juce::MidiKeyboardComponent(state, juce::MidiKeyboardComponent::horizontalKeyboard);
        keyboardComponent->setSize(600, 30);
        setContentOwned(keyboardComponent, true);
        setUsingNativeTitleBar(true);
        setResizable(true, true);
        setVisible(true);
    }

    void closeButtonPressed() override {
        setVisible(false);
    }
};

//==============================================================================
PluginColliderAudioProcessorEditor::PluginColliderAudioProcessorEditor(
    PluginColliderAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p),
      logViewer(&(p.logger.content)), layoutResizer (&layout, 1, true) {

    menuBar.reset(new juce::MenuBarComponent(this));
    addAndMakeVisible(menuBar.get());

    addAndMakeVisible(treeView);
    addAndMakeVisible(udpPort);
    addAndMakeVisible(setUdpPortButton);

    udpPort.setInputFilter(new juce::TextEditor::LengthAndCharacterRestriction(5, "0123456789"), true);
    udpPort.setText(juce::String(p.udpPort.getListenPort()), true);
    setUdpPortButton.setButtonText("Set UDP Port");
    setUdpPortButton.onClick = [this] () {
        juce::String port = udpPort.getText();
        audioProcessor.setUdpPort(port);
        udpPort.setText(juce::String(audioProcessor.udpPort.getListenPort()), true);
    };

    configButton.setButtonText("Configure");
    //addAndMakeVisible(configButton);
    // configButton.setBounds(10, 38, 130, 25);

    // configButton.onClick = [ this ] {
    //     settingsWindow = new juce::AlertWindow("Plugincollider settings", "", juce::AlertWindow::NoIcon);

    //     settingsWindow->addTextBlock("Plugin path");
    //     settingsWindow->addTextEditor("pluginPath", audioProcessor.pluginPath);
    //     settingsWindow->addTextBlock("Scsynth path");
    //     settingsWindow->addTextEditor("synthPath", audioProcessor.synthPath);
    //     settingsWindow->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey, 0, 0));
    //     settingsWindow->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey, 0, 0));

    //     settingsWindow->enterModalState(true, juce::ModalCallbackFunction::create([this](int r) {
    //         if (r) {
    //             scprintf("RESTART PLUGIN FOR SETTINGS TO TAKE EFFECT\n");
    //             juce::PropertiesFile *prop = audioProcessor.appProp.getUserSettings();
    //             prop->setValue("pluginPath", this->settingsWindow->getTextEditorContents("pluginPath"));
    //             prop->setValue("synthPath", this->settingsWindow->getTextEditorContents("synthPath"));
    //             audioProcessor.appProp.saveIfNeeded();
    //         }
    //     }), true);

    // };

    // synthDefPanel.loaddef.onClick = [this] () {
    //     scsynthChooser = std::make_unique<juce::FileChooser> ("Please select the moose you want to load...",
    //                                         juce::File(), "*.scsyndef");
    //     auto folderChooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
    //     scsynthChooser->launchAsync (folderChooserFlags, [this] (const juce::FileChooser& chooser) {
    //         juce::File scfile (chooser.getResult());
    //         if ( !scfile.exists() )
    //             return;

    //         std::unique_ptr<SynthDef> def;
    //         def.reset(SynthDef::fromFile(scfile));

    //         if ( def != nullptr ) {
    //             if ( !audioProcessor.loadSynthDef(def.get()) ) {
    //                 auto opts = juce::MessageBoxOptions().withTitle("Error").withMessage("SuperCollider refused to load the SynthDef").withButton("OK");
    //                 juce::AlertWindow::showAsync(opts, [](int res) {});
    //                 return;
    //             }
    //             synthDefPanel.refresh();
    //         } else {
    //             auto opts = juce::MessageBoxOptions().withTitle("Error").withMessage("Unable to read Synthdef file").withButton("OK");
    //             juce::AlertWindow::showAsync(opts, [](int res) {});
    //         }
    //     });
    // };

    // For now this is for debugging
    //addAndMakeVisible(cb1);
    cb1.setSliderStyle(juce::Slider::Rotary);
    cb1.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, true, 0, 0);
    cb1.setBounds(150, 10, 50, 50);
    juce::AudioParameterFloat *parameter = p.controlBus[0];
    cb1Attachment.reset(new juce::SliderParameterAttachment(*parameter, cb1, nullptr));

    addAndMakeVisible(treeView);
    treeView.setRootItemVisible(false);
    treeView.setRootItem(new RootItem(audioProcessor, dynamicViewPanel));
    addAndMakeVisible(rightPane);
    rightPane.addAndMakeVisible(logViewer);
    rightPane.addAndMakeVisible(dynamicViewPanel);
    addAndMakeVisible(layoutResizer);
    addAndMakeVisible(stats);
    stats.setJustificationType(juce::Justification::centredRight);

    layout.setItemLayout(0, -0.05, -0.9, -0.15);
    layout.setItemLayout(1, 5, 5, 5);
    layout.setItemLayout(2, -0.1, -0.9, -0.7);

    startTimer(400);
    setResizable(true, true);
    setSize(800, 550);
}

PluginColliderAudioProcessorEditor::~PluginColliderAudioProcessorEditor() {
    treeView.deleteRootItem();
    stopTimer();
}

void PluginColliderAudioProcessorEditor::timerCallback() {
    if (logLines != audioProcessor.logger.content.size()) {
        logLines = audioProcessor.logger.content.size();
        logViewer.setText(audioProcessor.logger.content.joinIntoString(""));
        logViewer.moveCaretToEnd();
    }

    audioProcessor.superCollider.getWorldStats(&worldStats);
    juce::AudioProcessLoadMeasurer *load = audioProcessor.getLoadMeasurer();
    stats.setText(juce::String::formatted(
                      "units: %i graph: %i groups: %i cpu: %0.3f xrun: %d", worldStats.mNumUnits,
                      worldStats.mNumGraphs, worldStats.mNumGroups, load->getLoadAsPercentage(), load->getXRunCount()),
                  juce::dontSendNotification);

    if ( audioProcessor.isAudioProcSuspended() ) {
        if ( isEnabled() )
            setEnabled(false);
    } else {
        if ( !isEnabled() ) {
            setEnabled(true);
        }
    }
}

juce::PopupMenu PluginColliderAudioProcessorEditor::getMenuForIndex(int topLevelMenuIndex, const juce::String& str) {
    juce::PopupMenu ret;
    switch(topLevelMenuIndex) {
    case 0 : {
        juce::PopupMenu udpLogging;
        juce::PopupMenu serverLogging;
        juce::PopupMenu logging;

        int udpLogLevel = audioProcessor.superCollider.getOSCDumpLevel();
        udpLogging.addItem("Off", true, udpLogLevel == 0,
            [this] { audioProcessor.superCollider.setOSCDumpLevel(0); });
        udpLogging.addItem("Level 1", true, udpLogLevel == 1,
            [this] { audioProcessor.superCollider.setOSCDumpLevel(1); });
        udpLogging.addItem("Level 2", true, udpLogLevel == 2,
            [this] { audioProcessor.superCollider.setOSCDumpLevel(2); });

        int verboseLevel = audioProcessor.superCollider.getVerboseLevel();
        serverLogging.addItem("Off", true, verboseLevel == 0,
            [this] { audioProcessor.superCollider.setVerboseLevel(0); });
        serverLogging.addItem("Level 1", true, verboseLevel == 1,
            [this] { audioProcessor.superCollider.setVerboseLevel(1); });
        serverLogging.addItem("Level 2", true, verboseLevel == 2,
            [this] { audioProcessor.superCollider.setVerboseLevel(2); });

        logging.addSubMenu("UDP", udpLogging);
        logging.addSubMenu("Server", serverLogging);

        ret.addItem("Configure scplugin path...", true, false, [this] {
            settingsWindow = new juce::AlertWindow("SCSynDef path", "", juce::AlertWindow::NoIcon);
            settingsWindow->addTextBlock("SCSynDef path");
            settingsWindow->addTextEditor("scsynthdef", audioProcessor.synthPath);
            settingsWindow->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey, 0, 0));
            settingsWindow->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey, 0, 0));

            settingsWindow->enterModalState(true, juce::ModalCallbackFunction::create([this](int r) {
                if (r) {
                    audioProcessor.logger.scprintf("RESTART PLUGIN FOR SETTINGS TO TAKE EFFECT\n");
                    juce::PropertiesFile *prop = audioProcessor.appProp.getUserSettings();
                    prop->setValue("synthPath", this->settingsWindow->getTextEditorContents("scsynthdef"));
                    audioProcessor.appProp.saveIfNeeded();
                }
            }), true);
        });

#ifndef STATIC_PLUGINS
        ret.addItem("Configure plugin path...", true, false, [this] {
            settingsWindow = new juce::AlertWindow("Plugin path", "", juce::AlertWindow::NoIcon);
            settingsWindow->addTextBlock("Plugin path");
            settingsWindow->addTextEditor("pluginPath", audioProcessor.pluginPath);
            settingsWindow->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey, 0, 0));
            settingsWindow->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey, 0, 0));

            settingsWindow->enterModalState(true, juce::ModalCallbackFunction::create([this](int r) {
                if (r) {
                    audioProcessor.logger.scprintf("RESTART PLUGIN FOR SETTINGS TO TAKE EFFECT\n");
                    juce::PropertiesFile *prop = audioProcessor.appProp.getUserSettings();
                    prop->setValue("pluginPath", this->settingsWindow->getTextEditorContents("pluginPath"));
                    audioProcessor.appProp.saveIfNeeded();
                }
            }), true);
        });
#endif
        ret.addSeparator();
        ret.addSubMenu("Logging", logging);
        ret.addSeparator();
        ret.addItem("Reboot server", true, false, [this] {
            audioProcessor.superCollider.reboot();
        });
        }
        break;
    case 1:
        ret.addItem("Midi keyboard", [this] {
            midikeyboard.reset(new MidiKeyboardWindow(audioProcessor.midiKeyboardState));
        });
        ret.addSeparator();
        ret.addItem("Internal plugin state (advanced debugging)", [this] {
            ValueTreeDebugger *vtd = new ValueTreeDebugger(audioProcessor.pluginState);
            value_tree_debugger.reset(vtd);
        });
        break;
    case 2:
        ret.addItem("About...", [this] {
            auto opts = juce::MessageBoxOptions().withTitle("Info").withMessage(juce::String("PluginCollider\n\nUsing SuperCollider ") + SC_VersionString() + "\n\nBuilt on " +  __DATE__).withButton("OK");
            juce::AlertWindow::showAsync(opts, [](int res) {});
        });
    }
    return ret;
}

void PluginColliderAudioProcessorEditor::paint(juce::Graphics &g) {
    // (Our component is opaque, so we must completely fill the background with
    // a solid colour)
    g.fillAll(
        getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(juce::Colours::white);
    g.setFont(15.0f);
}

void PluginColliderAudioProcessorEditor::resized() {
    int menuSize = juce::LookAndFeel::getDefaultLookAndFeel().getDefaultMenuBarHeight();
    menuBar->setBounds(0, 0, getWidth(), menuSize);
    udpPort.setBounds(10, 8 + menuSize, 70, 25);
    setUdpPortButton.setBounds(70, 8 + menuSize, 100, 25);
    stats.setBounds(194, 8 + menuSize, getWidth() - 194, 25);

    Component* comps[] = { &treeView, &layoutResizer, &rightPane };
    layout.layOutComponents(comps, 3,  0, 8 + menuSize + 35, getWidth(), getHeight() - (8+menuSize+35), false, true);    

    auto area = rightPane.getLocalBounds();
    logViewer.setBounds(area.removeFromBottom(200));
    dynamicViewPanel.setBounds(area);
}

//==============================================================================
void PluginColliderAudioProcessorEditor::menuItemSelected(int x, int y) {
}
