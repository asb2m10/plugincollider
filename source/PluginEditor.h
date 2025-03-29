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

class SynthDefTableListModel : public juce::TableListBoxModel {
    juce::ValueTree &vt;
public:
    SynthDefTableListModel(juce::ValueTree &vt) : vt(vt) {

    }

    int getNumColumns() {
        return 4;
    }

    int getNumRows() {
        return 10;
    }

    void paintRowBackground(juce::Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected) override {
        if (rowIsSelected)
            g.fillAll (juce::Colours::lightblue);
        else if (rowNumber % 2)
            g.fillAll (juce::Colours::blue);
    }

    void paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override {

    }


};

class SynthDefTableListTable: public juce::TableListBox {

};

class SynthDefPanel : public juce::Component {
    juce::ValueTree &vt;
    juce::ToggleButton autoStart;
    juce::Label synthname;
    SynthDefTableListTable parmTable;
    SynthDefTableListModel parmModel;
public:
    juce::TextButton loaddef;
    juce::TextButton play;
    juce::TextButton stop;

    SynthDefPanel(juce::ValueTree &vt) : vt(vt), parmModel(vt) {
        addAndMakeVisible(loaddef);
        loaddef.setButtonText("Load");

        addAndMakeVisible(synthname);
        addAndMakeVisible(play);
        play.setButtonText("Play");
        addAndMakeVisible(stop);
        stop.setButtonText("Stop");
        addAndMakeVisible(autoStart);
        autoStart.setButtonText("Play synth on load");

        addAndMakeVisible(parmTable);
        parmTable.setModel(&parmModel);
        parmTable.getHeader().addColumn("Argument", 2, 100);
        parmTable.getHeader().addColumn("Slider", 1, 200);
        parmTable.getHeader().addColumn("Low", 3, 30);
        parmTable.getHeader().addColumn("High", 4, 30);
        parmTable.getHeader().addColumn("Control Bus", 5, 60);

        refresh();
    }

    void refresh() {
        juce::String synthName = vt.getChildWithName(IDs::synths).getChildWithName(IDs::synth).getProperty(IDs::synthName);
        if ( synthName == "" )
            synthName = "No synthDef loaded";
        synthname.setText(juce::String("Synth: ") + synthName, juce::NotificationType::dontSendNotification);
    }

    void resized() override {
        auto bounds = getBounds();

        synthname.setBounds(50, 5, bounds.getWidth() - 200, 25);
        loaddef.setBounds(0, 5, 50, 25);
        play.setBounds(0, 35, 50, 25);
        stop.setBounds(55, 35, 50, 25);
        autoStart.setBounds(0, 65, 200, 25);
        parmTable.setBounds(190, 5, bounds.getWidth() - 190, bounds.getHeight());
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
        return juce::StringArray({"Server", "Synthdef", "Node", "Help" });
    }

  private:
    PluginColliderAudioProcessor &audioProcessor;
    LogViewer logViewer;
    juce::TextButton configButton;
    juce::TextEditor udpPort;
    juce::TextButton setUdpPortButton;
    std::unique_ptr<juce::FileChooser> scsynthChooser;

    int logLines = 0;
    juce::Label stats;
    juce::AlertWindow *settingsWindow;
    std::unique_ptr<juce::MenuBarComponent> menuBar;

    // For now this is for debugging
    juce::Slider cb1;
    std::unique_ptr<juce::SliderParameterAttachment> cb1Attachment;
    SynthDefPanel synthDefPanel;

#ifdef DEBUG
    std::unique_ptr<juce::DocumentWindow> value_tree_debugger;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        PluginColliderAudioProcessorEditor)
};
