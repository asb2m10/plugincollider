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

class ScratchpadPanel : public juce::Component {
    PluginColliderAudioProcessor &audioProcessor;
    juce::Value code;
    juce::CodeDocument codeDocument;
    juce::CodeEditorComponent codeEditor;
    juce::ValueTree vt;
public:
    ScratchpadPanel(juce::ValueTree item, PluginColliderAudioProcessor &processor) : audioProcessor(processor), vt(item),
        codeEditor(codeDocument, nullptr) {
        addAndMakeVisible(codeEditor);

        if ( vt.isValid() ) {
            codeDocument.replaceAllContent(vt.getProperty(IDs::spCode, ""));
        }
    }

    ~ScratchpadPanel() override {
        if ( vt.isValid() ) {
            vt.setProperty(IDs::spCode, codeDocument.getAllContent(), nullptr);
        }
    }

    void resized() override {
        codeEditor.setBounds(getLocalBounds());
    }
};