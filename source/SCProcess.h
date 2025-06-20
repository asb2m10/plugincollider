/*
    PluginCollider Copyright (c) 2025 Pascal Gauthier.
    SuperColliderAU Copyright (c) 2006 Gerard Roma.

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

#include <stdio.h>
#include "SC_CoreAudio.h"
#include "SC_HiddenWorld.h"
#include "SC_World.h"
#include "SC_WorldOptions.h"
#include "OSCMessages.h"
#include "sc_msg_iter.h"
#include "SCPluginDriver.h"
#include "SC_Node.h"
#include "scsynthsend.h"

#include "CommandFifo.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_osc/juce_osc.h>

/**
 * @brief This class represent a compiled SynthDef file.
 *
 */
class SynthDef {
    juce::MemoryBlock memoryBlock;
    juce::String name;
    juce::StringArray parameters;
    std::unique_ptr<float []> parametersValues;

public:
    static SynthDef *fromFile(juce::File file) {
        if (!file.existsAsFile())
            return nullptr;
        juce::MemoryBlock content;
        if (!file.loadFileAsData(content))
            return nullptr;
        return SynthDef::fromMemory(content);
    }
    static SynthDef *fromMemory(juce::MemoryBlock &newContent);

    juce::MemoryBlock &getContent() {
        return memoryBlock;
    }

    juce::String getName() {
        return name;
    }

    juce::StringArray getParameters() {
        return parameters;
    }

    float* getParametersValues() {
        return parametersValues.get();
    }

    juce::String guessParameterRange(int idx) {
        if ( idx < 0 || idx >= parameters.size() )
            return "0 1 0.001"; // default range

        juce::String name = parameters[idx];
        if ( name == "gate" ) {
            return "0 1 1"; // gate is always 0 or 1
        } else if ( name == "freq" ) {
            return "20 22000 0.1"; // frequency range
        } else if ( name == "amp" ) {
            return "0 1 0.001"; // amplitude range
        } else if ( name == "pan" ) {
            return "-1 1 0.001"; // pan range
        } else if ( name == "pitch" ) {
            return "-12 12 0.01"; // pitch range
        } else if ( name == "out" ) {
            return "1 16 1";
        }

        // we do our best to find the best low / high values based on the defaultValue
        int low, high;
        float defaultValue = parametersValues[idx];
        if ( defaultValue > -1 && defaultValue < 1 ) {
            low = -1;
            high = 1;
        } else {
            low = defaultValue / 5;
            high = defaultValue * 5;
        }
        return juce::String(low) + " " + juce::String(high) + " 0.001";
    }
};


// Dirty cheap logger
class SuperLogger : public juce::Logger {
public:
    juce::StringArray content;

    /**
     * Standard log message from current instance.
     */
    void log(const juce::String &message) {
        logMessage(message);
    }

    /**
     * Overriden messasge that might be called from static context.
     */
    void logMessage(const juce::String &message) override {
        if (content.size() > 4096)
            content.removeRange(0, 2048);
        content.add(message);
    }

    /**
     * Printf-like function that logs to the console and to the logger.
     */
    void scprintf(const char *fmt, ...) {
        va_list ap;
        va_start(ap, fmt);
        char buf[4096];
        int p = vsnprintf(buf, sizeof(buf), fmt, ap);
        printf("%s", buf);
        log(juce::String(buf));
    }
};


/**
 * @brief Out of the box recent C++ SuperCollider Node accessor with
 * embedded exceptions.
 */
class SCNodeWalker {
    Node *noderef;
public:
    SCNodeWalker(Node *node) {
        noderef = node;
    }

    bool isValid() {
        return noderef != nullptr;
    }

    bool isGroup() {
        if ( noderef == nullptr )
            return false;
        return noderef->mIsGroup != 0;
    }

    Node *node() {
        if ( noderef == nullptr )
            throw std::invalid_argument("Node is null");
        return noderef;
    }

    Group *group() {
        Node *n = node();
        if ( !n->mIsGroup )
            throw std::invalid_argument("Node is not a group");
        return (Group *) n;
    }

    SCNodeWalker parent() {
        Node *n = node();
        return SCNodeWalker((Node *) n->mParent);
    }

    SCNodeWalker next() {
        Node *n = node();
        return SCNodeWalker(n->mNext);
    }
};


class SCProcess {
public:
    struct WorldStats {
        uint32 mNumUnits, mNumGraphs, mNumGroups;
        WorldStats() { mNumUnits = mNumGraphs = mNumGroups = 0; }
    };

    SCProcess(SuperLogger &logger);
    ~SCProcess();
    void quit();

    /* returns true if the server has booted / rebooted */
    bool setup(float sampleRate, int buffSize, int numInputs, int numOutput,
               juce::String pluginPath, juce::String synthdefPath);
    void reboot();
    void run(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages);
    bool unrollOSCPacket(int inSize, char *inData, OSC_Packet *inPacket);
    //void setNodeValue(int nodeId, juce::String parmName, float value);

    void getWorldStats(WorldStats *stats) {
        const juce::GenericScopedTryLock<juce::CriticalSection> scopeLock(worldLock);
        if (scopeLock.isLocked()) {
            if (world != nullptr) {
                stats->mNumUnits = world->mNumUnits;
                stats->mNumGraphs = world->mNumGraphs;
                stats->mNumGroups = world->mNumGroups;
            }
        }
    }

    void freeNodes(int rootNodeId = 0);
    juce::StringArray getRegistredUnits();

    // Anything rt_ should be called from the audio thread since the worldLock is already acquired
    // ======================
    SCNodeWalker rt_getNode(int destNode);
    void rt_setControlBusValue(int bus, float value);
    bool rt_loadSynthDef(juce::MemoryBlock *block);
    void rt_freeGroup(int rootGroup);
    void rt_freeNode(int destNode);
    void rt_setNodeValue(int destNode, int idx, float value);
    void rt_getSynthDef(HeapStringList<64,4096> &list);
    SCErr rt_newGroup(int parentNode, int destGroup);
    int32_t rt_newSynth(juce::String name, int newId, int destNode);
    SCErr rt_queryTree(int rootGroup, big_scpacket *packet, bool flagParameters = false);
    void rt_dumpTree();
    void rt_assignControlBus(int nodeId, int nodeParamIdx, int busIdx);
    // ======================

    int getVerboseLevel() {
        return world != nullptr ? world->mVerbosity : 0;
    }

    int getOSCDumpLevel() {
        return world != nullptr ? world->mDumpOSC : 0;
    }

    void setVerboseLevel(int level) {
        if ( world != nullptr )
            world->mVerbosity = level;
    }

    void setOSCDumpLevel(int level) {
        if ( world != nullptr )
            world->mDumpOSC = level;
    }

private:
    friend class PluginColliderAudioProcessor;

    SuperLogger &logger;
    World *world;
    juce::CriticalSection worldLock;

    void bootServer();

    // ATTIC
    // ---
    //void sendParamChangeMessage(string name, float value);
    //void sendNote(int64 oscTime, int note, int velocity);
    //void sendTick(int64 oscTime, int bus);

    float sampleRate;
    int bufferSize;
    int numInputs;
    int numOutputs;
    juce::String pluginPath;
    juce::String synthdefPath;
};
