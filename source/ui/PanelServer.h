/*
PluginCollider Copyright (c) 2026 Pascal Gauthier.

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
#include "PluginModel.h"
#include <juce_gui_basics/juce_gui_basics.h>

class ServerPanel : public juce::Component {
    PluginColliderAudioProcessor &processor;
    juce::Label titleLabel;
    juce::ValueTree vt;

    // Server configuration components
    juce::Label maxWireBufsLabel;
    juce::TextEditor maxWireBufs;

    juce::Label realTimeMemorySizeLabel;
    juce::TextEditor realTimeMemorySize;

    juce::Label numBuffersLabel;
    juce::TextEditor numBuffers;

    juce::Label maxLoginsLabel;
    juce::TextEditor maxLogins;

    juce::Label pluginPathLabel;
    juce::TextEditor pluginPath;

    juce::Label synthDefPathLabel;
    juce::TextEditor synthDefPath;

    juce::TextButton rebootServerButton;
    juce::TextButton defaultConfigButton;
public:
    ServerPanel(const juce::ValueTree &srvVt, PluginColliderAudioProcessor &processor) : vt(srvVt), processor(processor) {
        titleLabel.setText("Server Configuration", juce::dontSendNotification);
        titleLabel.setJustificationType(juce::Justification::centredLeft);
        titleLabel.setFont(juce::Font(16.0f, juce::Font::bold));
        addAndMakeVisible(titleLabel);

        // Max Wire Buffers
        maxWireBufsLabel.setText("Max Wire Buffers:", juce::dontSendNotification);
        maxWireBufsLabel.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(maxWireBufsLabel);
        maxWireBufs.setText(vt.getProperty(IDs::srvMaxWireBufs, 64), juce::dontSendNotification);
        addAndMakeVisible(maxWireBufs);

        // Real-time Memory Size (in KB)
        realTimeMemorySizeLabel.setText("RT Memory Size (KB):", juce::dontSendNotification);
        realTimeMemorySizeLabel.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(realTimeMemorySizeLabel);
        realTimeMemorySize.setText(vt.getProperty(IDs::srvRealTimeMemorySize, 8192), juce::dontSendNotification);
        addAndMakeVisible(realTimeMemorySize);

        // Number of Buffers
        numBuffersLabel.setText("Number of Buffers:", juce::dontSendNotification);
        numBuffersLabel.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(numBuffersLabel);
        numBuffers.setText(vt.getProperty(IDs::srvNumBuffers, 8192), juce::dontSendNotification);
        addAndMakeVisible(numBuffers);

        // Max Logins
        maxLoginsLabel.setText("Max Logins:", juce::dontSendNotification);
        maxLoginsLabel.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(maxLoginsLabel);
        maxLogins.setText(vt.getProperty(IDs::srvMaxLogins, 32), juce::dontSendNotification);
        addAndMakeVisible(maxLogins);

        // Plugin Path
        pluginPathLabel.setText("SCPlugin path:", juce::dontSendNotification);
        pluginPathLabel.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(pluginPathLabel);
        pluginPath.setText(vt.getProperty(IDs::srvPluginPath), juce::dontSendNotification);
        addAndMakeVisible(pluginPath);

        // Plugin Path
        synthDefPathLabel.setText("SynthDef path:", juce::dontSendNotification);
        synthDefPathLabel.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(synthDefPathLabel);
        synthDefPath.setText(vt.getProperty(IDs::srvSynthDefPath), juce::dontSendNotification);
        addAndMakeVisible(synthDefPath);

        // Reboot Server Button
        rebootServerButton.setButtonText("Reboot Server");
        addAndMakeVisible(rebootServerButton);

        defaultConfigButton.setButtonText("Set as default config");
        addAndMakeVisible(defaultConfigButton);

        // Setup callbacks to update ValueTree
        maxWireBufs.onTextChange = [this]() {
            vt.setProperty(IDs::srvMaxWireBufs, maxWireBufs.getText().getIntValue(), nullptr);
        };

        realTimeMemorySize.onTextChange = [this]() {
            vt.setProperty(IDs::srvRealTimeMemorySize, realTimeMemorySize.getText().getIntValue(), nullptr);
        };

        numBuffers.onTextChange = [this]() {
            vt.setProperty(IDs::srvNumBuffers, numBuffers.getText().getIntValue(), nullptr);
        };

        maxLogins.onTextChange = [this]() {
            vt.setProperty(IDs::srvMaxLogins, maxLogins.getText().getIntValue(), nullptr);
        };

        pluginPath.onTextChange = [this]() {
            vt.setProperty(IDs::srvPluginPath, pluginPath.getText(), nullptr);
        };

        synthDefPath.onTextChange = [this]() {
            vt.setProperty(IDs::srvSynthDefPath, synthDefPath.getText(), nullptr);
        };

        rebootServerButton.onClick = [this]() {
            this->processor.rebootServer();
        };

        defaultConfigButton.onClick = [this]() {
            juce::PropertiesFile *prop = this->processor.appProp.getUserSettings();
            prop->setValue("srvMaxWireBufs", maxWireBufs.getText().getIntValue());
            prop->setValue("srvRealTimeMemorySize", realTimeMemorySize.getText().getIntValue());
            prop->setValue("srvNumBuffers", numBuffers.getText().getIntValue());
            prop->setValue("srvMaxLogins", maxLogins.getText().getIntValue());
            prop->setValue("pluginPath", pluginPath.getText());
            prop->setValue("synthPath", synthDefPath.getText());
            this->processor.appProp.saveIfNeeded();
        };
    }

    void resized() override {
        auto componentHeight = 25;
        auto labelWidth = 150;
        auto spacing = 5;

        auto bounds = getLocalBounds();
        bounds.removeFromTop(10);

        auto rowBounds = bounds.removeFromTop(componentHeight);

        auto buttonBounds = rowBounds.removeFromRight(120);
        rebootServerButton.setBounds(buttonBounds);
        rowBounds.removeFromRight(spacing);
        buttonBounds = rowBounds.removeFromRight(200);
        defaultConfigButton.setBounds(buttonBounds);
        titleLabel.setBounds(rowBounds);
        bounds.removeFromTop(10);

        // Max Wire Buffers
        rowBounds = bounds.removeFromTop(componentHeight);
        maxWireBufsLabel.setBounds(rowBounds.removeFromLeft(labelWidth));
        rowBounds.removeFromLeft(spacing);
        maxWireBufs.setBounds(rowBounds.removeFromLeft(labelWidth));
        bounds.removeFromTop(spacing);

        // Real-time Memory Size
        // rowBounds = bounds.removeFromTop(componentHeight);
        realTimeMemorySizeLabel.setBounds(rowBounds.removeFromLeft(labelWidth));
        rowBounds.removeFromLeft(spacing);
        realTimeMemorySize.setBounds(rowBounds.removeFromLeft(labelWidth));
        bounds.removeFromTop(spacing);

        // Number of Buffers
        rowBounds = bounds.removeFromTop(componentHeight);
        numBuffersLabel.setBounds(rowBounds.removeFromLeft(labelWidth));
        rowBounds.removeFromLeft(spacing);
        numBuffers.setBounds(rowBounds.removeFromLeft(labelWidth));
        bounds.removeFromTop(spacing);

        // Max Logins
        // rowBounds = bounds.removeFromTop(componentHeight);
        maxLoginsLabel.setBounds(rowBounds.removeFromLeft(labelWidth));
        rowBounds.removeFromLeft(spacing);
        maxLogins.setBounds(rowBounds.removeFromLeft(labelWidth));
        bounds.removeFromTop(spacing * 2);

        // Plugin Path
        rowBounds = bounds.removeFromTop(componentHeight);
        pluginPathLabel.setBounds(rowBounds.removeFromLeft(labelWidth));
        rowBounds.removeFromLeft(spacing);
        pluginPath.setBounds(rowBounds);
        bounds.removeFromTop(spacing * 2);

        // SynthDef Path
        rowBounds = bounds.removeFromTop(componentHeight);
        synthDefPathLabel.setBounds(rowBounds.removeFromLeft(labelWidth));
        rowBounds.removeFromLeft(spacing);
        synthDefPath.setBounds(rowBounds);
        bounds.removeFromTop(spacing * 2);
    }
};