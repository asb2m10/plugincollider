Plugincollider
==============

Plugincollider is a generic (cross-platform/plugin format) wrapper that allows using a [SuperCollider](https://supercollider.github.io/) server inside a VST3 or AU plugin. The embedded server may be controlled over OSC as usual.

Now support Linux, macOS and Windows.

An external installation of [SuperCollider](https://supercollider.github.io/) is required to run the interpreted code. For now Plugincollider only acts as a server; consider this as a scsynth replacement. The classic SuperCollider UI is still used to send "synths/code" the the server that is running inside the plugin.

*Plugincollider is based on AU version of https://github.com/supercollider/SuperColliderAU*

# State of the project

SuperCollider is a highly modular ecosystem (sc-plugins, scsynth definitions) that needs to be adapted for each platform from the VST3/clap component. For now consider this as a vanilla scsynth implementation with no external plugins.

Latest builds are availables from [https://github.com/asb2m10/plugincollider/actions](https://github.com/asb2m10/plugincollider/actions)

### TODO

- [ ] implement /midi and /velocity from DAW midi message
- [ ] more accurate OSC DAW timing
- [ ] *Windows* bundle sndfile.dll within the plugin installation
- [ ] *macOS* enable Plugincollider to use SuperCollider scsynth plugin that the user previously installed

### macOS notes

macOS makes it harder to use shared libraries from SuperCollider to Plugincollider. It is easier to build it from your computer for now. I will check how I can do this without having to repackage everything in the plugin within the distribution.

### Windows notes

If you use the binary, you need to install [libsndfile](https://libsndfile.github.io/libsndfile/) and the bin directory (C:\Program Files\libsndfile\bin) must be put in the Windows PATH. Consider using [Chocolatey](https://chocolatey.org/install) with `choco install libsndfile` that does this for you.

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

## Build instructions

Be sure to install SuperCollider and JUCE dependencies; dont forget [sndfile](https://github.com/libsndfile/libsndfile). Then clone recursivly the repository and build Plugincollider like a normal cmake project :

    git clone --recursive https://github.com/asb2m10/Plugincollider
    cd Plugincollider
    mkdir build
    cd build
    cmake ..       # add `-G Xcode` if you want to use Xcode
    make

In order to test the plugin, with sclang execute this code (replace port 8898 where the server port is actually running):

    o = ServerOptions.new;
    s = Server.remote(\Plugincollider, NetAddr("127.0.0.1", 8898), o);
    { [SinOsc.ar(439, 0, 0.2), SinOsc.ar(444, 0, 0.2)] }.play(s);
