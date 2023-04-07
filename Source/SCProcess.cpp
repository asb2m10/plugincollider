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
#include "SC_OscUtils.hpp"
#include "SC_Prototypes.h"
#include "SC_StringParser.h"
#include "SC_WorldOptions.h"
#include "sc_msg_iter.h"

const int kDefaultPortNumber = 9989;
const int kDefaultBlockSize = 64;
const int kDefaultBeatDiv = 1;
const int kDefaultNumWireBufs = 64;
const int kDefaultRtMemorySize = 8192;

#ifdef WIN32
     #define close closesocket
#endif

void null_reply_func(struct ReplyAddress * /*addr*/, char * /*msg*/,
                     int /*size*/);
int scprocess_scprintf(const char *format, va_list ap);

///// from SC_ComPort.cpp ///////////
bool ProcessOSCPacket(World *inWorld, OSC_Packet *inPacket);

SCProcess::SCProcess() {
    SetPrintFunc(scprocess_scprintf);

    world = nullptr;
    portNum = 0;
}

SCProcess::~SCProcess() {
    const juce::ScopedLock lock(worldLock);
    if (world) {
        World_Cleanup(world, true);
        mPort->stopAsioThread();
        free(mPort);
    }
}

int SCProcess::findNextFreeUdpPort(int startNum) {
    int server_socket = -1;
    struct sockaddr_in mBindSockAddr;
    int numberOfTries = 100;
    int port = startNum;
    if ((server_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        scprintf("failed to create udp socket\n");
        return -1;
    }

    memset(&mBindSockAddr, 0, sizeof(mBindSockAddr));
    // bzero((char *)&mBindSockAddr, );
    mBindSockAddr.sin_family = AF_INET;
    mBindSockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    mBindSockAddr.sin_port = htons(port);
    const char on = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    while (bind(server_socket, (struct sockaddr *)&mBindSockAddr,
                sizeof(mBindSockAddr)) < 0) {
        if (--numberOfTries < 0 || (errno != EADDRINUSE)) {
            scprintf("unable to bind udp socket\n");
            return -1;
        }
        port++;
        mBindSockAddr.sin_port = htons(port);
    }

    close(server_socket);

    return port;
}

void SCProcess::setup(float sampleRate, int buffSize, int numInputs,
                      int numOutputs, int udpPort) {

    // avoid restarting server if the settings are the same
    if (world != nullptr) {
        bool same = true;

        same &= sampleRate == world->mSampleRate;
        same &= buffSize == world->mBufLength;
        same &= numInputs == world->mNumInputs;
        same &= numOutputs == world->mNumOutputs;

        if (same)
            return;
    }

    if (portNum == 0) {
        portNum = findNextFreeUdpPort(udpPort);
        if (this->portNum >= 0) {
            std::string bindTo("0.0.0.0");
            mPort = new UDPPort(this, bindTo.c_str(), this->portNum);
        }
    }

    const juce::ScopedLock lock(worldLock);

    if (world != nullptr)
        World_Cleanup(world, false);

    WorldOptions options;
    options.mPreferredSampleRate = sampleRate;
    options.mBufLength = buffSize;
    options.mPreferredHardwareBufferFrameSize = buffSize;
    options.mMaxWireBufs = kDefaultNumWireBufs;
    options.mRealTimeMemorySize = kDefaultRtMemorySize;
    options.mNumBuffers = 8192;
    options.mNumInputBusChannels = numInputs;
    options.mNumOutputBusChannels = numOutputs;
    options.mVerbosity = 2;

    world = World_New(&options);
    world->mDumpOSC = 2;

    if (world) {
        OSCMessages messages;
        small_scpacket packet = messages.initTreeMessage();
        World_SendPacket(world, 16, (char *)packet.buf, null_reply_func);
        scprintf("*******************************************************\n");
        scprintf("PluginCollider Initialized \n");
        scprintf("PluginCollider mPreferredHardwareBufferFrameSize: %d \n",
                 options.mPreferredHardwareBufferFrameSize);
        scprintf("PluginCollider mBufLength: %d \n", options.mBufLength);
        scprintf("PluginCollider  port: %d \n", portNum);
        scprintf("PluginCollider  mMaxWireBufs: %d \n", options.mMaxWireBufs);
        scprintf("PluginCollider  mRealTimeMemorySize: %d \n",
                 options.mRealTimeMemorySize);
        scprintf("PluginCollider  mNumInputBusChannels %d \n",
                 options.mNumInputBusChannels);
        scprintf("PluginCollider  mNumOutputBusChannels %d \n",
                 options.mNumOutputBusChannels);
        scprintf("*******************************************************\n");
    } else {
        scprintf("Unable to initiale world\n");
    }
}

bool SCProcess::unrollOSCPacket(int inSize, char *inData,
                                OSC_Packet *inPacket) {
    const juce::ScopedTryLock lock(worldLock);

    if (!lock.isLocked())
        return false;

    if (!strcmp(inData, "#bundle")) { // is a bundle
        char *data;
        char *dataEnd = inData + inSize;
        int len = 16;
        bool hasNestedBundle = false;

        // get len of nested messages only, without len of nested bundle(s)
        data = inData + 16; // skip bundle header
        while (data < dataEnd) {
            int32 msgSize = OSCint(data);
            data += sizeof(int32);
            if (strcmp(data, "#bundle")) // is a message
                len += sizeof(int32) + msgSize;
            else
                hasNestedBundle = true;
            data += msgSize;
        }

        if (hasNestedBundle) {
            if (len > 16) { // not an empty bundle
                // add nested messages to bundle buffer
                char *buf = (char *)malloc(len);
                inPacket->mSize = len;
                inPacket->mData = buf;

                memcpy(buf, inData, 16); // copy bundle header
                data = inData + 16;      // skip bundle header
                while (data < dataEnd) {
                    int32 msgSize = OSCint(data);
                    data += sizeof(int32);
                    if (strcmp(data, "#bundle")) { // is a message
                        memcpy(buf, data - sizeof(int32),
                               sizeof(int32) + msgSize);
                        buf += msgSize;
                    }
                    data += msgSize;
                }

                // process this packet without its nested bundle(s)
                if (!ProcessOSCPacket(world, inPacket)) {
                    free(buf);
                    return false;
                }
            }

            // process nested bundle(s)
            data = inData + 16; // skip bundle header
            while (data < dataEnd) {
                int32 msgSize = OSCint(data);
                data += sizeof(int32);
                if (!strcmp(data, "#bundle")) { // is a bundle
                    OSC_Packet *packet =
                        (OSC_Packet *)malloc(sizeof(OSC_Packet));
                    memcpy(packet, inPacket,
                           sizeof(OSC_Packet)); // clone inPacket

                    if (!unrollOSCPacket(msgSize, data, packet)) {
                        free(packet);
                        return false;
                    }
                }
                data += msgSize;
            }
        } else { // !hasNestedBundle
            char *buf = (char *)malloc(inSize);
            inPacket->mSize = inSize;
            inPacket->mData = buf;
            memcpy(buf, inData, inSize);

            if (!ProcessOSCPacket(world, inPacket)) {
                free(buf);
                return false;
            }
        }
    } else { // is a message
        char *buf = (char *)malloc(inSize);
        inPacket->mSize = inSize;
        inPacket->mData = buf;
        memcpy(buf, inData, inSize);

        if (!ProcessOSCPacket(world, inPacket)) {
            free(buf);
            return false;
        }
    }

    return true;
}

void SCProcess::run(juce::AudioBuffer<float> &buffer,
                    juce::MidiBuffer &midiMessages) {
    if (world->mRunning) {
        SC_PluginAudioDriver *driver =
            (SC_PluginAudioDriver *)this->world->hw->mAudioDriver;
        driver->callback(buffer, midiMessages);
    }
}

void SCProcess::makeSynth() {
    if (world->mRunning) {
        OSCMessages messages;
        small_scpacket packet;
        size_t messageSize = messages.createSynthMessage(&packet, synthName);
        World_SendPacket(world, messageSize, (char *)packet.buf,
                         null_reply_func);
    }
}

void SCProcess::sendParamChangeMessage(string name, float value) {
    OSCMessages messages;
    //    if(synthName.){
    small_scpacket packet;
    size_t messageSize = messages.parameterMessage(&packet, name, value);
    World_SendPacket(world, messageSize, (char *)packet.buf, null_reply_func);
    //    }
}

void SCProcess::sendTick(int64 oscTime, int bus) {
    OSCMessages messages;
    small_scpacket packet = messages.sendTickMessage(oscTime, bus);
    World_SendPacket(world, 40, (char *)packet.buf, null_reply_func);
}

void SCProcess::sendNote(int64 oscTime, int note, int velocity) {
    OSCMessages messages;
    small_scpacket packet = messages.noteMessage(oscTime, note, velocity);
    World_SendPacket(world, 92, (char *)packet.buf, null_reply_func);
}

void SCProcess::quit() {}

int scprocess_scprintf(const char *fmt, va_list ap) {
    char buf[4096];
    int p = vsnprintf(buf, sizeof(buf), fmt, ap);
    printf("%s", buf);
    juce::Logger::writeToLog(string(buf));
    return p;
}