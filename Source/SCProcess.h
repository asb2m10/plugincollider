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

#ifndef _SCProcess_
#define _SCProcess_

#include "SC_WorldOptions.h"
#include "SC_World.h"
#include "SC_HiddenWorld.h"
#include "SC_CoreAudio.h"

#include "OSCMessages.h"
#include "UDPPort.h"

#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>

#include "SCPluginAPI.h"

#include <juce_audio_processors/juce_audio_processors.h>

class SCProcess {

public:
	SCProcess();
    ~SCProcess();
    string synthName;
    int portNum;
	void startUp(WorldOptions options, string pluginsPath, string synthdefsPath, int preferredPort);
	void makeSynth();
	void sendParamChangeMessage(string name, float value);
	void sendNote(int64 oscTime, int note, int velocity);
    void sendTick(int64 oscTime, int bus);
    void freeAll();
    void quit();
    void run(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, double sampleRate);

private:
    World* world;
    int findNextFreeUdpPort(int startNum);
    UDPPort* mPort;
    /*AudioBufferList *input;
    AudioBufferList *output;*/
};

#endif
