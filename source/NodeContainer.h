/**
 *
 * Plugincollider Copyright (c) 2025 Pascal Gauthier.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 *
 */

#pragma once
#include <juce_core/juce_core.h>
#include "PluginModel.h"
#include "SCProcess.h"

class BaseNode {
public:
    int nodeid, parentid;

    enum class NodeType {
        Group, FX, Note
    };
    NodeType type;

    BaseNode(juce::ValueTree &vt, int parentid) : parentid(parentid) {
        nodeid = vt[IDs::nodeid];
        type = NodeType::Group;
    }
};

class SynthNode : public BaseNode {
protected:
    bool valid = false;
    std::unordered_map<int, float> precompiledMapValue;
    std::unordered_map<int, int> controlBusMap;
    char synthName[127] = { 0 };
public:
    SynthNode(juce::ValueTree &vt, int parentid) : BaseNode(vt, parentid) {
        if ( ! vt.hasProperty(IDs::synthBlob) ) {
            return;
        }

        juce::String name = vt[IDs::synthName];
        strncpy(synthName, name.toRawUTF8(), 127);
    }

    bool isValid() const {
        return valid;
    }
};

class FXNode : public SynthNode {
public:
    FXNode(juce::ValueTree &vt, int parentid) : SynthNode(vt, parentid) {
        juce::ValueTree parameters = vt.getChildWithName(IDs::parameters);
        for(int i=0;i< parameters.getNumChildren(); i++) {
            juce::ValueTree param = parameters.getChild(i);
            int cbIdx = param.getProperty(IDs::pControlBus);
            if ( cbIdx != -1 ) {
                controlBusMap.emplace(i, cbIdx);
                continue;
            }

            if ( param.hasProperty(IDs::pCurrentValue) && param.getProperty(IDs::pCurrentValue) != param.getProperty(IDs::pDefaultValue) ) {
                float value = param.getProperty(IDs::pCurrentValue);
                precompiledMapValue.emplace(i, value);
            }
        }
        type = NodeType::FX;
        valid = true;
    }

    void rt_start(SCProcess &superCollider) {
        superCollider.rt_newSynth((int *) synthName, nodeid, parentid);
        for(const auto &p: controlBusMap) {
            superCollider.rt_assignControlBus(nodeid, p.first, p.second);
        }
        for(const auto &p: precompiledMapValue) {
            superCollider.rt_setNodeValue(nodeid, p.first, p.second);
        }
    }
};

class MidiNode : public SynthNode {
    int lowNote = 0, highNote = 127, freqIdx = -1, velocityIdx = -1, gateIdx = -1;
    bool mono = false, legato = false;
    int activeSynths[127] = { 0 };

public:
    MidiNode(juce::ValueTree &vt, int parentid) : SynthNode(vt, parentid) {
        juce::String range = vt[IDs::synthNoteRange];
        juce::StringArray token;
        token.addTokens(range, " ");
        lowNote = token[0].getIntValue();
        highNote = token[1].getIntValue();
        // mono = vt[IDs::mono];
        // legato = vt[IDs::legato];

        juce::ValueTree parameters = vt.getChildWithName(IDs::parameters);
        for(int i=0;i< parameters.getNumChildren(); i++) {
            juce::ValueTree param = parameters.getChild(i);

            juce::String pName = param.getProperty(IDs::pName);
            if ( pName == "freq" ) {
                freqIdx = i;
                continue;
            }
            if ( pName == "amp" ) {
                velocityIdx = i;
                continue;
            }
            if ( pName == "gate" ) {
                gateIdx = i;
                continue;
            }

            int cbIdx = param.getProperty(IDs::pControlBus);
            if ( cbIdx != -1 ) {
                controlBusMap.emplace(i, cbIdx);
                continue;
            }

            if ( param.hasProperty(IDs::pCurrentValue) && param.getProperty(IDs::pCurrentValue) != param.getProperty(IDs::pDefaultValue) ) {
                float value = param.getProperty(IDs::pCurrentValue);
                precompiledMapValue.emplace(i, value);
            }
        }

        type = NodeType::Note;
        valid = true;
    }

    void noteOn(SCProcess &superCollider, int note, float velocity) {
        if ( note < lowNote || note > highNote )
            return;

        int lastNode = activeSynths[note];
        if ( lastNode != 0 ) {
            if ( superCollider.rt_getNode(lastNode).isValid() ) {
                if ( gateIdx != -1 ) {
                    superCollider.rt_freeNode(lastNode);
                }
            }
        }

        int node = superCollider.rt_newSynth((int *) synthName, -1, nodeid);
        if ( node == 0 )
            return;

        activeSynths[note] = node;
        for(const auto &p: controlBusMap) {
            superCollider.rt_assignControlBus(node, p.first, p.second);
        }
        for(const auto &p: precompiledMapValue) {
            superCollider.rt_setNodeValue(node, p.first, p.second);
        }
        if ( freqIdx != -1 ) {
            superCollider.rt_setNodeValue(node, freqIdx, juce::MidiMessage::getMidiNoteInHertz(note));
        }
        if ( velocityIdx != -1 ) {
            superCollider.rt_setNodeValue(node, velocityIdx, velocity);
        }
    }

