/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

//#include <JuceHeader.h>
#include "PluginProcessor.h"

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
    int logLines = 0;
    juce::Label stats;
    juce::AlertWindow *settingsWindow;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        PluginColliderAudioProcessorEditor)
};
