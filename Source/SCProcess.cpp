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

const int kDefaultPortNumber = 9989;
const int kDefaultBlockSize = 64;
const int kDefaultBeatDiv = 1;
const int kDefaultNumWireBufs = 64;
const int kDefaultRtMemorySize = 8192;

#ifdef __APPLE__
   const juce::File DEFAULT_PLUGIN_PATH("/Applications/SuperCollider.app/Contents/Resources/plugins");
#elif __unix__
   const juce::File DEFAULT_PLUGIN_PATH("/usr/lib/SuperCollider/plugins");
#endif

void null_reply_func(struct ReplyAddress* /*addr*/, char* /*msg*/, int /*size*/);

SCProcess::SCProcess() : world(nullptr)
{
    portNum = 0;
}

SCProcess::~SCProcess() {
    const juce::ScopedLock lock(worldLock);    
    if(world){
      World_Cleanup(world, true);
      mPort->stopAsioThread();
      free(mPort);
    }
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

void SCProcess::setup(float sampleRate, int buffSize, int numInputs, int numOutputs, juce::File *plugin, juce::File *synth) {
    
    // avoid restarting server if the settings are the same
    if ( world != nullptr ) {
        bool same = true;

        same &= sampleRate == world->mSampleRate;
        same &= buffSize == world->mBufLength;
        same &= numInputs == world->mNumInputs;
        same &= numOutputs == world->mNumOutputs;

        if ( same )
            return;
    }

    const juce::ScopedLock lock(worldLock);

	OSCMessages messages;
    std::string bindTo("0.0.0.0");

    if ( world == nullptr ) 
        World_Cleanup(world, false);

    WorldOptions options;
    options.mPreferredSampleRate = sampleRate;
    options.mBufLength = buffSize;
    options.mPreferredHardwareBufferFrameSize = buffSize;
    options.mMaxWireBufs = kDefaultNumWireBufs;
    options.mRealTimeMemorySize = kDefaultRtMemorySize;
    options.mNumBuffers = 8192;
    options.mNumInputBusChannels = 2;
    options.mNumOutputBusChannels = 2;
    options.mVerbosity = 2;

    setenv("SC_PLUGIN_PATH", DEFAULT_PLUGIN_PATH.getFullPathName().toRawUTF8(), 1);
    //setenv("SC_SYNTHDEF_PATH", synthdefsPath.c_str(), 1);

    this->portNum = findNextFreeUdpPort(8898);

    world = World_New(&options);
    world->mDumpOSC=2;

    if (world) {
        if (this->portNum >= 0) mPort = new UDPPort(world,  bindTo.c_str(), this->portNum);
        //if (this->portNum >= 0) World_OpenUDP(world,  bindTo.c_str(), this->portNum);
        small_scpacket packet = messages.initTreeMessage();
        World_SendPacket(world, 16, (char*)packet.buf, null_reply_func);
    }

    scprintf("*******************************************************\n");
    scprintf("PluginCollider Initialized \n");
    scprintf("PluginCollider mPreferredHardwareBufferFrameSize: %d \n",options.mPreferredHardwareBufferFrameSize );
    scprintf("PluginCollider mBufLength: %d \n",options.mBufLength );
    scprintf("PluginCollider  port: %d \n", portNum );
    scprintf("PluginCollider  mMaxWireBufs: %d \n", options.mMaxWireBufs );
    scprintf("PluginCollider  mRealTimeMemorySize: %d \n", options.mRealTimeMemorySize );
    scprintf("PluginCollider  mNumOutputBusChannels %d \n", options.mNumOutputBusChannels );
	scprintf("*******************************************************\n");
    fflush(stdout);
}


void SCProcess::run(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages){
    if (world->mRunning){
        SC_PluginAudioDriver* driver = (SC_PluginAudioDriver*)this->world->hw->mAudioDriver;
        driver->callback(buffer, midiMessages);
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



void SCProcess::quit(){
}


void SCProcess::startUp(WorldOptions options, string pluginsPath, string synthdefsPath, int preferredPort){
    char stringBuffer[PATH_MAX] ;
	OSCMessages messages;
    std::string bindTo("0.0.0.0");

    setenv("SC_PLUGIN_PATH", pluginsPath.c_str(), 1);
    setenv("SC_SYNTHDEF_PATH", synthdefsPath.c_str(), 1);
    this->portNum = findNextFreeUdpPort(preferredPort);

    if ( world != nullptr ) 
        World_Cleanup(world, false);

    world = World_New(&options);
    world->mDumpOSC=2;

    if (world) {
        if (this->portNum >= 0) mPort = new UDPPort(world,  bindTo.c_str(), this->portNum);
        //if (this->portNum >= 0) World_OpenUDP(world,  bindTo.c_str(), this->portNum);
        small_scpacket packet = messages.initTreeMessage();
        World_SendPacket(world, 16, (char*)packet.buf, null_reply_func);
      }
}
