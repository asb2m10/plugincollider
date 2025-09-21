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


#include "SCProcess.h"
#include "SC_CoreAudio.h"
#include "SC_HiddenWorld.h"
#include "SC_OscUtils.hpp"
#include "SC_Prototypes.h"
#include "SC_StringParser.h"
#include "SC_WorldOptions.h"
#include "sc_msg_iter.h"
#include "SC_PlugIn.h"
#include "SC_GraphDef.h"
#include "SC_Group.h"
#include "SC_UnitDef.h"
#include <stdlib.h>

#include "NodeContainer.h"

const int kDefaultNumWireBufs = 64;
const int kDefaultRtMemorySize = 8192;

// Needed to load UIUGenJUCE in static plugins
extern void UIUGens_Load(InterfaceTable* table);
extern InterfaceTable gInterfaceTable;

void null_reply_func(struct ReplyAddress * /*addr*/, char * /*msg*/,
                     int /*size*/);
int scprocess_scprintf(const char *format, va_list ap);

///// from SC_ComPort.cpp ///////////
bool ProcessOSCPacket(World *inWorld, OSC_Packet *inPacket);

/**
 * @brief Helper Stream class to read Pascal Strings
 *
 */
class MemoryInputPStream : public juce::MemoryInputStream {
public:
    MemoryInputPStream(const juce::MemoryBlock& data) : MemoryInputStream(data, false) {
    }

    juce::String readPString() {
        int sz = readByte();
        if ( sz > getNumBytesRemaining() )
            jassertfalse;
        char *c = ((char *)getData() + getPosition());
        skipNextBytes(sz);
        return juce::String(c, sz);
    }
};

SynthDef::SynthDef(juce::MemoryBlock &newContent) {
    if (newContent.getSize() < 0)
        throw InvalidSynthDef("MemoryBlock empty");
    MemoryInputPStream stream(newContent);

    // Check header
    if (stream.readIntBigEndian() != (('S' << 24) | ('C' << 16) | ('g' << 8) | 'f') /*'SCgf'*/) {
        scprintf("Invalid SynthDef header\n");
        throw InvalidSynthDef("Invalid SyntDef header");
    }

    // SynthDef version
    int version = stream.readIntBigEndian();
    if ( version != 2 ) {
        scprintf("SynthDef version %d not supported\n", version);
        throw InvalidSynthDef("SynthDef version not supported");
    }
    // number of synth definition in file
    stream.readShortBigEndian();

    memoryBlock = newContent;
    name = stream.readPString();

    /* number of constant */
    int numConstant = stream.readIntBigEndian();
    stream.skipNextBytes(numConstant * 4);

    /* number of parameters values */
    int numParametersValues = stream.readIntBigEndian();
    jassert(numParametersValues<256);

    parametersValues.reset(new float[numParametersValues]);
    for(int i=0;i<numParametersValues;i++) {
        parametersValues[i] = stream.readFloatBigEndian();
    }

    /* number of parameters names */
    int numParameters = stream.readIntBigEndian();
    jassert(numParameters<256);

    int lastParameterPos = 0;
    for(int i=0;i<numParameters;i++) {
        juce::String currentName = stream.readPString();
        int pos = stream.readIntBigEndian();
        if ( lastParameterPos != pos ) {
            // It was an array parameter, remove it and reinsert it with the array parameter postfix
            juce::String arrayname = parameters[parameters.size()-1];
            parameters.remove(parameters.size()-1);

            int items = pos - lastParameterPos + 1;
            for (int i=0;i<items;i++) {
                parameters.add(arrayname + "[" + juce::String(i+1) + "]");
            }
        }
        lastParameterPos = pos + 1;
        parameters.add(currentName);
    }

    // We still have to check if the last parameter name was an array
    if ( lastParameterPos != numParametersValues ) {
        juce::String name = parameters[parameters.size()-1];
        parameters.remove(parameters.size()-1);

        int items = numParametersValues - lastParameterPos + 1;
        for (int i=0;i<items;i++) {
            parameters.add(name + "[" + juce::String(i+1) + "]");
        }
    }
}

SCProcess::SCProcess(SuperLogger &logger) : logger(logger) {
    SetPrintFunc(scprocess_scprintf);
    world = nullptr;
}

SCProcess::~SCProcess() {
    const juce::ScopedLock lock(worldLock);
    if (world) {
        World_Cleanup(world, false);
    }
    world = nullptr;
}

bool SCProcess::setup(float sampleRate, int buffSize, int numInputs, int numOutputs) {
    // avoid restarting server if the settings are the same
    if (world != nullptr) {
        bool same = true;

        same &= sampleRate == world->mSampleRate;
        same &= buffSize == world->mBufLength;
        same &= numInputs == world->mNumInputs;
        same &= numOutputs == world->mNumOutputs;
        if (same)
            return false;
    }

    if ( ! juce::isPowerOfTwo(buffSize) ) {
        logger.scprintf("Warning: your DAW latency settings is not based on the power of two. Some SC plugins might not work properly.\n");
    }

    this->sampleRate = sampleRate;
    bufferSize = buffSize;
    this->numInputs = numInputs;
    this->numOutputs = numOutputs;

    bootServer();
    return true;
}

