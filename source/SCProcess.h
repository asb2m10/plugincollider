/**
 *
 * Plugincollider Copyright (c) 2021-2025 Pascal Gauthier.
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

#include <stdio.h>
#include "SC_CoreAudio.h"
#include "SC_HiddenWorld.h"
#include "SC_World.h"
#include "SC_WorldOptions.h"
#include "OSCMessages.h"
#include "sc_msg_iter.h"
#include "SCPluginDriver.h"

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

class SCProcess {
public:
    struct WorldStats {
        uint32 mNumUnits, mNumGraphs, mNumGroups;
        WorldStats() { mNumUnits = mNumGraphs = mNumGroups = 0; }
    };

    SCProcess(SuperLogger &logger);
    SCProcess();
    ~SCProcess();
    void quit();

    /* returns true if the server has booted / rebooted */
    bool setup(float sampleRate, int buffSize, int numInputs, int numOutput,
               juce::String pluginPath, juce::String synthdefPath);
    void reboot();
    void run(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages);
    bool unrollOSCPacket(int inSize, char *inData, OSC_Packet *inPacket);

    void setNodeValue(int nodeId, int idx, float value);

    // [ TO BE CALLED WITH WOLRDLOCK ]
    void setControlBusValue(int bus, float value);

    WorldStats getWorldStats() {
        const juce::GenericScopedTryLock<juce::CriticalSection> scopeLock(
            worldLock);
        WorldStats stats;
        if (scopeLock.isLocked()) {
            if (world != nullptr) {
                stats.mNumUnits = world->mNumUnits;
                stats.mNumGraphs = world->mNumGraphs;
                stats.mNumGroups = world->mNumGroups;
            }
        }
        return stats;
    }

    bool loadSynthdef(juce::MemoryBlock &block);

    void playSynth(juce::String name);
    void playSynthNote(juce::String name, int note, int velocity);
    void stopNode(int nodeId);

    void showSynthdef();
private:
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
