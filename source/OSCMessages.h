/**
 *
 * Plugincollider Copyright (c)2025 Pascal Gauthier.
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
#include <juce_osc/juce_osc.h>
#include "SCProcess.h"
#include "SC_Types.h"

//static int kDefaultNodeId = 1000;
static int kDefaultGroupId = 1;


class OSCMemoryBlock {
    juce::MemoryBlock block;
public:
    OSCMemoryBlock(juce::OSCMessage &msg);
    char *getData() {
        return (char *) block.getData();
    }
    int getSize() {
        return block.getSize();
    }

    static juce::OSCMessage parseMessage(const void* sourceData, size_t sourceDataSize);
};

juce::String argument2String(juce::OSCArgument *arg);

class OSCArgumentWalker {
    juce::OSCArgument *args;
    juce::OSCArgument *ends;
public:
    OSCArgumentWalker(juce::OSCMessage &msg) {
        args = msg.begin();
        ends = msg.end();
    }

    juce::String getString() {
        return args->getString();
    }

    int getInt() {
        return args->getInt32();
    }

    float getFloat() {
        return args->getFloat32();
    }

    void operator++() {
        if ( args != ends ) {
            args++;
        } else {
            scprintf("Warning: unexpected end of argument from OSC message\n");
        }
    }
};
