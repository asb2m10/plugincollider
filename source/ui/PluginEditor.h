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
#include "DynamicViewPanel.h"

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
                                           public juce::Timer, public juce::MenuBarModel {
  public:
    PluginColliderAudioProcessorEditor(PluginColliderAudioProcessor &);
    ~PluginColliderAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics &) override;
    void resized() override;

    virtual void timerCallback() override;

    juce::PopupMenu getMenuForIndex(int topLevelMenuIndex, const juce::String& str) override;
    void menuItemSelected(int, int) override;
    juce::StringArray getMenuBarNames() {
        return juce::StringArray({"Server", "SynthDef", "Tools", "Help" });
    }

  private:
    PluginColliderAudioProcessor &audioProcessor;
    LogViewer logViewer;
    juce::TextButton configButton;
    juce::TextEditor udpPort;
    juce::TextButton setUdpPortButton;
    SCProcess::WorldStats worldStats;

    int logLines = 0;
    juce::Label stats;
    juce::AlertWindow *settingsWindow;
    std::unique_ptr<juce::MenuBarComponent> menuBar;

    juce::StretchableLayoutManager layout;
    juce::StretchableLayoutResizerBar layoutResizer;

    // For now this is for debugging
    juce::Slider cb1;
    std::unique_ptr<juce::SliderParameterAttachment> cb1Attachment;

    DynamicViewPanel dynamicViewPanel;
    juce::Component rightPane;
    juce::TreeView treeView;

//#ifdef DEBUG
    std::unique_ptr<juce::DocumentWindow> value_tree_debugger;
    std::unique_ptr<juce::DocumentWindow> midikeyboard;
//#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        PluginColliderAudioProcessorEditor)
};
