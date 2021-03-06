/*
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
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */

#pragma once

#include "SC_CoreAudio.h"
#include "SC_HiddenWorld.h"
#include "SC_World.h"
#include "SC_WorldOptions.h"

#include "OSCMessages.h"
#include "UDPPort.h"

#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

#include "SCPluginDriver.h"

#include <juce_audio_processors/juce_audio_processors.h>

class UDPPort;

class SCProcess {
    juce::File synthPath;
    juce::File pluginPath;

  public:
    struct WorldStats {
        uint32 mNumUnits, mNumGraphs, mNumGroups;
        WorldStats() { mNumUnits = mNumGraphs = mNumGroups = 0; }
    };

    SCProcess();
    ~SCProcess();
    void quit();
    void setup(float sampleRate, int buffSize, int numInputs, int numOutput,
               juce::File *plugin, juce::File *synthDef);
    void run(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages);
    bool unrollOSCPacket(int inSize, char *inData, OSC_Packet *inPacket);
    int portNum;

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

  private:
    World *world;
    juce::CriticalSection worldLock;

    int findNextFreeUdpPort(int startNum);
    UDPPort *mPort;

    // ATTIC
    // ---
    string synthName;
    void startUp(WorldOptions options, string pluginsPath, string synthdefsPath,
                 int preferredPort);
    void makeSynth();
    void sendParamChangeMessage(string name, float value);
    void sendNote(int64 oscTime, int note, int velocity);
    void sendTick(int64 oscTime, int bus);
};
