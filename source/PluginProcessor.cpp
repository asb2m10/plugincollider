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


#include "PluginProcessor.h"
#include "ui/PluginEditor.h"
#include "NodeContainer.h"
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
        addParameter(controlBus[i] = new ControlBusParameter(i)); // default value
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

    pluginState = juce::ValueTree(IDs::root);
    resetPluginState();

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
    if ( pluginState.hasProperty(IDs::udpport)) {
        int targetPort = pluginState.getProperty(IDs::udpport);
        if ( udpPort.connectToPort(targetPort) ) {
            logger.scprintf("Server listening to port %d\n", targetPort);
            return true;
        }
        logger.scprintf("Unable to bind to registered port %d, seeking random available port\n", targetPort);
    }

    if ( ! udpPort.connectToNextFreePort(8898) ) {
        logger.scprintf("Unable to find free UDP port\n");
        return false;
    }

    int newPort = udpPort.getListenPort();
    logger.scprintf("Server listening to port %d\n", newPort);
    pluginState.setProperty(IDs::udpport, newPort, nullptr);
    return true;
}

bool PluginColliderAudioProcessor::setUdpPort(juce::String value) {
    int udpPortCheck = atoi(value.toRawUTF8());

    if ( udpPortCheck < 1024 || udpPortCheck > 65535 ) {
        logger.scprintf("Invalid udp port specified: %s\n", value.toRawUTF8());
        return false;
    }

    pluginState.setProperty(IDs::udpport, udpPortCheck, nullptr);
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

    command.reset();

    if ( superCollider.setup(sampleRate, samplesPerBlock, getTotalNumInputChannels(),
                        getTotalNumOutputChannels(), pluginPath, synthPath) ) {
        rt_loadSynthDef(pluginState.getChildWithName(IDs::rootnode));
        container = std::make_unique<NodeContainer>(pluginState.getChildWithName(IDs::rootnode));
        container->rt_allocate(superCollider);
    }

    loadMeasurer.reset(sampleRate, samplesPerBlock);
}

void PluginColliderAudioProcessor::releaseResources() {
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    container->rt_free(superCollider);
    loadMeasurer.reset();
    superCollider.quit();
}

void PluginColliderAudioProcessor::reloadNodeContainer() {
    ASyncReply<std::unique_ptr<NodeContainer>> reply;
    reply.content = std::make_unique<NodeContainer>(pluginState.getChildWithName(IDs::rootnode));
    command.push([this, &reply](PluginColliderAudioProcessor &proc) {
        container->rt_free(superCollider);
        std::swap(container, reply.content);
        container->rt_allocate(superCollider);
        reply.notify(0);
    });
    reply.wait();
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

    juce::AudioProcessLoadMeasurer::ScopedTimer timer(loadMeasurer, buffer.getNumSamples());

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    midiKeyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(), true);

    const juce::ScopedLock lock(superCollider.worldLock);
    try {
        command.call(*this);
        container->rt_processMidiMessages(superCollider, midiMessages);
        superCollider.run(buffer, midiMessages);
    } catch (std::exception &e) {
        logger.scprintf("!!! Catching exception on dsp thread: %s\n", e.what());
    }

    buffer.applyGain(*gain);
}

