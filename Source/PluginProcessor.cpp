/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
PluginColliderAudioProcessor::PluginColliderAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)
              .withOutput("Out-3-4", juce::AudioChannelSet::stereo(), false)
              .withOutput("Out-5-6", juce::AudioChannelSet::stereo(), false)
              .withOutput("Out-7-8", juce::AudioChannelSet::stereo(), false))
#endif
{
  addParameter(gain = new juce::AudioParameterFloat("gain", // parameterID
                                                    "Gain", // parameter name
                                                    0.0f,   // minimum value
                                                    1.0f,   // maximum value
                                                    0.5f)); // default value
}

PluginColliderAudioProcessor::~PluginColliderAudioProcessor() {
  scprintf("PluginCollider bye\n");
  superCollider.quit();
}

//==============================================================================
const juce::String PluginColliderAudioProcessor::getName() const {
  return JucePlugin_Name;
}

bool PluginColliderAudioProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
  return true;
#else
  return false;
#endif
}

bool PluginColliderAudioProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
  return true;
#else
  return false;
#endif
}

bool PluginColliderAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
  return true;
#else
  return false;
#endif
}

double PluginColliderAudioProcessor::getTailLengthSeconds() const {
  return 0.0;
}

int PluginColliderAudioProcessor::getNumPrograms() {
  return 1; // NB: some hosts don't cope very well if you tell them there are 0
            // programs, so this should be at least 1, even if you're not really
            // implementing programs.
}

int PluginColliderAudioProcessor::getCurrentProgram() { return 0; }

void PluginColliderAudioProcessor::setCurrentProgram(int index) {}

const juce::String PluginColliderAudioProcessor::getProgramName(int index) {
  return {};
}

void PluginColliderAudioProcessor::changeProgramName(int index, const juce::String &newName) {}

//==============================================================================
void PluginColliderAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
  // Use this method as the place to do any pre-playback
  // initialisation that you need..

  // juce::File synthdefs =
  // juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("Application
  // Support/SuperCollider/synthdefs");
  superCollider.setup(sampleRate, samplesPerBlock, getTotalNumInputChannels(), getTotalNumOutputChannels(), nullptr, nullptr);
}

void PluginColliderAudioProcessor::releaseResources() {
  // When playback stops, you can use this as an opportunity to free up any
  // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool PluginColliderAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const {
  // FIX THIS, (see how it works with auval)
  return true;
}
#endif

void PluginColliderAudioProcessor::processBlock(
    juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages) {
  juce::ScopedNoDenormals noDenormals;
  auto totalNumInputChannels = getTotalNumInputChannels();
  auto totalNumOutputChannels = getTotalNumOutputChannels();

  // In case we have more outputs than inputs, this code clears any output
  // channels that didn't contain input data, (because these aren't
  // guaranteed to be empty - they may contain garbage).
  // This is here to avoid people getting screaming feedback
  // when they first compile a plugin, but obviously you don't need to keep
  // this code if your algorithm always overwrites all the output channels.
  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    buffer.clear(i, 0, buffer.getNumSamples());

  // TODO: There is probably something to send to world in term of timing...
  auto *playhead = getPlayHead();
  if (playhead != NULL) {
    juce::AudioPlayHead::CurrentPositionInfo posInfo;
    playhead->getCurrentPosition(posInfo);
    // posInfo.timeInSeconds;
  }

  superCollider.run(buffer, midiMessages);
  buffer.applyGain(*gain);
}

//==============================================================================
bool PluginColliderAudioProcessor::hasEditor() const {
  return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor *PluginColliderAudioProcessor::createEditor() {
  return new PluginColliderAudioProcessorEditor(*this);
}

//==============================================================================
void PluginColliderAudioProcessor::getStateInformation(juce::MemoryBlock &destData) {
  // You should use this method to store your parameters in the memory block.
  // You could do that either as raw data, or use the XML or ValueTree classes
  // as intermediaries to make it easy to save and load complex data.
}

void PluginColliderAudioProcessor::setStateInformation(const void *data, int sizeInBytes) {
  // You should use this method to restore your parameters from this memory
  // block, whose contents will have been created by the getStateInformation()
  // call.
}

bool PluginColliderAudioProcessor::getActivityMonitor() {
  bool activity = curActivity;
  curActivity = false;
  return activity;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new PluginColliderAudioProcessor();
}
