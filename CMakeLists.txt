cmake_minimum_required(VERSION 3.15)

# The top-level CMakeLists.txt file for a project must contain a literal, direct call to the
# `project()` command. `project()` sets up some helpful variables that describe source/binary
# directories, and the current project version. This is a standard CMake command.

if(NOT SC_DIR)
set(SC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/supercollider)
endif()

enable_language(C CXX)

set(CMAKE_CXX_STANDARD 17)
if(NOT MSVC) # should be enought with line above
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -std=c++17")
endif()

if (APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
	set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -stdlib=libc++")
endif (APPLE)

if (NOT APPLE AND NOT WIN32)
	# on apple we leave this undefined so it defaults to .scx (in the code)
	add_definitions("-DSC_PLUGIN_EXT=\".so\"")
endif()

set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES} "")
set(CMAKE_FIND_LIBRARY_PREFIXES ${CMAKE_FIND_LIBRARY_PREFIXES} "")

set(PLUGINS_DIR ${SC_DIR}/build/server/plugins/${CMAKE_BUILD_TYPE}/)
file(GLOB PLUGINS ${SC_DIR}/build/server/plugins/${CMAKE_BUILD_TYPE}/*.scx)

set(BOOST_INCLUDE_DIR ${SC_DIR}/external_libraries/boost)
set(TLSF_INCLUDE_DIR ${SC_DIR}/external_libraries/TLSF-2.4.6/src)

file (GLOB BOOST_LIBS ${BOOST_INCLUDE_DIR}/libs/filesystem/src/*.cpp)

set_property(SOURCE ${SC_DIR}/common/SC_Filesystem_macos.cpp PROPERTY COMPILE_FLAGS -xobjective-c++)

set(CMAKE_MODULE_PATH ${SC_DIR}/cmake_modules)
include (${SC_DIR}/cmake_modules/FinalFile.cmake)
if (NOT APPLE AND NOT WIN32)
	set(SNDFILE_INCLUDE_DIR ${SC_DIR}/external_libraries/libsndfile)
	include (${SC_DIR}/cmake_modules/FindSndfile.cmake)
else()
	# On Apple and Windows, we build a local sndfile
	# Setup libsndfile
	include(FetchContent)
	FetchContent_Declare(
    	sndfile
    	GIT_REPOSITORY "https://github.com/libsndfile/libsndfile"
    	GIT_TAG 17a19ab264fcf238f46104cea9af9a0a5ca3786d
    	GIT_PROGRESS TRUE
    	GIT_SHALLOW TRUE
	)

	set(ENABLE_EXTERNAL_LIBS OFF CACHE INTERNAL "xiph!")
	set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "build shared!")
	set(BUILD_PROGRAMS OFF CACHE INTERNAL "dont build programs")
	set(BUILD_TESTING OFF CACHE INTERNAL "dont build testing")
	set(BUILD_EXAMPLES OFF CACHE INTERNAL "dont build examples")
	set(ENABLE_CPACK OFF CACHE INTERNAL "disable cpack")
	set(ENABLE_STATIC_RUNTIME OFF CACHE INTERNAL "disable static runtime")
	set(ENABLE_PACKAGE_CONFIG OFF CACHE INTERNAL "disable package config")

	FetchContent_MakeAvailable(sndfile)

	set(SNDFILE_LIBRARIES sndfile)
	set(SNDFILE_INCLUDE_DIR ${sndfile_SOURCE_DIR}/include)
endif()

include_directories(${SC_DIR}/external_libraries
	${BOOST_INCLUDE_DIR}
	${SNDFILE_INCLUDE_DIR}
	${PTHREADS_INCLUDE_DIR}
	${TLSF_INCLUDE_DIR}
	${SC_DIR}/external_libraries/boost_sync/include
	${SC_DIR}/external_libraries/boost_lockfree
	${SC_DIR}/external_libraries/nova-simd
	${SC_DIR}/external_libraries/nova-tt
	${SC_DIR}/common
	${SC_DIR}/include/common
    ${SC_DIR}/include/server/
    ${SC_DIR}/include/plugin_interface
	${SC_DIR}/include/plugin_interface
    ${SC_DIR}/server/scsynth/
	${SC_DIR}/server/common
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
	)

# This is a hack since we will define our own Driver
add_definitions("-DSC_AUDIO_API=SC_AUDIO_API_PORTAUDIO")

if(NOT APPLE)
	#apple gets it`s own definition in SC_fftlib.cpp
	#this can be changed to SC_FFT_FFTW when we can link to fftw
	add_definitions("-DSC_FFT_GREEN")
endif()

if (NOT SC_DYNAMIC_PLUGINS)
	add_definitions("-DSTATIC_PLUGINS")
endif()

if (APPLE)
	add_definitions("-DSC_DARWIN")
	add_definitions("-D__ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES=0")
endif (APPLE)
add_definitions(-w) # TODO: fix deprecation warnings

if (NOT Boost_FOUND)
	file(GLOB boost_system_sources ${SC_DIR}/external_libraries/boost/libs/system/src/*cpp)
	file(GLOB boost_filesystem_sources ${SC_DIR}/external_libraries/boost/libs/filesystem/src/*cpp)
endif()

if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN -DNOMINMAX -D_WIN32_WINNT=0x0600)

    #avoid unnecesary autolink
    add_definitions(-DBOOST_DATE_TIME_NO_LIB -DBOOST_ALL_NO_LIB)

    # use session manager to set shared memory location, see issue 2409
    add_definitions(-DBOOST_INTERPROCESS_BOOTSTAMP_IS_SESSION_MANAGER_BASED)
endif()

if(MSVC)
  foreach(flag CMAKE_C_FLAGS CMAKE_CXX_FLAGS)
    set(${flag} "${${flag}} /wd4018") # signed/unsigned mismatch
    set(${flag} "${${flag}} /wd4102") # unreferenced label
    set(${flag} "${${flag}} /wd4267") # conversion from 'size_t' to 'int', possible loss of data
    set(${flag} "${${flag}} /wd4244") # conversion from '__int64' to 'int', possible loss of data
    set(${flag} "${${flag}} /wd4305") # '=' : truncation from 'double' to 'float'
    set(${flag} "${${flag}} /wd4309") # '=' : truncation of constant value
    set(${flag} "${${flag}} /wd4800") # forcing value to bool 'true' or 'false' (performance warning)
    set(${flag} "${${flag}} /wd4996") # The POSIX name for this item is deprecated.
  endforeach()

# this flag causes MSVC to correctly report __cplusplus, which prevents a compiler error 
# caused by some versions of libsndfile's C++ header redefining nullptr as NULL. 
# See https://devblogs.microsoft.com/cppblog/msvc-now-correctly-reports-__cplusplus/
  add_compile_options("/Zc:__cplusplus")

# _ENABLE_ATOMIC_ALIGNMENT_FIX prevents the build from breaking when VS2015 update 2 upwards are used
# see http://boost.2283326.n4.nabble.com/lockfree-ENABLE-ATOMIC-ALIGNMENT-FIX-for-VS-2015-Update-2-td4685955.html
  add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -D_ENABLE_ATOMIC_ALIGNMENT_FIX)

  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif(MSVC)

# If SuperCollider has been built, use SC_Version.h otherwise use the forged version
IF (EXISTS ${SC_DIR}/build/common)
	include_directories(${SC_DIR}/build/common)
ELSE()
	include_directories(Source/generated)
ENDIF()

if (NOT Boost_FOUND)
	file(GLOB boost_system_sources ${SC_DIR}/external_libraries/boost/libs/system/src/*cpp)
	file(GLOB boost_filesystem_sources ${SC_DIR}/external_libraries/boost/libs/filesystem/src/*cpp)
endif()

list(APPEND CMAKE_CXX_SOURCE_FILE_EXTENSIONS c)

project(PluginCollider VERSION 0.1.0)

add_subdirectory(libs/JUCE)             # If you've put JUCE in a subdirectory called JUCE

juce_add_plugin(PluginCollider
    # VERSION ...                               # Set this if the plugin version is different to the project version
     ICON_BIG  Resources/ui/plugincollider_256x256.png
    # ICON_SMALL ...
    COMPANY_NAME "Digital Suburban"                 # Specify the name of the plugin's author
	BUNDLE_ID "com.digitalsuburban.PluginCollider"  # The bundle identifier for the plugin, if it should be a standalone bundle
    IS_SYNTH FALSE                                  # Is this a synth or an effect?
    NEEDS_MIDI_INPUT TRUE                           # Does the plugin need midi input?
    # NEEDS_MIDI_OUTPUT TRUE/FALSE                  # Does the plugin need midi output?
    # IS_MIDI_EFFECT TRUE/FALSE                     # Is this plugin a MIDI effect?
    # EDITOR_WANTS_KEYBOARD_FOCUS TRUE/FALSE        # Does the editor need keyboard focus?
    COPY_PLUGIN_AFTER_BUILD TRUE                    # Should the plugin be installed to a default location after building?
    PLUGIN_MANUFACTURER_CODE DGSB                   # A four-character manufacturer id with at least one upper-case character
    PLUGIN_CODE PLSC                                # A unique four-character plugin id with exactly one upper-case character
                                                    # GarageBand 10.3 requires the first letter to be upper-case, and the remaining letters to be lower-case
    FORMATS VST3 Standalone                         # The formats to build. Other valid formats are: AAX Unity VST AU AUv3
    PRODUCT_NAME "PluginCollider")                  # The name of the final executable, which can differ from the target name

	# juce_generate_juce_header(PluginCollider)

if (APPLE)
	target_sources(PluginCollider
	PRIVATE
    ${SC_DIR}/common/SC_Apple.mm
	${SC_DIR}/common/SC_Filesystem_macos.cpp)
endif (APPLE)

if (UNIX)
	target_sources(PluginCollider
	PRIVATE
	${SC_DIR}/common/SC_Filesystem_unix.cpp)
endif (UNIX)

if (WIN32)
	target_sources(PluginCollider
	PRIVATE
	${SC_DIR}/common/SC_Filesystem_win.cpp)
endif (WIN32)

if (NOT SC_DYNAMIC_PLUGINS)
target_sources(PluginCollider
    PRIVATE
	# sc plugins ==========================================
	${SC_DIR}/server/plugins/BinaryOpUGens.cpp
	${SC_DIR}/server/plugins/ChaosUGens.cpp
	${SC_DIR}/server/plugins/DelayUGens.cpp
	${SC_DIR}/server/plugins/DemandUGens.cpp
	${SC_DIR}/server/plugins/DemoUGens.cpp
	${SC_DIR}/server/plugins/DynNoiseUGens.cpp
	${SC_DIR}/server/plugins/FilterUGens.cpp
	${SC_DIR}/server/plugins/GendynUGens.cpp
	${SC_DIR}/server/plugins/GrainUGens.cpp
	${SC_DIR}/server/plugins/IOUGens.cpp
	${SC_DIR}/server/plugins/LFUGens.cpp
	${SC_DIR}/server/plugins/MulAddUGens.cpp
	${SC_DIR}/server/plugins/NoiseUGens.cpp
	${SC_DIR}/server/plugins/OscUGens.cpp
	${SC_DIR}/server/plugins/PanUGens.cpp
	${SC_DIR}/server/plugins/PhysicalModelingUGens.cpp
	${SC_DIR}/server/plugins/ReverbUGens.cpp
	${SC_DIR}/server/plugins/TestUGens.cpp
	${SC_DIR}/server/plugins/TriggerUGens.cpp
	${SC_DIR}/server/plugins/UnaryOpUGens.cpp
	${SC_DIR}/server/plugins/DiskIO_UGens.cpp
	${SC_DIR}/server/plugins/FFT_UGens.cpp
	${SC_DIR}/server/plugins/FFTInterfaceTable.cpp
	${SC_DIR}/server/plugins/FFT2InterfaceTable.cpp
	${SC_DIR}/server/plugins/PartitionedConvolution.cpp
	${SC_DIR}/server/plugins/PV_UGens.cpp)
endif()

target_sources(PluginCollider
    PRIVATE
	${BOOST_LIBS}
	${SC_DIR}/common/fftlib.c
	${SC_DIR}/common/SC_fftlib.cpp
	${SC_DIR}/common/SC_AllocPool.cpp
	${SC_DIR}/common/SC_Errors.cpp
	${SC_DIR}/common/Samp.cpp
	${SC_DIR}/common/SC_StringBuffer.cpp
	${SC_DIR}/common/SC_StringParser.cpp
	${SC_DIR}/common/sc_popen.cpp
	${SC_DIR}/common/strtod.c
	${SC_DIR}/common/SC_Reply.cpp

	# scsynth stuff =======================================
	${SC_DIR}/external_libraries/TLSF-2.4.6/src/tlsf.c
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
	${SC_DIR}/server/scsynth/SC_World.cpp

	# plugincollider itself =============================
	Source/UDPPort.cpp 
	Source/OSCMessages.cpp
    Source/PluginEditor.cpp
    Source/PluginProcessor.cpp
	Source/SCProcess.cpp
	Source/SCPluginDriver.cpp

	${boost_system_sources}
	${boost_filesystem_sources})

target_compile_definitions(PluginCollider
    PUBLIC
        # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
        JUCE_WEB_BROWSER=0  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_plugin` call
        JUCE_USE_CURL=0     # If you remove this, add `NEEDS_CURL TRUE` to the `juce_add_plugin` call
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0)

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
		)
