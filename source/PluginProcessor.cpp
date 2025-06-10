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
        juce::ValueTree synth = pluginState.getChildWithName(IDs::synths).getChildWithName(IDs::synth);
        if ( synth.isValid() ) {
            if ( synth.hasProperty(IDs::synthBlob) && synth.getProperty(IDs::synthBlob).isBinaryData() ) {
                superCollider.rt_loadSynthDef(synth.getProperty(IDs::synthBlob).getBinaryData());
                recompileState();
                if ( synth.getProperty(IDs::staticSynth) )
                    rt_playSynth();
            }
        }
    }

    loadMeasurer.reset(sampleRate, samplesPerBlock);
}

void PluginColliderAudioProcessor::releaseResources() {
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    loadMeasurer.reset();
    superCollider.quit();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool PluginColliderAudioProcessor::isBusesLayoutSupported(
    const BusesLayout &layouts) const {
    // FIX THIS, (see how it works with auval)
    return true;
}
#endif

int PluginColliderAudioProcessor::rt_playSynth() {
    if ( !superCollider.rt_getNode(kDefaultGroupId).isValid() ) {
        logger.scprintf("PluginCollider default group (1) was removed !\n");
        return 0;
    }
    if ( synthState.synthName[0] == 0 )
        return 0;
    int node = superCollider.rt_newSynth(synthState.synthName, -1, kDefaultGroupId);
    if ( node != 0 ) {
        for (const auto &p: synthState.precompiledMapValue) {
            superCollider.rt_setNodeValue(node, p.first, p.second);
        }
        for(const auto &p: synthState.precompiledMapValue) {
            superCollider.rt_setNodeValue(node, p.first, p.second);
        }
    }
    return node;
}

void PluginColliderAudioProcessor::processBlock(
    juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    juce::AudioProcessLoadMeasurer::ScopedTimer timer (loadMeasurer, buffer.getNumSamples());

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // TODO: There is probably something to send to world in term of timing...
    // auto *playhead = getPlayHead();
    // if (playhead != NULL) {
    //     juce::AudioPlayHead::CurrentPositionInfo posInfo;
    //     playhead->getCurrentPosition(posInfo);
    //     // posInfo.timeInSeconds;
    // }

    midiKeyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(), true);

    const juce::ScopedLock lock(superCollider.worldLock);
    try {
        if ( ! synthState.isStaticSynth ) {
            for (const auto meta : midiMessages) {
                const auto msg = meta.getMessage();
                if ( msg.isNoteOn() ) {
                    int node = rt_playSynth();
                    if ( node == 0 )
                        continue;

                    int note = msg.getNoteNumber();
                    int lastNode = boundedMidiVoice[note];
                    if ( lastNode != 0 ) {
                        if ( superCollider.rt_getNode(lastNode).isValid() )
                            superCollider.rt_freeNode(lastNode);
                        boundedMidiVoice[note] = 0;
                    }

                    boundedMidiVoice[note] = node;
                    if ( synthState.freqIdx != -1 ) {
                        superCollider.rt_setNodeValue(node, synthState.freqIdx, msg.getMidiNoteInHertz(note));
                    }

                    if ( synthState.velocityIdx != -1 ) {
                        superCollider.rt_setNodeValue(node, synthState.velocityIdx, msg.getFloatVelocity());
                    }

                    for(const auto &p: synthState.controlBusMap) {
                        superCollider.rt_assignControlBus(node, p.first, p.second);
                    }

                    for(const auto &p: synthState.precompiledMapValue) {
                        superCollider.rt_setNodeValue(node, p.first, p.second);
                    }
                } else if ( msg.isNoteOff() ) {
                    int note = msg.getNoteNumber();
                    if ( boundedMidiVoice[note] != 0 ) {
                        if ( superCollider.rt_getNode(boundedMidiVoice[note]).isValid() ) {
                            if ( synthState.gateIdx != -1 )
                                superCollider.rt_setNodeValue(boundedMidiVoice[note], synthState.gateIdx, 0);
                            // else {
                            //     superCollider.rt_freeNode(boundedMidiVoice[note]);
                            //     boundedMidiVoice[note] = 0;
                            // }
                        }
                    }
                }
            }
        }
        command.call(*this);
    } catch (std::exception &e) {
        logger.scprintf("!!! Catching exception on dsp thread: %s\n", e.what());
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

void PluginColliderAudioProcessor::parameterValueChanged (int parameterIndex, float newValue) {
    command.push([this, parameterIndex, newValue](PluginColliderAudioProcessor &proc) {
        float tagetValue = controlBus[parameterIndex-1]->getRangedValue(newValue);
        superCollider.rt_setControlBusValue(parameterIndex-1, tagetValue);
    });
}

bool PluginColliderAudioProcessor::loadSynthDef(SynthDef *synthDef) {
    const juce::ScopedLock lock(superCollider.worldLock);

    if ( superCollider.world == nullptr )
        return false;

    if ( !superCollider.rt_loadSynthDef(&(synthDef->getContent())) )
        return false;

    pluginState.getChildWithName(IDs::synths).removeAllChildren(nullptr);

    juce::ValueTree synth = juce::ValueTree(IDs::synth);
    synth.setProperty(IDs::synthName, synthDef->getName(), nullptr);
    synth.setProperty(IDs::synthBlob, synthDef->getContent(), nullptr);
    synth.setProperty(IDs::staticSynth, false, nullptr);
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
    synth.addChild(parameters, 0, nullptr);
    pluginState.getChildWithName(IDs::synths).addChild(synth, 0, nullptr);

    recompileState();
    return true;
}

void PluginColliderAudioProcessor::recompileState() {
    SynthState nextSynthState;
    memset(nextSynthState.synthName, 0, 127);
    nextSynthState.freqIdx = -1;
    nextSynthState.velocityIdx = -1;
    nextSynthState.gateIdx = -1;
    juce::ValueTree synth = pluginState.getChildWithName(IDs::synths).getChildWithName(IDs::synth);
    if ( synth.isValid() ) {
        nextSynthState.isStaticSynth = synth.getProperty(IDs::staticSynth);
        juce::String name = synth.getProperty(IDs::synthName);
        strncpy(nextSynthState.synthName, name.toRawUTF8(), 127);
        juce::ValueTree params = synth.getChildWithName(IDs::parameters);
        for(int i=0;i<params.getNumChildren();i++) {
            juce::ValueTree param = params.getChild(i);

            if ( ! nextSynthState.isStaticSynth ) {
                juce::String pName = param.getProperty(IDs::pName);
                if ( pName == "freq" ) {
                    nextSynthState.freqIdx = i;
                    continue;
                }
                if ( pName == "amp" ) {
                    nextSynthState.velocityIdx = i;
                    continue;
                }
                if ( pName == "gate" ) {
                    nextSynthState.gateIdx = i;
                    continue;
                }
            }

            int cbIdx = param.getProperty(IDs::pControlBus, -1);
            if ( cbIdx != -1 ) {
                nextSynthState.controlBusMap.emplace(i, cbIdx);
                continue;
            }

            if ( param.hasProperty(IDs::pCurrentValue) && param.getProperty(IDs::pCurrentValue) != param.getProperty(IDs::pDefaultValue) ) {
                float value = param.getProperty(IDs::pCurrentValue);
                nextSynthState.precompiledMapValue.emplace(i, value);
            }
        }
    }

    // Push the synthstate to the command queue on the audio thread
    command.push([nextSynthState](PluginColliderAudioProcessor &proc) {
        proc.synthState = std::move(nextSynthState);
    });
}

void PluginColliderAudioProcessor::valueTreePropertyChanged(juce::ValueTree &treeWhosePropertyHasChanged, const juce::Identifier &property) {
     if ( property == IDs::pCurrentValue ) {
        int idx = treeWhosePropertyHasChanged.getProperty(IDs::pIdx);
        float value = treeWhosePropertyHasChanged.getProperty(IDs::pCurrentValue);
        recompileState();
        command.push([this, idx, value](PluginColliderAudioProcessor &proc) {
            superCollider.rt_setNodeValue(kDefaultGroupId, idx, value);
        });
         return;
     }

    if ( property == IDs::pControlBus ) {
        recompileState();
    }

     if ( property == IDs::staticSynth ) {
        resetStaticSynth();
        recompileState();
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
    if ( childWhichHasBeenRemoved.getType() == IDs::synth ) {
        command.push([this](PluginColliderAudioProcessor &proc) {
            superCollider.rt_freeGroup(kDefaultGroupId);
        });
    }
}

//==============================================================================
void PluginColliderAudioProcessor::getStateInformation(juce::MemoryBlock &destData) {
    std::unique_ptr<juce::XmlElement> xml(pluginState.createXml());
    copyXmlToBinary(*xml, destData);
}

void PluginColliderAudioProcessor::setStateInformation(const void *data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary(data, sizeInBytes));
    juce::ValueTree tmpState = juce::ValueTree::fromXml(*xmlState);

    resetPluginState();

    // if ( tmpState.getProperty(IDs::version) != IDS_VERSION ) {
    //     resetPluginState();
    //     recompileState();
    // } else {
    //     pluginState = tmpState;
    // }
}

void PluginColliderAudioProcessor::resetPluginState() {
    pluginState.removeListener(this);
    pluginState.removeAllChildren(nullptr);

    juce::ValueTree synths = juce::ValueTree(IDs::synths);
    pluginState.addChild(synths, 1, nullptr);
    synths.addChild(juce::ValueTree(IDs::synth), 1, nullptr);

    juce::ValueTree controlBusses = juce::ValueTree(IDs::controlbuses);
    for(int i=0;i<NUMBER_OF_CONTROL_BUSES;i++) {
        juce::ValueTree controlBus = juce::ValueTree(IDs::controlbus);
        controlBus.setProperty(IDs::cbName, juce::String("Control Bus ") + juce::String(i), nullptr);
        controlBus.setProperty(IDs::cbIdx, i, nullptr);
        controlBus.setProperty(IDs::cbRange, "0 1 0.001", nullptr);
        controlBusses.addChild(controlBus, i, nullptr);
    }
    pluginState.addChild(controlBusses, 0, nullptr);
    juce::ValueTree controlBus = juce::ValueTree(IDs::controlbus);

    juce::ValueTree scratchpad = juce::ValueTree(IDs::scratchpad);
    scratchpad.setProperty(IDs::spCode, "", nullptr);
    pluginState.addChild(scratchpad, 0, nullptr);

    pluginState.setProperty(IDs::version, IDS_VERSION, nullptr);
    pluginState.addListener(this);
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
