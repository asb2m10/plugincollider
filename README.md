PluginCollider
===============

*PluginCollider is an experimental fork of https://github.com/supercollider/SuperColliderAU and it is still not complete since it only builts only on macOS.*

## Current PluginCollider status before it becomes a GA project:

- [ ] Remove CoreAudio dependencies (e.g. Linux support and Windows support)
- [ ] Multichannel support
- [ ] Process midi-input/output and OSC timecode

PluginCollider is a generic (multiplatform/plugin format) wrapper that allows using a SuperCollider server inside a VST3 or AU. The embedded server may be controlled over OSC as usual. 

(JUCE)[https://juce.com/] framework is used as a generic wrapper.
* It provides a unified build system among plateforms and plugins configuration
* It provides a "Standalone" plugin version that greatly simplify development and debugging
* Simplifies plugin format evolutions and maintenance

In order to build PluginCollider, you first need to build SuperCollider, which is included in this repository as a submodule. For this to work you must first clone the PluginCollider with the recursive flag:

`git clone --recursive https://github.com/asb2m10/plugincollider`

After this, cd to the libs/supercollider directory and build as explained in the Build Instructions section in README_MACOS.md. This is needed for generating `SC_Version.hpp` and also for compiling plugins. It is important to note that the build process for PluginCollider assumes that the name of the supercollider build folder `build`.

After compiling SuperCollider, cd back to the PluginCollider root directory and run:

    mkdir build
    cd build
    cmake ..       # add `-G Xcode` if you want to use Xcode
    make

In order to test the plugin, with sclang execute this code:

   s = Server.remote(\pluginCollider, NetAddr("127.0.0.1", <listening plugin port>), o);
   { [SinOsc.ar(439, 0, 0.2), SinOsc.ar(444, 0, 0.2)] }.play(s);