    /**
     * Trigger gate = 0 for midi note off
     */
    void noteOff(SCProcess &superCollider, int note) {
        if ( note < lowNote || note > highNote )
            return;
        if ( activeSynths[note] != 0 ) {
            if ( superCollider.rt_getNode(activeSynths[note]).isValid() && gateIdx != -1 ) {
                superCollider.rt_setNodeValue(activeSynths[note], gateIdx, 0);
            }
        }
    }

    /**
     * Ignore gate message, kill the node
     */
    void nodeOff(SCProcess &superCollider, int note) {
        if ( note < lowNote || note > highNote )
            return;
        superCollider.rt_freeNode(activeSynths[note]);
    }

    void panic(SCProcess &superCollider) {
        for(int i=0;i<127;i++) {
            if ( activeSynths[i] != 0 ) {
                if ( superCollider.rt_getNode(activeSynths[i]).isValid() ) {
                    superCollider.rt_freeNode(activeSynths[i]);
                }
                activeSynths[i] = 0;
            }
        }

    }
};

class NodeContainer {
    std::vector<std::unique_ptr<BaseNode>> globalnodes;
    std::vector<MidiNode*> midinodes;

    void insertNode(juce::ValueTree nodes, int parentId) {
        for (auto node : nodes) {
            if ( node.hasType(IDs::notenode) ) {
                std::unique_ptr<MidiNode> midiNode = std::make_unique<MidiNode>(node, parentId);
                if ( ! midiNode->isValid() )
                    continue;
                midinodes.push_back(midiNode.get());
                globalnodes.emplace_back(std::move(midiNode));
                continue;
            }
            
            if ( node.hasType(IDs::fxnode) ) {
                std::unique_ptr<FXNode> fxNode = std::make_unique<FXNode>(node, parentId);
                if ( ! fxNode->isValid() )
                    continue;
                globalnodes.emplace_back(std::move(fxNode));
                continue;
             }

            if (node.hasType(IDs::groupnode)) {
                globalnodes.emplace_back(std::make_unique<BaseNode>(node, parentId));
                insertNode(node, node[IDs::nodeid]);
            }
        }
    }

public:
    NodeContainer(juce::ValueTree rootNode) {
        insertNode(rootNode, 1);
    }

    void rt_allocate(SCProcess &superCollider) {
        for(auto &fx: globalnodes) {
            switch(fx->type) {
                case BaseNode::NodeType::Group:
                case BaseNode::NodeType::Note:
                    //scprintf("Add group %d parent %d\n", fx->nodeid, fx->parentid);
                    superCollider.rt_newGroup(fx->parentid, fx->nodeid);
                    break;
                case BaseNode::NodeType::FX: {
                    FXNode *fxNode = static_cast<FXNode *>(fx.get());
                    //scprintf("Add fx %d parent %d\n", fxNode->nodeid, fxNode->parentid);
                    fxNode->rt_start(superCollider);
                    break;
                }
            }
        }
    }

    void rt_free(SCProcess &superCollider) {
        for(auto it = globalnodes.rbegin(); it != globalnodes.rend(); it++ ) {
            int nodeId = it->get()->nodeid;
            if ( nodeId == 1 )
                continue;
            superCollider.rt_freeNode(nodeId);
        }
    }

    void rt_panic(SCProcess &superCollider) {
        for(auto midinote: midinodes) {
            midinote->panic(superCollider);
        }
    }

    void rt_processMidiMessages(SCProcess &superCollider, juce::MidiBuffer &midiMessages) {
        int scheduledNote[128] = { 0 };

        for (const auto meta : midiMessages) {
            const auto msg = meta.getMessage();
            int note = msg.getNoteNumber();
            if ( msg.isNoteOn() ) {
                scheduledNote[note] = 1;
                for(auto midinote: midinodes) {
                    midinote->noteOn(superCollider, note, msg.getFloatVelocity());
                }
            } else if ( msg.isNoteOff() ) {
                for(auto midinote: midinodes) {
                    if ( scheduledNote[note] == 0 )
                        midinote->noteOff(superCollider, note);
                    else
                        midinote->nodeOff(superCollider, note);
                }
            }
        }
    }
};
