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

/**
 ** Dirtycheap SuperCollider language tokenizer. 
 */
class SuperColliderTokeniser : public juce::CodeTokeniser {
public:
    enum TokenType {
        tokenType_Error = 0,
        tokenType_Keyword,
        tokenType_Identifier,
        tokenType_Number,
        tokenType_String,
        tokenType_Comment,
        tokenType_Operator,
        tokenType_Other
    };

    int readNextToken(juce::CodeDocument::Iterator& source) override {
        source.skipWhitespace();

        if ( source.peekNextChar() == '/' ) {
            source.skip();

            if ( source.peekNextChar() == '*' ) {
                bool lastWasStar = false;
                for (;;) {
                    auto c = source.nextChar();
                    if (c == 0 || (c == '/' && lastWasStar))
                        break;
                    lastWasStar = (c == '*');
                }                
                source.skip();
                return tokenType_Comment; // Multi-line comment
            }

            if ( source.peekNextChar() == '/' ) {
                // Single-line comment
                source.skipToEndOfLine();
                return tokenType_Comment;
            }
        }

        if ( source.peekNextChar() == '"' ) {
            // String literal
            source.skip();
            while ( !source.isEOF() ) {
                auto c = source.peekNextChar();
                if (c == '"') {
                    source.skip();
                    break;
                }
                if (c == '\\')
                    source.skip();
                source.skip();
            }
            return tokenType_String;
        }

        if ( juce::CharacterFunctions::isDigit(source.peekNextChar()) ) {
            // Number
            while (juce::CharacterFunctions::isDigit(source.peekNextChar()) || source.peekNextChar() == '.')
                source.skip();
            return tokenType_Number;
        }

        if ( isIdentifierStart(source.peekNextChar()) ) {
            juce::String ident;
            while ( isIdentifierBody(source.peekNextChar()) ) {
                ident += source.peekNextChar();
                source.skip();
            }
            if ( isKeyword(ident) )
                return tokenType_Keyword;
            return tokenType_Identifier;
        }

        if ( isOperator(source.peekNextChar()) ) {
            source.skip();
            return tokenType_Operator;
        }

        if ( !source.isEOF() )
            source.skip();

        return tokenType_Other;
    }

    juce::CodeEditorComponent::ColourScheme getDefaultColourScheme() {
        struct Type {
            const char* name;
            juce::uint32 colour;
        };

        const Type types[] = {
            { "Error",       0xffF44747 },
            { "Keyword",     0xff569CD6 },
            { "Identifier",  0xffffffff },
            { "Number",      0xffB5CEA8 },
            { "String",      0xffD4D4D4 },
            { "Comment",     0xffcccccc },
            { "Operator",    0xffB5CEA8 },
            { "Other",       0xffB5CEA8 },
        };

        juce::CodeEditorComponent::ColourScheme cs;

        for (auto& t : types)
            cs.set (t.name, juce::Colour (t.colour));

        return cs;
    }

private:
    static bool isIdentifierStart (juce::juce_wchar c) {
        return juce::CharacterFunctions::isLetter (c) || c == '_';
    }

    static bool isIdentifierBody (juce::juce_wchar c) {
        return juce::CharacterFunctions::isLetterOrDigit (c) || c == '_';
    }

    static bool isOperator (juce::juce_wchar c) {
        return juce::String("+-*/%=!<>&|^~?:.").containsChar (c);
    }

    static bool isKeyword (const juce::String& ident) {
        static const juce::StringArray keywords {
            "arg", "classvar", "const", "super", "this", "var", "if", "else", "false", "inf", "nil",
            "true", "thisFunction", "thisFunctionDef", "thisMethod", "thisProcess", "thisThread",
            "currentEnvironment", "topEnvironment"
        };
        return keywords.contains (ident);
    }
};

class ScratchpadPanel : public juce::Component {
    juce::CodeDocument codeDocument;
    SuperColliderTokeniser tokenizer;
    juce::CodeEditorComponent codeEditor = { codeDocument, &tokenizer };
    juce::ValueTree vt;
public:
    ScratchpadPanel(juce::ValueTree item, PluginColliderAudioProcessor &processor) : vt(item) {
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