//==============================================================================
bool PluginColliderAudioProcessor::hasEditor() const {
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor *PluginColliderAudioProcessor::createEditor() {
    return new PluginColliderAudioProcessorEditor(*this);
}

void PluginColliderAudioProcessor::parameterValueChanged(int parameterIndex, float newValue) {
    command.push([this, parameterIndex, newValue](PluginColliderAudioProcessor &proc) {
        float targetValue = controlBus[parameterIndex-1]->getRangedValue(newValue);
        superCollider.rt_setControlBusValue(parameterIndex-1, targetValue);
    });
}

bool PluginColliderAudioProcessor::replaceSynthDef(juce::MemoryBlock &block, juce::ValueTree &target) {
    ASyncReply<int> reply;
    command.push([this, &reply, &block](PluginColliderAudioProcessor &proc) {
        reply.notify(proc.superCollider.rt_loadSynthDef(&block) ? 0 : 1);
    });
    if ( reply.wait() != 0 ) {
        return false;
    }

    std::unique_ptr<SynthDef> synthDef;
    synthDef.reset(SynthDef::fromMemory(block));

    target.setProperty(IDs::synthBlob, block, nullptr);
    target.setProperty(IDs::synthName, synthDef->getName(), nullptr);

    target.removeChild(target.getChildWithName(IDs::parameters), nullptr);
    juce::ValueTree parameters = juce::ValueTree(IDs::parameters);
    for(int i=0;i<synthDef->getParameters().size();i++) {
        juce::ValueTree parameter = juce::ValueTree(IDs::parameter);
        parameter.setProperty(IDs::pName, synthDef->getParameters()[i], nullptr);
        parameter.setProperty(IDs::pIdx, i, nullptr);
        parameter.setProperty(IDs::pDefaultValue, synthDef->getParametersValues()[i], nullptr);
        parameter.setProperty(IDs::pRange, synthDef->guessParameterRange(i), nullptr);
        parameter.setProperty(IDs::pControlBus, -1, nullptr);
        parameters.addChild(parameter, i, nullptr);
    }
    target.addChild(parameters, -1, nullptr);

    return true;
}

void PluginColliderAudioProcessor::rt_loadSynthDef(juce::ValueTree vt) {
    if ( vt.hasType(IDs::groupnode) ) {
        for(int i=0;i<vt.getNumChildren();i++) {
            rt_loadSynthDef(vt.getChild(i));
        }
        return;
    }
    if ( vt.hasType(IDs::notenode) || vt.hasType(IDs::fxnode) ) {
        juce::MemoryBlock *block = vt.getProperty(IDs::synthBlob).getBinaryData();
        if ( block != nullptr ) {
            superCollider.rt_loadSynthDef(block);
        }
    }
}

void PluginColliderAudioProcessor::valueTreePropertyChanged(juce::ValueTree &treeWhosePropertyHasChanged, const juce::Identifier &property) {
     if ( property == IDs::pCurrentValue ) {
        int idx = treeWhosePropertyHasChanged.getProperty(IDs::pIdx);
        float value = treeWhosePropertyHasChanged.getProperty(IDs::pCurrentValue);
        juce::ValueTree node = treeWhosePropertyHasChanged.getParent().getParent();
        int nodeid = node.getProperty(IDs::nodeid, -1);
        if ( nodeid != -1 ) {
            command.push([this, nodeid, idx, value](PluginColliderAudioProcessor &proc) {
                superCollider.rt_setNodeValue(nodeid, idx, value);
            });
        }
        return;
     }

    if ( property == IDs::pControlBus ) {
        reloadNodeContainer();
    }

    if ( property == IDs::cbName ) {
        int idx = treeWhosePropertyHasChanged.getProperty(IDs::cbIdx);
        juce::String name = treeWhosePropertyHasChanged.getProperty(IDs::cbName);
        controlBus[idx]->setName(name);
        const auto details = juce::AudioProcessorListener::ChangeDetails{}.withParameterInfoChanged(true);
        updateHostDisplay(details);
    }

    if ( property == IDs::cbRange ) {
        int idx = treeWhosePropertyHasChanged.getProperty(IDs::cbIdx);
        PluginColliderRange range(treeWhosePropertyHasChanged.getProperty(IDs::cbRange));
        controlBus[idx]->setRange(range);
        const auto details = juce::AudioProcessorListener::ChangeDetails{}.withParameterInfoChanged(true);
        updateHostDisplay(details);
    }
}

void PluginColliderAudioProcessor::valueTreeChildRemoved (juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) {
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
