/*
    PluginCollider Copyright (c) 2021 Pascal Gauthier.    
    Copyright (c) 2002 James McCartney. All rights reserved.
    http://www.audiosynth.com

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
#include "SC_CoreAudio.h"
#include <stdarg.h>
#include "SC_Prototypes.h"
#include "SC_HiddenWorld.h"
#include "SC_WorldOptions.h"
#include "SC_Time.hpp"
#include <math.h>
#include <stdlib.h>

#include "SCPluginDriver.h"

int32 server_timeseed() { return timeSeed(); }

int64 oscTimeNow() { return OSCTime(getTime()); }

void initializeScheduler() {}

SC_AudioDriver* SC_NewAudioDriver(struct World* inWorld) { return new SC_PluginAudioDriver(inWorld); }

SC_PluginAudioDriver::SC_PluginAudioDriver(struct World* inWorld): SC_AudioDriver(inWorld) {
    mInputChannelCount = inWorld->mNumInputs;
    mOutputChannelCount = inWorld->mNumOutputs;
}

SC_PluginAudioDriver::~SC_PluginAudioDriver()
{

}

void sc_SetDenormalFlags();
int SC_PluginAudioDriver::callback(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    sc_SetDenormalFlags();
    World* world = mWorld;
 
    mFromEngine.Free();
    mToEngine.Perform();
    mOscPacketsToEngine.Perform();

    const float** inBuffers = (const float**) buffer.getArrayOfReadPointers();
    float** outBuffers = (float**) buffer.getArrayOfWritePointers();

    int numSamples = NumSamplesPerCallback();
    int bufFrames = mWorld->mBufLength;
    int numBufs = numSamples / bufFrames;

    float* inBuses = mWorld->mAudioBus + mWorld->mNumOutputs * bufFrames;
    float* outBuses = mWorld->mAudioBus;
    int32* inTouched = mWorld->mAudioBusTouched + mWorld->mNumOutputs;
    int32* outTouched = mWorld->mAudioBusTouched;
    int bufFramePos = 0;    

    int64 oscTime = mOSCbuftime;
    int64 oscInc = mOSCincrement;
    double oscToSamples = mOSCtoSamples;

   for (int i = 0; i < numBufs; ++i, mWorld->mBufCounter++, bufFramePos += bufFrames) {
        int32 bufCounter = mWorld->mBufCounter;
        int32* tch;

        // copy+touch inputs
        tch = inTouched;
        for (int k = 0; k < mInputChannelCount; ++k) {
            const float* src = inBuffers[k] + bufFramePos;
            float* dst = inBuses + k * bufFrames;
            memcpy(dst, src, bufFrames * sizeof(float));
            *tch++ = bufCounter;
        }

        // run engine
        int64 schedTime;
        int64 nextTime = oscTime + oscInc;
        // DEBUG
        /*
        if (mScheduler.Ready(nextTime)) {
            double diff = (mScheduler.NextTime() - mOSCbuftime)*kOSCtoSecs;
            scprintf("rdy %.6f %.6f %.6f %.6f \n", (mScheduler.NextTime()-gStartupOSCTime) * kOSCtoSecs,
        (mOSCbuftime-gStartupOSCTime)*kOSCtoSecs, diff, (nextTime-gStartupOSCTime)*kOSCtoSecs);
        }
        */
        while ((schedTime = mScheduler.NextTime()) <= nextTime) {
            float diffTime = (float)(schedTime - oscTime) * oscToSamples + 0.5;
            float diffTimeFloor = floor(diffTime);
            world->mSampleOffset = (int)diffTimeFloor;
            world->mSubsampleOffset = diffTime - diffTimeFloor;

            if (world->mSampleOffset < 0)
                world->mSampleOffset = 0;
            else if (world->mSampleOffset >= world->mBufLength)
                world->mSampleOffset = world->mBufLength - 1;

            SC_ScheduledEvent event = mScheduler.Remove();
            event.Perform();
        }
        world->mSampleOffset = 0;
        world->mSubsampleOffset = 0.0f;

        World_Run(world);

        // copy touched outputs
        tch = outTouched;
        for (int k = 0; k < mOutputChannelCount; ++k) {
            float* dst = outBuffers[k] + bufFramePos;
            if (*tch++ == bufCounter) {
                const float* src = outBuses + k * bufFrames;
                memcpy(dst, src, bufFrames * sizeof(float));
            } else {
                memset(dst, 0, bufFrames * sizeof(float));
            }
        }

        // update buffer time
        oscTime = mOSCbuftime = nextTime;
    }

    mAudioSync.Signal();
}

// ====================================================================
//
//
bool SC_PluginAudioDriver::DriverSetup(int* outNumSamples, double* outSampleRate) {
    *outSampleRate = mPreferredSampleRate;
    *outNumSamples = mPreferredHardwareBufferFrameSize;
    return true;
}

bool SC_PluginAudioDriver::DriverStart() {
    return true;
}

bool SC_PluginAudioDriver::DriverStop() {
    return true;
}
