/*
    PluginCollider Copyright (c) 2025 Pascal Gauthier.

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
#include <juce_core/juce_core.h>

#define IDS_VERSION "A"
const int NUMBER_OF_CONTROL_BUSES = 32;

namespace IDs {
#define DECLARE_ID(name) const juce::Identifier name (#name);
    DECLARE_ID(root)
    DECLARE_ID(version)
    DECLARE_ID(udpport)
    DECLARE_ID(controlbuses)
    DECLARE_ID(controlbus)
    DECLARE_ID(cbName)
    DECLARE_ID(cbRange)
    DECLARE_ID(cbIdx)

    DECLARE_ID(rootnode)
    DECLARE_ID(fxnode)
    DECLARE_ID(groupnode)
    DECLARE_ID(notenode)
    DECLARE_ID(nodename)
    DECLARE_ID(nodeid)
    DECLARE_ID(nodeCount)

    DECLARE_ID(synthName)
    DECLARE_ID(synthBlob)
    DECLARE_ID(synthNoteRange)
    DECLARE_ID(synthMono)

    DECLARE_ID(parameters)
    DECLARE_ID(parameter)

    DECLARE_ID(pName)
    DECLARE_ID(pIdx)
    DECLARE_ID(pCurrentValue)
    DECLARE_ID(pDefaultValue)
    DECLARE_ID(pControlBus)
    DECLARE_ID(pRange)

    DECLARE_ID(scratchpad)
    DECLARE_ID(spCode)
};

