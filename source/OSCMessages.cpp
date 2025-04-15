/**
 *
 * Plugincollider Copyright (c) 2025 Pascal Gauthier.
 * Most of this file is part of JUCE; licensed on terms of the AGPLv3
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

#include "OSCMessages.h"

/**
 * This class was stipped from JUCE since it was not planned to be public.
 */
struct OSCOutputStream {
    OSCOutputStream(juce::MemoryBlock &block) : output(block, false) {
    }

    //==============================================================================
    bool writeInt32 (int32 value) {
        return output.writeIntBigEndian (value);
    }

    bool writeUint64 (uint64 value) {
        return output.writeInt64BigEndian (int64 (value));
    }

    bool writeFloat32 (float value) {
        return output.writeFloatBigEndian (value);
    }

    bool writeString (const juce::String& value) {
        if (! output.writeString (value))
            return false;

        const size_t numPaddingZeros = ~value.getNumBytesAsUTF8() & 3;

        return output.writeRepeatedByte ('\0', numPaddingZeros);
    }

    bool writeBlob (const juce::MemoryBlock& blob) {
        if (! (output.writeIntBigEndian ((int) blob.getSize())
                && output.write (blob.getData(), blob.getSize())))
            return false;

        const size_t numPaddingZeros = ~(blob.getSize() - 1) & 3;

        return output.writeRepeatedByte (0, numPaddingZeros);
    }

    bool writeColour (juce::OSCColour colour) {
        return output.writeIntBigEndian ((int32) colour.toInt32());
    }

    bool writeTimeTag (juce::OSCTimeTag timeTag) {
        return output.writeInt64BigEndian (int64 (timeTag.getRawTimeTag()));
    }

    bool writeAddress (const juce::OSCAddress& address) {
        return writeString (address.toString());
    }

    bool writeAddressPattern (const juce::OSCAddressPattern& ap) {
        return writeString (ap.toString());
    }

    bool writeTypeTagString (const juce::OSCTypeList& typeList) {
        output.writeByte (',');

        if (typeList.size() > 0)
            output.write (typeList.begin(), (size_t) typeList.size());

        output.writeByte ('\0');

        size_t bytesWritten = (size_t) typeList.size() + 1;
        size_t numPaddingZeros = ~bytesWritten & 0x03;

        return output.writeRepeatedByte ('\0', numPaddingZeros);
    }

    bool writeArgument (const juce::OSCArgument& arg) {
        switch (arg.getType()) {
            case 'i' : return writeInt32(arg.getInt32());
            case 'f' : return writeFloat32(arg.getFloat32());
            case 's' : return writeString(arg.getString());
            case 'b' : return writeBlob(arg.getBlob());
            case 'r' : return writeColour(arg.getColour());
            default:
                jassertfalse;
                return false;
        }
    }

    //==============================================================================
    bool writeMessage (const juce::OSCMessage& msg) {
        if (! writeAddressPattern (msg.getAddressPattern()))
            return false;

        juce::OSCTypeList typeList;

        for (auto& arg : msg)
            typeList.add (arg.getType());

        if (! writeTypeTagString (typeList))
            return false;

        for (auto& arg : msg)
            if (! writeArgument (arg))
                return false;

        return true;
    }

    bool writeBundle (const juce::OSCBundle& bundle) {
        if (! writeString ("#bundle"))
            return false;

        if (! writeTimeTag (bundle.getTimeTag()))
            return false;

        for (auto& element : bundle)
            if (! writeBundleElement (element))
                return false;

        return true;
    }

    //==============================================================================
    bool writeBundleElement (const juce::OSCBundle::Element& element) {
        const int64 startPos = output.getPosition();

        if (! writeInt32 (0))   // writing dummy value for element size
            return false;

        if (element.isBundle()) {
            if (! writeBundle (element.getBundle()))
                return false;
        } else {
            if (! writeMessage (element.getMessage()))
                return false;
        }

        const int64 endPos = output.getPosition();
        const int64 elementSize = endPos - (startPos + 4);

        return output.setPosition (startPos)
                    && writeInt32 ((int32) elementSize)
                    && output.setPosition (endPos);
    }

private:
    juce::MemoryOutputStream output;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSCOutputStream)
};

OSCMemoryBlock::OSCMemoryBlock(juce::OSCMessage &msg) {
    OSCOutputStream outStream(block);
    outStream.writeMessage(msg);
}

