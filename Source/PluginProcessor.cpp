/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

const int kDefaultPortNumber = 9989;
const int kDefaultBlockSize = 64;
const int kDefaultBeatDiv = 1;
const int kDefaultNumWireBufs = 64;
const int kDefaultRtMemorySize = 8192;

//==============================================================================
PluginColliderAudioProcessor::PluginColliderAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    superCollider = new SCProcess();

    addParameter (gain = new juce::AudioParameterFloat ("gain", // parameterID
                                                        "Gain", // parameter name
                                                        0.0f,   // minimum value
                                                        1.0f,   // maximum value
                                                        0.5f)); // default value
}

PluginColliderAudioProcessor::~PluginColliderAudioProcessor()
{
    superCollider->quit();
    delete superCollider;
}

//==============================================================================
const juce::String PluginColliderAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PluginColliderAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PluginColliderAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PluginColliderAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PluginColliderAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PluginColliderAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int PluginColliderAudioProcessor::getCurrentProgram()
{
    return 0;
}

void PluginColliderAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String PluginColliderAudioProcessor::getProgramName (int index)
{
    return {};
}

void PluginColliderAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void PluginColliderAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    WorldOptions options;
    options.mPreferredSampleRate = sampleRate;
    options.mBufLength = samplesPerBlock;
    options.mPreferredHardwareBufferFrameSize = samplesPerBlock;
    options.mMaxWireBufs = kDefaultNumWireBufs;
    options.mRealTimeMemorySize = kDefaultRtMemorySize;
    options.mNumBuffers = 8192;
    options.mNumInputBusChannels = 2;
    options.mNumOutputBusChannels = 2;
    options.mVerbosity = 2;
    
    juce::File pluginDir("/Applications/SuperCollider.app/Contents/Resources/plugins");
    //juce::File pluginDir("/usr/lib/SuperCollider/plugins");
    //juce::File pluginDir("/home/asb2m10/var/src/plugincollider/libs/supercollider/build/server/plugins");
    juce::File synthdefs = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("Application Support/SuperCollider/synthdefs");
    superCollider->startUp(options, pluginDir.getFullPathName().toStdString(), synthdefs.getFullPathName().toStdString(), 9989);

    scprintf("*******************************************************\n");
    scprintf("PluginCollider Initialized \n");
    scprintf("PluginCollider mPreferredHardwareBufferFrameSize: %d \n",options.mPreferredHardwareBufferFrameSize );
    scprintf("PluginCollider mBufLength: %d \n",options.mBufLength );
    scprintf("PluginCollider  port: %d \n", superCollider->portNum );
    scprintf("PluginCollider  mMaxWireBufs: %d \n", options.mMaxWireBufs );
    scprintf("PluginCollider  mRealTimeMemorySize: %d \n", options.mRealTimeMemorySize );
	scprintf("*******************************************************\n");
    
    fflush(stdout);
}

void PluginColliderAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool PluginColliderAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void PluginColliderAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // TODO: There is probably something to send to world in term of timing...
    auto *playhead = getPlayHead();
    if ( playhead != NULL ) {
        juce::AudioPlayHead::CurrentPositionInfo posInfo;
        playhead->getCurrentPosition(posInfo);
        //posInfo.timeInSeconds;
    }
    
    superCollider->run(buffer, midiMessages, getSampleRate());
    buffer.applyGain(*gain);
}

//==============================================================================
bool PluginColliderAudioProcessor::hasEditor() const
{
    return true;  // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* PluginColliderAudioProcessor::createEditor()
{
    return new PluginColliderAudioProcessorEditor (*this);
}

//==============================================================================
void PluginColliderAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void PluginColliderAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

bool PluginColliderAudioProcessor::getActivityMonitor()
{
    bool activity = curActivity;
    curActivity = false;
    return activity;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginColliderAudioProcessor();
}
