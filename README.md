Plugincollider
==============

Plugincollider is a generic (multiplatform/plugin format) wrapper that allows using a [SuperCollider](https://supercollider.github.io/) server inside a VST3 or AU plugin. The embedded server may be controlled over OSC as usual. 

Now support Linux, macOS and Windows.

An external installation of [Supercollider](https://supercollider.github.io/) is required to run the interpreted code. For now Plugincollider only acts as a server; consider this as a scsynth replacement. The classic Supercollider UI is still used to send "synths/code" the the server that is running inside the plugin.

*Plugincollider is based on AU version of https://github.com/supercollider/SuperColliderAU*

## Build instructions

Be sure to install Supercollider and JUCE dependencies; dont forget [sndfile](https://github.com/libsndfile/libsndfile). Then clone recursivly the repository and build Plugincollider like a normal cmake project :

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
