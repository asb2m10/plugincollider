Plugincollider
==============

Plugincollider is a generic (cross-platform/plugin format) wrapper that allows using a [SuperCollider](https://supercollider.github.io/) server inside a VST3 or AU plugin. The embedded server may be controlled over OSC as usual.

Now support Linux, macOS and Windows.

*Plugincollider is based on AU version of https://github.com/supercollider/SuperColliderAU*

## State of the project

SuperCollider is a highly modular ecosystem (sc-plugins, scsynth definitions) that needs to be adapted for each platform from the VST3/clap component. For now consider this as a vanilla scsynth implementation with no external plugins.

Latest build are available from [https://github.com/asb2m10/plugincollider/actions](https://github.com/asb2m10/plugincollider/actions)

## Usage - Supercollider server
Plugincollider can be used as a standard SuperCollider server by using the SC IDE (or any other sclang client). You can test the plugin by using this SC code (where Plugincollider is running at 127.0.0.1:8898) :

    o = ServerOptions.new;
    s = Server.remote(\Plugincollider, NetAddr("127.0.0.1", 8898), o);
    { [SinOsc.ar(439, 0, 0.2), SinOsc.ar(444, 0, 0.2)] }.play(s);

## Usage - SynthDefs files

Plugincollider can load previously compiled [SynthDefs](scsyndef) (*.scsyndef) that will be saved within the DAW plugin state. No installation/usage of Supercollider afterwards is required if you want to exclusively use scsyndef files.

If the SynthDef has arguments, they will be exposed to the plugin and the user can set the lower and upper values for each arguments. The user can then easily change them from the Plugincollider UI.

### FX Mode

If there is a SynthDef loaded, the plugin can be put in "FX Mode" that will run this SynthDef on a single node every time the plugin is running. This can be useful if you want to use the plugin as an effect or a drone/noodle.

### Non-Fx Mode (Synth Mode)

In this mode (and when a SynthDef is loaded), everytime Plugincollider receives a midi node, it will trigger this synth based on those parameters:

| Midi Event         | ScSynDef args | Conversion                                                          |
| ------------------ | ------------- | ------------------------------------------------------------------- |
| note-on midi note  | freq          | Converts midi node (0-127) to frequency (hz)                        |
| note-on velocity   | amp           | Converts midi velocity (0-127) to amp(0.0-1.0), linearly (for now). |
| note-off           | gate          | Sends gate=0 on midi note off                                       |

If for example your SynthDef doesn't have the gate arguments, the node will be freed once the note off is triggered.

# Known issues

* Be sure to set your DAW latency size to a power of two (256, 512, 1024) otherwise some SC plugins might not work properly.
* If you are running multiple VST instances, scsynth errors messages might end up into one specific unrelated vst logs since scsynth is design to be run into one single process. Some DAWs has a "Dedicated process" runtime that might resolve this issue.

# TODO

- [ ] more accurate handling of "WorldLock" - espacially for OSC messages
- [x] implement /note and /amp from DAW midi message
- [ ] more accurate OSC DAW timing
- [ ] *macOS* enable Plugincollider to use SuperCollider scsynth plugin that the user previously installed
- [x] *Windows* bundle sndfile.dll within the plugin installation

### macOS notes

macOS makes it harder to use shared libraries from SuperCollider to Plugincollider. It is easier to build it from your computer for now. I will check how I can do this without having to repackage everything in the plugin within the distribution.

# Interacting with SuperCollider server (scsynth) from DAW

Plugin parameters are now linked to the first 32 control buses. The SC code must normalize the values from 0.0 to 1.0; consider using a helper function like this:

    var mkMappedBus = {|sym, defaultRange=([0, 1]), type=\linlin|
        var v = NamedControl.kr(sym);
        var range = NamedControl.kr(sym ++ 'Range', defaultRange);
        v.perform(type, 0, 1, *range)
    };

    x = SynthDef("freqtest", {
            var freq = mkMappedBus.(\freq, [20, 20000], \linexp);
            var sig = SinOsc.ar(freq);
            Out.ar(sig, sig!2 * 1);
        }).play(s);

    b = Bus.control(s, 1);
    x.map(\freq, b)

Please note that control buses are not yet read from server to DAW.

# Build instructions

Be sure to install SuperCollider and JUCE dependencies; dont forget [sndfile](https://github.com/libsndfile/libsndfile) on Linux. Then clone recursivly the repository and build Plugincollider like a normal cmake project :

    git clone --recursive https://github.com/asb2m10/Plugincollider
    cd Plugincollider
    mkdir build
    cd build
    cmake ..       # add `-G Xcode` if you want to use Xcode
    make

In order to test the plugin, with sclang execute this code (replace port 8898 where the server port is actually running):

