/*
    PluginCollider Copyright (c) 2021 Pascal Gauthier.
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

#include "SCProcess.h"
#include "SC_CoreAudio.h"
#include "SC_HiddenWorld.h"
#include "SC_Prototypes.h"
#include "SC_StringParser.h"
#include "SC_WorldOptions.h"
#include "SC_TimeDLL.hpp"
#include "SC_Time.hpp"
#include "SC_AU.h"

void null_reply_func(struct ReplyAddress* /*addr*/, char* /*msg*/, int /*size*/);

SCProcess::SCProcess() : world(nullptr)
{
    portNum = 0;
}

SCProcess::~SCProcess() {
}

int SCProcess::findNextFreeUdpPort(int startNum){
	int server_socket = -1;
	struct sockaddr_in mBindSockAddr;
	int numberOfTries = 100;
	int port = startNum;
	if ((server_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		scprintf("failed to create udp socket\n");
		return -1;
	}

	bzero((char *)&mBindSockAddr, sizeof(mBindSockAddr));
	mBindSockAddr.sin_family = AF_INET;
	mBindSockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	mBindSockAddr.sin_port = htons(port);
	const int on = 1;
	setsockopt( server_socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	while (bind(server_socket, (struct sockaddr *)&mBindSockAddr, sizeof(mBindSockAddr)) < 0) {
		if(--numberOfTries <0 || (errno != EADDRINUSE)) {
			scprintf("unable to bind udp socket\n");
			return -1;
		}
        port++;
        mBindSockAddr.sin_port = htons(port);
	}
	close(server_socket);
	return port;
}

void SCProcess::startUp(WorldOptions options, string pluginsPath, string synthdefsPath, int preferredPort){
    char stringBuffer[PATH_MAX] ;
	OSCMessages messages;
    std::string bindTo("0.0.0.0");

    setenv("SC_PLUGIN_PATH", pluginsPath.c_str(), 1);
    setenv("SC_SYNTHDEF_PATH", synthdefsPath.c_str(), 1);
    this->portNum = findNextFreeUdpPort(preferredPort);
    world = World_New(&options);
    world->mDumpOSC=2;

    if (world) {
        if (this->portNum >= 0) mPort = new UDPPort(world,  bindTo.c_str(), this->portNum);
        //if (this->portNum >= 0) World_OpenUDP(world,  bindTo.c_str(), this->portNum);
        small_scpacket packet = messages.initTreeMessage();
        World_SendPacket(world, 16, (char*)packet.buf, null_reply_func);
      }
}


void SCProcess::makeSynth(){
    if (world->mRunning){
        OSCMessages messages;
        small_scpacket packet;
        size_t messageSize =  messages.createSynthMessage(&packet, synthName);
        World_SendPacket(world, messageSize, (char*)packet.buf, null_reply_func);
    }
}

void SCProcess::sendParamChangeMessage(string name, float value){
    OSCMessages messages;
//    if(synthName.){
        small_scpacket packet;
        size_t messageSize = messages.parameterMessage(&packet, name,value);
        World_SendPacket(world, messageSize, (char*)packet.buf, null_reply_func);
//    }
}

void SCProcess::sendTick(int64 oscTime, int bus){
    OSCMessages messages;
    small_scpacket packet = messages.sendTickMessage(oscTime, bus);
    World_SendPacket(world, 40, (char*)packet.buf, null_reply_func);

}

void SCProcess::sendNote(int64 oscTime, int note, int velocity){
    OSCMessages messages;
    small_scpacket packet = messages.noteMessage(oscTime, note, velocity);
    World_SendPacket(world, 92, (char*)packet.buf, null_reply_func);
}


void SCProcess::run(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, double sampleRate){
    if (world->mRunning){
        SC_PluginAudioDriver* driver = (SC_PluginAudioDriver*)this->world->hw->mAudioDriver;
        driver->callback(buffer, midiMessages);
    }
}

void SCProcess::freeAll() {
    OSCMessages messages;
    small_scpacket packet = messages.freeAllMessage();
    World_SendPacket(world, packet.size(), (char*)packet.buf, null_reply_func);
    scprintf("free\n");
}

void SCProcess::quit(){
    if(world){
      World_Cleanup(world, true);
      mPort->stopAsioThread();
      free(mPort);
    }
}
