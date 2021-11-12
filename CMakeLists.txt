cmake_minimum_required(VERSION 3.15)

# The top-level CMakeLists.txt file for a project must contain a literal, direct call to the
# `project()` command. `project()` sets up some helpful variables that describe source/binary
# directories, and the current project version. This is a standard CMake command.

set(SC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/supercollider)

set(CMAKE_BUILD_TYPE "Debug")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -std=c++11")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
#set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -stdlib=libc++")

set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES} "")
set(CMAKE_FIND_LIBRARY_PREFIXES ${CMAKE_FIND_LIBRARY_PREFIXES} "")

set(PLUGINS_DIR ${SC_DIR}/build/server/plugins/${CMAKE_BUILD_TYPE}/)
file(GLOB PLUGINS ${SC_DIR}/build/server/plugins/${CMAKE_BUILD_TYPE}/*.scx)
set(SNDFILE_INCLUDE_DIR ${SC_DIR}/external_libraries/libsndfile)
set(BOOST_INCLUDE_DIR ${SC_DIR}/external_libraries/boost)
set(TLSF_INCLUDE_DIR ${SC_DIR}/external_libraries/TLSF-2.4.6/src)

file (GLOB BOOST_LIBS ${BOOST_INCLUDE_DIR}/libs/filesystem/src/*.cpp)

set_property(SOURCE ${SC_DIR}/common/SC_Filesystem_macos.cpp PROPERTY COMPILE_FLAGS -xobjective-c++)

set(CMAKE_MODULE_PATH ${SC_DIR}/cmake_modules)
include (${SC_DIR}/cmake_modules/FinalFile.cmake)
include (${SC_DIR}/cmake_modules/FindSndfile.cmake)
find_package(Sndfile)

include_directories(${SC_DIR}/external_libraries
	${BOOST_INCLUDE_DIR}
	${SNDFILE_INCLUDE_DIR}
	${PTHREADS_INCLUDE_DIR}
	${TLSF_INCLUDE_DIR}
	${SC_DIR}/external_libraries/boost_sync/include
	${SC_DIR}/external_libraries/boost_lockfree
	${SC_DIR}/external_libraries/nova-simd
	${SC_DIR}/external_libraries/nova-tt
	)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
    ${SC_DIR}/include/common
    ${SC_DIR}/common
    ${SC_DIR}/build/common
    ${SC_DIR}/include/server/
    ${SC_DIR}/include/plugin_interface
    ${SC_DIR}/server/scsynth/
	${SC_DIR}/server/common
    )

#add_definitions("-DSC_AUDIO_API=SC_AUDIO_API_AUDIOUNITS" ${AUDIOUNITS_DEFINITIONS})
add_definitions("-DSC_AUDIO_API=SC_AUDIO_API_PORTAUDIO")
#add_definitions("-DSC_DARWIN")
#add_definitions("-D__ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES=0")
add_definitions(-w) # TODO: fix deprecation warnings

if (NOT Boost_FOUND)
	file(GLOB boost_system_sources ${SC_DIR}/external_libraries/boost/libs/system/src/*cpp)
	file(GLOB boost_filesystem_sources ${SC_DIR}/external_libraries/boost/libs/filesystem/src/*cpp)
endif()

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/Source
	${SC_DIR}/include/common
	${SC_DIR}/common
	${SC_DIR}/build/common
	${SC_DIR}/include/server/
	${SC_DIR}/include/plugin_interface
	${SC_DIR}/server/scsynth/
	)

if (NOT Boost_FOUND)
	file(GLOB boost_system_sources ${SC_DIR}/external_libraries/boost/libs/system/src/*cpp)
	file(GLOB boost_filesystem_sources ${SC_DIR}/external_libraries/boost/libs/filesystem/src/*cpp)
endif()

project(PluginCollider VERSION 0.1.0)

# If you've installed JUCE somehow (via a package manager, or directly using the CMake install
# target), you'll need to tell this project that it depends on the installed copy of JUCE. If you've
# included JUCE directly in your source tree (perhaps as a submodule), you'll need to tell CMake to
# include that subdirectory as part of the build.

# find_package(JUCE CONFIG REQUIRED)        # If you've installed JUCE to your system
# or
add_subdirectory(libs/JUCE)             # If you've put JUCE in a subdirectory called JUCE

# If you are building a VST2 or AAX plugin, CMake needs to be told where to find these SDKs on your
# system. This setup should be done before calling `juce_add_plugin`.

# juce_set_vst2_sdk_path(...)
# juce_set_aax_sdk_path(...)

# `juce_add_plugin` adds a static library target with the name passed as the first argument
# (PluginCollider here). This target is a normal CMake target, but has a lot of extra properties set
# up by default. As well as this shared code static library, this function adds targets for each of
# the formats specified by the FORMATS arguments. This function accepts many optional arguments.
# Check the readme at `docs/CMake API.md` in the JUCE repo for the full list.

juce_add_plugin(PluginCollider
    # VERSION ...                               # Set this if the plugin version is different to the project version
     ICON_BIG  libs/supercollider/icons/sc_cube_256x256.png                              # ICON_* arguments specify a path to an image file to use as an icon for the Standalone
    # ICON_SMALL ...
    # COMPANY_NAME ...                          # Specify the name of the plugin's author
    IS_SYNTH FALSE                              # Is this a synth or an effect?
    # NEEDS_MIDI_INPUT TRUE/FALSE               # Does the plugin need midi input?
    # NEEDS_MIDI_OUTPUT TRUE/FALSE              # Does the plugin need midi output?
    # IS_MIDI_EFFECT TRUE/FALSE                 # Is this plugin a MIDI effect?
    # EDITOR_WANTS_KEYBOARD_FOCUS TRUE/FALSE    # Does the editor need keyboard focus?
    # COPY_PLUGIN_AFTER_BUILD TRUE/FALSE        # Should the plugin be installed to a default location after building?
    PLUGIN_MANUFACTURER_CODE DGSB               # A four-character manufacturer id with at least one upper-case character
    PLUGIN_CODE PLSC                            # A unique four-character plugin id with exactly one upper-case character
                                                # GarageBand 10.3 requires the first letter to be upper-case, and the remaining letters to be lower-case
    FORMATS VST3 Standalone                     # The formats to build. Other valid formats are: AAX Unity VST AU AUv3
    PRODUCT_NAME "PluginCollider")              # The name of the final executable, which can differ from the target name

# `juce_generate_juce_header` will create a JuceHeader.h for a given target, which will be generated
# into your build tree. This should be included with `#include <JuceHeader.h>`. The include path for
# this header will be automatically added to the target. The main function of the JuceHeader is to
# include all your JUCE module headers; if you're happy to include module headers directly, you
# probably don't need to call this.

# juce_generate_juce_header(PluginCollider)

# `target_sources` adds source files to a target. We pass the target that needs the sources as the
# first argument, then a visibility parameter for the sources which should normally be PRIVATE.
# Finally, we supply a list of source files that will be built into the target. This is a standard
# CMake command.

target_sources(PluginCollider
    PRIVATE
	${BOOST_LIBS}
	${SC_DIR}/common/fftlib.c
	${SC_DIR}/common/SC_fftlib.cpp
	${SC_DIR}/common/SC_AllocPool.cpp
	${SC_DIR}/common/SC_Errors.cpp
    #${SC_DIR}/common/SC_Apple.mm
	${SC_DIR}/common/Samp.cpp
	${SC_DIR}/common/SC_StringBuffer.cpp
	${SC_DIR}/common/SC_StringParser.cpp
	${SC_DIR}/common/sc_popen.cpp
	${SC_DIR}/common/strtod.c
	${SC_DIR}/common/SC_Reply.cpp
	#${SC_DIR}/common/SC_Filesystem_macos.cpp
	${SC_DIR}/common/SC_Filesystem_unix.cpp
	${SC_DIR}/external_libraries/TLSF-2.4.6/src/tlsf.c
	#${SC_DIR}/server/scsynth/SC_AU.cpp
	${SC_DIR}/server/scsynth/SC_BufGen.cpp
	${SC_DIR}/server/scsynth/SC_ComPort.cpp
	${SC_DIR}/server/scsynth/SC_CoreAudio.cpp
	${SC_DIR}/server/scsynth/SC_Graph.cpp
	${SC_DIR}/server/scsynth/SC_GraphDef.cpp
	${SC_DIR}/server/scsynth/SC_Group.cpp
	${SC_DIR}/server/scsynth/SC_Lib_Cintf.cpp
	${SC_DIR}/server/scsynth/SC_Lib.cpp
	${SC_DIR}/server/scsynth/SC_MiscCmds.cpp
	${SC_DIR}/server/scsynth/SC_Node.cpp
	${SC_DIR}/server/scsynth/SC_Rate.cpp
	${SC_DIR}/server/scsynth/SC_SequencedCommand.cpp
	${SC_DIR}/server/scsynth/SC_Str4.cpp
	${SC_DIR}/server/scsynth/SC_Unit.cpp
	${SC_DIR}/server/scsynth/SC_UnitDef.cpp
	


	# ${SC_DIR}/server/scsynth/Rendezvous.cpp
	${SC_DIR}/server/scsynth/SC_World.cpp

	Source/UDPPort.cpp 
	Source/OSCMessages.cpp
    Source/PluginEditor.cpp
    Source/PluginProcessor.cpp
	Source/SCProcess.cpp

	Source/SCPluginAPI.cpp

	${boost_system_sources}
	${boost_filesystem_sources}	
    ${PLUGINS})


# `target_compile_definitions` adds some preprocessor definitions to our target. In a Projucer
# project, these might be passed in the 'Preprocessor Definitions' field. JUCE modules also make use
# of compile definitions to switch certain features on/off, so if there's a particular feature you
# need that's not on by default, check the module header for the correct flag to set here. These
# definitions will be visible both to your code, and also the JUCE module code, so for new
# definitions, pick unique names that are unlikely to collide! This is a standard CMake command.

target_compile_definitions(PluginCollider
    PUBLIC
        # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
        JUCE_WEB_BROWSER=0  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_plugin` call
        JUCE_USE_CURL=0     # If you remove this, add `NEEDS_CURL TRUE` to the `juce_add_plugin` call
        JUCE_VST3_CAN_REPLACE_VST2=0)

# If your target needs extra binary assets, you can add them here. The first argument is the name of
# a new static library target that will include all the binary resources. There is an optional
# `NAMESPACE` argument that can specify the namespace of the generated binary data class. Finally,
# the SOURCES argument should be followed by a list of source files that should be built into the
# static library. These source files can be of any kind (wav data, images, fonts, icons etc.).
# Conversion to binary-data will happen when your target is built.

# juce_add_binary_data(AudioPluginData SOURCES ...)

# `target_link_libraries` links libraries and JUCE modules to other libraries or executables. Here,
# we're linking our executable target to the `juce::juce_audio_utils` module. Inter-module
# dependencies are resolved automatically, so `juce_core`, `juce_events` and so on will also be
# linked automatically. If we'd generated a binary data target above, we would need to link to it
# here too. This is a standard CMake command.

target_link_libraries(PluginCollider
    PRIVATE
        # AudioPluginData           # If we'd created a binary data target, we'd link to it here
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
		${Boost_SYSTEM_LIBRARY} ${Boost_FILESYSTEM_LIBRARY}
		${SNDFILE_LIBRARIES}
		sndfile
		)
