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

class PanelServer : public juce::Component {
    PluginColliderAudioProcessor &processor;
    juce::Label labelPluginPath;
    juce::TextEditor pluginPath;

    juce::Label labelSynthPath;
    juce::TextEditor synthPath;


    juce::TextButton restartServer;
    juce::TextButton undoChanges;
    juce::ValueTree vt;
public:
    PanelServer(const juce::ValueTree &serverVt, PluginColliderAudioProcessor &processor) : vt(serverVt), processor(processor) {
        labelPluginPath.setText("Server plugin path", juce::dontSendNotification);
        addAndMakeVisible(labelPluginPath);
        pluginPath.setText("Plugins", juce::dontSendNotification);
        addAndMakeVisible(pluginPath);
        labelSynthPath.setText("Synth path", juce::dontSendNotification);
        addAndMakeVisible(labelSynthPath);
        synthPath.setText("Plugins", juce::dontSendNotification);
        addAndMakeVisible(synthPath);

        restartServer.setButtonText("Restart Server");
        addAndMakeVisible(restartServer);

        undoChanges.setButtonText("Undo Changes");
        addAndMakeVisible(undoChanges);
    }

    void resized() override {
        auto bounds = getLocalBounds();

        bounds.removeFromBottom(3);
        auto bottom = bounds.removeFromBottom(25);
        restartServer.setBounds(bottom.removeFromLeft(150));
        bottom.removeFromLeft(3);
        undoChanges.setBounds(bottom.removeFromLeft(150));

        labelPluginPath.setBounds(bounds.removeFromTop(25));
        pluginPath.setBounds(bounds.removeFromTop(40));
        labelSynthPath.setBounds(bounds.removeFromTop(25));
        synthPath.setBounds(bounds.removeFromTop(40));

    }

    void paint(juce::Graphics &g) override {
        juce::Colour current = getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId);
        g.fillAll(current.darker());
    }
};