void SCProcess::reboot() {
    if ( world == nullptr )
        return;
    bootServer();
}

void SCProcess::bootServer() {
    const juce::ScopedLock lock(worldLock);

    if (world != nullptr) {
        World_Cleanup(world, false);
    }

    logger.scprintf("*************** SuperCollider booting ***************\n");

    WorldOptions options;
    options.mPreferredSampleRate = sampleRate;
    options.mBufLength = bufferSize;
    options.mPreferredHardwareBufferFrameSize = bufferSize;
    options.mMaxWireBufs = kDefaultNumWireBufs;
    options.mRealTimeMemorySize = kDefaultRtMemorySize;
    options.mNumBuffers = 8192;
    options.mNumInputBusChannels = numInputs;
    options.mNumOutputBusChannels = numOutputs;
    options.mVerbosity = 2;
    options.mMaxLogins = 32;
#if STATIC_PLUGINS
    logger.scprintf("SC_PLUGIN_PATH is ignored since PluginCollider is compiled with SC static plugins\n");
#else
    options.mUGensPluginPath = pluginPath.toRawUTF8();
#endif

    // For now the only way to set SynthDefs path
    if (! synthDefPath.isEmpty() ) {
#ifdef WIN32
        _putenv_s("SC_SYNTHDEF_PATH", synthDefPath.toRawUTF8());
#else
        setenv("SC_SYNTHDEF_PATH", synthDefPath.toRawUTF8(), 1);
#endif
    }

    world = World_New(&options);

    if (world) {
        world->mDumpOSC = 0;
        rt_newGroup(0, kDefaultGroupId);

#if STATIC_PLUGINS
        UIUGens_Load(&gInterfaceTable);
#endif
        logger.scprintf("WorldOptions: BufLength(%d) MaxWireBufs(%d) RealTimeMemorySize(%d) "
                 "mNumInputBusChannels(%d) mNumOutputBusChannels(%d)\n",
                options.mBufLength, options.mMaxWireBufs, options.mRealTimeMemorySize,
                options.mNumInputBusChannels, options.mNumOutputBusChannels);
        logger.scprintf("*************** SuperCollider boot success ***************\n");
    } else {
        logger.scprintf("*************** SuperCollider boot failed ***************\n");
    }
}

void SCProcess::rt_getSynthDef(HeapStringList<64,4096> &list) {
    for (int i=0;i<world->hw->mGraphDefLib->TableSize();i++) {
        GraphDef *gf = world->hw->mGraphDefLib->AtIndex(i);
        if ( gf != nullptr ) {
            if  ( strncmp("system_", (const char*) gf->mNodeDef.mName, 6) ) {
                list.add((const char *) gf->mNodeDef.mName);
            }
        }
    }
}

void SCProcess::run(juce::AudioBuffer<float> &buffer,
                    juce::MidiBuffer &midiMessages) {
    if (world->mRunning) {
        SC_PluginAudioDriver *driver =
            (SC_PluginAudioDriver *)this->world->hw->mAudioDriver;
        driver->callback(buffer, midiMessages);
    }
}

void SCProcess::freeNodes(int rootNodeId) {
    if (world->mRunning) {
        juce::OSCMessage msg("/g_freeAll", rootNodeId);
        OSCMemoryBlock block(msg);
        World_SendPacket(world, block.getSize(), block.getData(), null_reply_func);
    }
}

SCNodeWalker SCProcess::rt_getNode(int destNode) {
    if ( world == nullptr )
        throw std::invalid_argument("SC World is null");
    return SCNodeWalker(World_GetNode(world, destNode));
}

void SCProcess::rt_freeGroup(int rootGroup) {
    SCNodeWalker node = rt_getNode(rootGroup);
    if ( node.isValid() && node.isGroup() ) {
        Group_DeleteAll(node.group());
    }
}

void SCProcess::rt_freeNode(int destNode) {
    SCNodeWalker node = rt_getNode(destNode);
    if ( node.isValid() )
        Node_Delete(node.node());
}

void SCProcess::rt_setNodeValue(int destNode, int idx, float value) {
    Node_SetControl(rt_getNode(destNode).node(), idx, value);
}

SCErr SCProcess::rt_newGroup(int parentNode, int destGroup) {
    Group *parent = rt_getNode(parentNode).group();
    Group* newGroup = nullptr;
    SCErr err = Group_New(world, destGroup, &newGroup);
    if (err) {
        if (err == kSCErr_DuplicateNodeID) {
            newGroup = World_GetGroup(world, destGroup);
            if (!newGroup || !newGroup->mNode.mParent || newGroup->mNode.mParent != parent)
                return err;
        } else
            return err;
    } else {
        Group_AddTail(parent, &newGroup->mNode);
    }
    return 0;
}

