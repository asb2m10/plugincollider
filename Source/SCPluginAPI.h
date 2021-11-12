
#include "SC_CoreAudio.h"
#include <juce_audio_processors/juce_audio_processors.h>

class SC_PluginAudioDriver : public SC_AudioDriver {
    int mInputChannelCount, mOutputChannelCount;
protected:
    // Driver interface methods
    virtual bool DriverSetup(int* outNumSamplesPerCallback, double* outSampleRate);
    virtual bool DriverStart();
    virtual bool DriverStop();

public:
    SC_PluginAudioDriver(struct World* inWorld);
    virtual ~SC_PluginAudioDriver();

    int callback(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);
};