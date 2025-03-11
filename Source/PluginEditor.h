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
    SynthDefPanel(juce::ValueTree &vt) : vt(vt) {
        addAndMakeVisible(loaddef);
        loaddef.setButtonText("Load Synthdef...");
        loaddef.onClick = [this] () {
            scsynthChooser = std::make_unique<juce::FileChooser> ("Please select the moose you want to load...",
                                               juce::File("/home/asb2m10/.local/share/SuperCollider/synthdefs"),
                                               "*.scsyndef");
            auto folderChooserFlags = juce::FileBrowserComponent::openMode;

            scsynthChooser->launchAsync (folderChooserFlags, [this] (const juce::FileChooser& chooser) {
                juce::File scfile (chooser.getResult());
                std::unique_ptr<SynthDef> def;
                def.reset(SynthDef::fromFile(scfile));

                if ( def != nullptr ) {
                    scprintf("Found %s", def->getName().toRawUTF8());
                    synthname.setText(def->getName(), juce::NotificationType::dontSendNotification);
                    this->vt.setProperty(IDs::synthdef, def->getContent(), nullptr);
                }
            });
        };
    }

    void refresh() {

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
    juce::TextButton loadSynthdefs;

    std::unique_ptr<juce::FileChooser> scsynthChooser;

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
