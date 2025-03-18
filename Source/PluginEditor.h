/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

//#include <JuceHeader.h>
#include "PluginProcessor.h"

class SynthDefPanel : public juce::Component {
    juce::ValueTree &vt;
    juce::TextButton loaddef;
    juce::Label synthname;
    juce::ToggleButton autoStart;
    std::unique_ptr<juce::FileChooser> scsynthChooser;
public:
    juce::TextButton play;
    juce::TextButton stop;
    juce::TextButton set;
    juce::TextEditor setterIdx;
    juce::TextEditor setterValue;

    SynthDefPanel(juce::ValueTree &vt) : vt(vt) {
        addAndMakeVisible(loaddef);
        loaddef.setButtonText("Load");
        loaddef.onClick = [this] () {
            scsynthChooser = std::make_unique<juce::FileChooser> ("Please select the moose you want to load...",
                                               juce::File("/home/asb2m10/src/plugincollider/scsyndef"),
                                               "*.scsyndef");
            auto folderChooserFlags = juce::FileBrowserComponent::openMode;

            scsynthChooser->launchAsync (folderChooserFlags, [this] (const juce::FileChooser& chooser) {
                juce::File scfile (chooser.getResult());
                if ( !scfile.exists() )
                    return;

                std::unique_ptr<SynthDef> def;
                def.reset(SynthDef::fromFile(scfile));

                if ( def != nullptr ) {
                    synthname.setText(juce::String("Synth: ") + def->getName(), juce::NotificationType::dontSendNotification);
                    this->vt.setProperty(IDs::synthdef, def->getContent(), nullptr);
                } else {
                    auto opts = juce::MessageBoxOptions().withTitle ("Error").withMessage("Unable to read Synthdef file");
                    juce::AlertWindow::showAsync(opts, [](int res) {});
                }
            });
        };

        addAndMakeVisible(synthname);
        addAndMakeVisible(play);
        play.setButtonText("Play");
        addAndMakeVisible(stop);
        stop.setButtonText("Stop");
        addAndMakeVisible(autoStart);
        autoStart.setButtonText("Auto load");

        addAndMakeVisible(set);
        set.setButtonText("Set Value");
        addAndMakeVisible(setterIdx);
        addAndMakeVisible(setterValue);
        refresh();
    }

    void refresh() {
        juce::var ret = vt.getProperty(IDs::synthdef);
        juce::MemoryBlock *mb = ret.getBinaryData();
        std::unique_ptr<SynthDef> def(SynthDef::fromMemory(*mb));
        if ( def != nullptr )
            synthname.setText(juce::String("Synth: ") + def->getName(), juce::NotificationType::dontSendNotification);
        else
            synthname.setText("Synth: no synthdef loaded", juce::NotificationType::dontSendNotification);
    }

    void resized() override {
        auto bounds = getBounds();

        synthname.setBounds(50, 5, bounds.getWidth() - 200, 25);
        loaddef.setBounds(0, 5, 50, 25);
        play.setBounds(0, 35, 50, 25);
        stop.setBounds(55, 35, 50, 25);
        autoStart.setBounds(110, 35, 100, 25);

        set.setBounds(bounds.getWidth() - 95, 5, 90, 25);
        setterIdx.setBounds(bounds.getWidth() - 95, 35, 30, 25);
        setterValue.setBounds(bounds.getWidth() - 52, 35, 42, 25);
    }
};

class LogViewer : public juce::TextEditor {
    juce::StringArray *log;

public:
    LogViewer(juce::StringArray *content) {
        log = content;
        setMultiLine(true);
        setReadOnly(true);
        setScrollbarsShown(true);
    }

    void addPopupMenuItems(juce::PopupMenu &menuToAddTo,
                           const juce::MouseEvent *mouseClickEvent) {
        menuToAddTo.addItem(juce::StandardApplicationCommandIDs::copy, "Copy");
        menuToAddTo.addItem(juce::StandardApplicationCommandIDs::selectAll,
                            "Select All");
        menuToAddTo.addSeparator();
        menuToAddTo.addItem(juce::StandardApplicationCommandIDs::del,
                            "Clear logs");
    }

    void performPopupMenuAction(int menuItemID) {
        if (menuItemID == juce::StandardApplicationCommandIDs::del)
            log->clearQuick();
        else
            juce::TextEditor::performPopupMenuAction(menuItemID);
    }
};

//==============================================================================
/**
 */
class PluginColliderAudioProcessorEditor : public juce::AudioProcessorEditor,
                                           public juce::Timer {
  public:
    PluginColliderAudioProcessorEditor(PluginColliderAudioProcessor &);
    ~PluginColliderAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics &) override;
    void resized() override;

    virtual void timerCallback() override;

  private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    PluginColliderAudioProcessor &audioProcessor;
    juce::TextButton freeAll;
    LogViewer logViewer;
    juce::TextButton configButton;
    juce::TextButton rebootButton;
    juce::TextEditor udpPort;
    juce::TextButton setUdpPortButton;
    juce::TextButton showSynthdefs;

    int logLines = 0;
    juce::Label stats;
    juce::AlertWindow *settingsWindow;

    // For now this is for debugging
    juce::Slider cb1;
    std::unique_ptr<juce::SliderParameterAttachment> cb1Attachment;
    SynthDefPanel synthDefPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        PluginColliderAudioProcessorEditor)
};
