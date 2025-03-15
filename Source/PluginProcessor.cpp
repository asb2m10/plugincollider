/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "UDPPort.h"

//==============================================================================
PluginColliderAudioProcessor::PluginColliderAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)
              .withOutput("Out-3-4", juce::AudioChannelSet::stereo(), false)
              .withOutput("Out-5-6", juce::AudioChannelSet::stereo(), false)
              .withOutput("Out-7-8", juce::AudioChannelSet::stereo(), false)), superCollider(logger)
#endif
{
    addParameter(gain = new juce::AudioParameterFloat("gain", // parameterID
                                                      "Gain", // parameter name
                                                      0.0f,   // minimum value
                                                      1.0f,   // maximum value
                                                      0.5f)); // default value
    juce::Logger::setCurrentLogger(&logger);

    for(int i=0;i<32;i++) {
        addParameter(controlBus[i] = new juce::AudioParameterFloat(juce::String("cb") + juce::String(i), // parameterID
                                                      juce::String("ControlBus-")+juce::String(i+1), // parameter name
                                                      0.0f,   // minimum value
                                                      1.0f,   // maximum value
                                                      0.5f)); // default value
        controlBus[i]->addListener(this);
    }

    juce::PropertiesFile::Options options;
    options.applicationName = "PluginCollider";
    options.osxLibrarySubFolder = "Application Support";
    options.folderName = "PluginCollider";
    options.filenameSuffix = "settings";
    appProp.setStorageParameters(options);

    // TODO: move this to the .config directory.
    juce::PropertiesFile *prop = appProp.getUserSettings();
    synthPath = prop->getValue("synthPath", "");
#ifdef WIN32
    pluginPath = prop->getValue("pluginPath", "C:\\Program Files\\SuperCollider\\plugins");
#elif __APPLE__
    pluginPath = prop->getValue("pluginPath", "/Applications/SuperCollider.app/Contents/Resources/plugins");
#else
    pluginPath = prop->getValue("pluginPath", "/usr/lib/SuperCollider/plugins");
#endif

    udpPort.handleMessage = [this] (char *msg, int size, OSC_Packet *packet) {
        return superCollider.unrollOSCPacket(size, msg, packet);
    };

    pluginState = juce::ValueTree(IDs::ROOT);
    pluginState.addListener(this);

    if ( ! bindUdpPort() ) {
        logger.scprintf("Unable to bind to UDP port");
    }
}

PluginColliderAudioProcessor::~PluginColliderAudioProcessor() {
    logger.scprintf("PluginCollider bye\n");
    superCollider.quit();
    juce::Logger::setCurrentLogger(nullptr);
}

bool PluginColliderAudioProcessor::bindUdpPort() {
    if ( pluginState.hasProperty(IDs::udpPort)) {
        int targetPort = pluginState.getProperty(IDs::udpPort);
        if ( udpPort.connectToPort(targetPort) ) {
            logger.scprintf("Server listning to port %d\n", targetPort);
            return true;
        }
        logger.scprintf("Unable to bind to registred port %d, seeking random available port\n", targetPort);
    }

    if ( ! udpPort.connectToNextFreePort(8898) ) {
        logger.scprintf("Unable to find free UDP port\n");
        return false;
    }

    int newPort = udpPort.getListenPort();
    logger.scprintf("Server listning to port %d\n", newPort);
    pluginState.setProperty(IDs::udpPort, newPort, nullptr);
    return true;
}

bool PluginColliderAudioProcessor::setUdpPort(juce::String value) {
    int udpPortCheck = atoi(value.toRawUTF8());

    if ( udpPortCheck < 1024 || udpPortCheck > 65535 ) {
        logger.scprintf("Invalid udp port specified: %s\n", value.toRawUTF8());
        return false;
    }

    pluginState.setProperty(IDs::udpPort, udpPortCheck, nullptr);
    return bindUdpPort();
}

//==============================================================================
void PluginColliderAudioProcessor::prepareToPlay(double sampleRate,
                                                 int samplesPerBlock) {
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    // juce::File synthdefs =
    // juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("Application
    // Support/SuperCollider/synthdefs");

    superCollider.setup(sampleRate, samplesPerBlock, getTotalNumInputChannels(),
                        getTotalNumOutputChannels(), pluginPath, synthPath);
}

void PluginColliderAudioProcessor::releaseResources() {
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    superCollider.quit();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool PluginColliderAudioProcessor::isBusesLayoutSupported(
    const BusesLayout &layouts) const {
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

    command.call(*this);
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

void PluginColliderAudioProcessor::valueTreePropertyChanged(juce::ValueTree &treeWhosePropertyHasChanged, const juce::Identifier &property) {
    if ( property == IDs::synthdef ) {
        juce::var ret = pluginState.getProperty(IDs::synthdef);
        if ( ! ret.isBinaryData() )
            return;
        juce::MemoryBlock *mb = ret.getBinaryData();

        std::unique_ptr<SynthDef> def(SynthDef::fromMemory(*mb));
        logger.scprintf("Loading %s\n", def->getName().toRawUTF8());

        superCollider.loadSynthdef(def->getContent());
    }
}

//==============================================================================
void PluginColliderAudioProcessor::getStateInformation(juce::MemoryBlock &destData) {
    std::unique_ptr<juce::XmlElement> xml(pluginState.createXml());
    copyXmlToBinary (*xml, destData);
}

void PluginColliderAudioProcessor::setStateInformation(const void *data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary(data, sizeInBytes));
    pluginState = juce::ValueTree::fromXml(*xmlState);
}

bool PluginColliderAudioProcessor::getActivityMonitor() {
    bool activity = curActivity;
    curActivity = false;
    return activity;
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
    return 1; // NB: some hosts don't cope very well if you tell them there are
              // 0 programs, so this should be at least 1, even if you're not
              // really implementing programs.
}

int PluginColliderAudioProcessor::getCurrentProgram() { return 0; }

void PluginColliderAudioProcessor::setCurrentProgram(int index) {}

const juce::String PluginColliderAudioProcessor::getProgramName(int index) {
    return {};
}

void PluginColliderAudioProcessor::changeProgramName(
    int index, const juce::String &newName) {}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
    return new PluginColliderAudioProcessor();
}