bool SCProcess::rt_loadSynthDef(juce::MemoryBlock *block) {
    GraphDef *inList = GraphDef_Recv(world, (char *) block->getData(), nullptr);
    if ( inList != nullptr ) {
        GraphDef_Define(world, inList);
        return true;
    }
    return false;
}

void SCProcess::rt_setControlBusValue(int bus, float value) {
    if ( bus < 0 || bus >= world->mNumControlBusChannels ) {
        logger.scprintf("Invalid control bus %d; available %d\n", bus,world->mNumControlBusChannels);
        return;
    }
    world->mControlBusTouched[bus] = world->mBufCounter;
    world->mControlBus[bus] = value;
}

void SCProcess::rt_assignControlBus(int nodeId, int nodeParamIdx, int busIdx) {
    Node *node = rt_getNode(nodeId).node();
    if ( node == nullptr ) {
        logger.scprintf("Invalid node %d\n", nodeId);
        return;
    }
    if ( busIdx < 0 || busIdx >= world->mNumControlBusChannels ) {
        logger.scprintf("Invalid control bus %d; available %d\n", busIdx ,world->mNumControlBusChannels);
    } else {
        Node_MapControl(node, nodeParamIdx, busIdx);
    }
}

void SCProcess::rt_dumpTree() {
    Group_DumpTreeAndControls(rt_getNode(0).group());
}

int32_t SCProcess::rt_newSynth(int *hashname, int newId, int destNode) {
    GraphDef* def = World_GetGraphDef(world, hashname);
    if ( def == nullptr ) {
        logger.scprintf("Syntdef not found: %s\n", (char *) hashname);
        return 0;
    }

    // we create a empty message, we will reconfigure the node afterward
    sc_msg_iter msg(0, "");

    Graph* graph = nullptr;
    int err = Graph_New(world, def, newId, &msg, &graph, true);

    if ( err ) {
        logger.scprintf("Unable to create instance\n");
        return 0;
    }

    if ( destNode != 0 ) {
        Group_AddTail(rt_getNode(destNode).group(), &graph->mNode);
    }

    return graph->mNode.mID;
}

extern HashTable<struct UnitDef, Malloc>* gUnitDefLib;
juce::StringArray SCProcess::getRegistredUnits() {
    juce::StringArray ret;
    if ( world != nullptr ) {
        for(int i=0;i<gUnitDefLib->TableSize();i++) {
            UnitDef *unit = gUnitDefLib->AtIndex(i);
            if ( unit != nullptr ) {
                ret.add((char *) unit->mUnitDefName);
            }
        }
    }
    return ret;
}

SCErr SCProcess::rt_queryTree(int rootGroup, big_scpacket *packet, bool flagParameters) {
    Group *group = rt_getNode(rootGroup).group();
    if (group == nullptr) {
        return kSCErr_GroupNotFound;
    }
    packet->adds("/reply");
    if ( flagParameters ) {
        // first count the total number of nodes to know how many tags the packet should have
        int numNodes = 1; // include this one
        int numControlsAndDefs = 0;
        Group_CountNodeAndControlTags(group, &numNodes, &numControlsAndDefs);
        // nodeID and numChildren + numControlsAndDefs + controlFlag
        packet->maketags(numNodes * 2 + numControlsAndDefs + 2);
        packet->addtag(',');
        packet->addtag('i');
        packet->addi(1); // include controls flag
        Group_QueryTreeAndControls(group, packet);
    } else {
        int numNodeTags = 2; // include this one
        Group_CountNodeTags(group, &numNodeTags);
        packet->maketags(numNodeTags + 2); // nodeID and numChildren
        packet->addtag(',');
        packet->addtag('i');
        packet->addi(0); // include controls flag
        Group_QueryTree(group, packet);
    }
    return 0;
}

void SCProcess::quit() {
    // NO-UP since we dont want the plugin to close
}

int scprocess_scprintf(const char *fmt, va_list ap) {
    char buf[4096];
    int p = vsnprintf(buf, sizeof(buf), fmt, ap);
    printf(buf);
    juce::Logger::writeToLog(juce::String(buf));
    return p;
}

// This is copied from SC source code since it is not made public
bool SCProcess::unrollOSCPacket(int inSize, char *inData, OSC_Packet *inPacket) {
    const juce::ScopedTryLock lock(worldLock);

    if (!lock.isLocked())
        return true;

    if (world == nullptr)
        return true;

    if (!world->mRunning)
        return true;

    if (world->mDumpOSC)
        dumpOSC(world->mDumpOSC, inSize, inData);

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
