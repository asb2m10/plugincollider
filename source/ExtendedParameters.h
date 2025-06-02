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

#include <juce_audio_processors/juce_audio_processors.h>

class PluginColliderRange : public juce::NormalisableRange<float> {
public:
    PluginColliderRange() = default;

    explicit PluginColliderRange(const juce::var v) {
        juce::StringArray token;
        token.addTokens(v.toString(), false);
        if ( token.size() == 3 ) {
            start = token[0].getFloatValue();
            end = token[1].getFloatValue();
            interval = token[2].getFloatValue();
        }
    }

    juce::var toVar() const {
        return juce::String(start) + " " +
               juce::String(end) + " " +
               juce::String(interval);
    }
};

class ParameterControlBus : public juce::AudioParameterFloat {
    juce::String name;
    juce::NormalisableRange<float> defaultRange;
    PluginColliderRange currentRange;
    juce::CriticalSection updateLock;
public:
    ParameterControlBus(int idx) : juce::AudioParameterFloat(
            juce::String("cb") + juce::String(idx), juce::String("Control Bus ") + juce::String(idx), 0, 1, 0.5) {
        this->name = juce::String("Control Bus ") + juce::String(idx);
    }

    juce::String getName(int maximumStringLength) const override {
        juce::ScopedTryLock lock(updateLock);
        if ( lock.isLocked() )
            return name.substring(0, maximumStringLength);
        return "Updating...";
    }

    void setName(const juce::String &newName) {
        juce::ScopedLock lock(updateLock);
        name = newName;
    }

    void setRange(PluginColliderRange range) {
        juce::ScopedLock lock(updateLock);
        currentRange = range;
    }

    float getRangedValue(float dawValue) {
        juce::ScopedTryLock lock(updateLock);
        if ( lock.isLocked() ) {
            return currentRange.convertFrom0to1(dawValue);
        }
        return 0.5;
    }

    const juce::NormalisableRange<float>& getNormalisableRange() const override {
        juce::ScopedTryLock lock(updateLock);
        if ( lock.isLocked() ) {
            return currentRange;
        }
        return defaultRange;
    }
};
