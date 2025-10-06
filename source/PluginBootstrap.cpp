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

#ifdef WIN32
#include <windows.h>


static std::string GetRegistryStringValueSingleCall(HKEY hRootKey, const std::string& subkey, const std::string& valueName) {
    DWORD dataSize = 0;
    LSTATUS status = RegGetValueA(hRootKey, subkey.c_str(), valueName.c_str(), RRF_RT_REG_SZ, NULL, NULL, &dataSize);
    if (status != ERROR_SUCCESS) {
        return "";
    }

    std::string value(dataSize, '\0');
    status = RegGetValueA(hRootKey, subkey.c_str(), valueName.c_str(), RRF_RT_REG_SZ, NULL, (LPBYTE)&value[0], &dataSize);
    if (status != ERROR_SUCCESS) {
        return "";
    }
    if (!value.empty() && value.back() == '\0') {
        value.pop_back();
    }
    return value;
}

void PluginColliderAudioProcessor::performBootstrap() {
    juce::PropertiesFile *prop = appProp.getUserSettings();
    pluginPath = prop->getValue("pluginPath", "C:\\Program Files\\SuperCollider-3.14.0\\plugins");

    synthDefPath = prop->getValue("synthPath", "");
    if ( synthDefPath.isEmpty() ) {
        juce::File userSynthDefs = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                                       .getChildFile("Application Support/SuperCollider/synthdefs");
        if ( userSynthDefs.exists() ) {
            prop->setValue("synthPath", userSynthDefs.getFullPathName());
            synthDefPath = userSynthDefs.getFullPathName();
        }
    }

    // First we check if it is already loaded
    HMODULE hModule = GetModuleHandle("sndfile.dll");
    if (hModule == NULL) {
        // Try to load it from the default path
        hModule = LoadLibrary("sndfile.dll");
        if (hModule == NULL) {
            // Not found, using PluginCollider install path from registry
            std::string installPath = GetRegistryStringValueSingleCall(HKEY_LOCAL_MACHINE, "SOFTWARE\\Digital Suburban\\PluginCollider", "InstallPath");
            if ( installPath.empty() ) {
                logger.scprintf("Could not find PluginCollider install path in registry, please rerun the installer\n");
                return;
            }
            juce::File dllPath = juce::File(installPath).getChildFile("sndfile.dll");
            
            if ( dllPath.exists() == false ) {
                logger.scprintf("Could not find sndfile.dll in PluginCollider install path, please rerun the installer\n");
                return;
            }
            juce::String fullPathName = dllPath.getFullPathName();
            hModule = LoadLibrary(fullPathName.toRawUTF8());
            if ( hModule == NULL ) {
                logger.scprintf("Could not load sndfile.dll\n");
                return;
            }
        }
    }

    bootstrapSucessfull = true;
}


#elif __APPLE__
void PluginColliderAudioProcessor::performBootstrap() {
    juce::PropertiesFile *prop = appProp.getUserSettings();    
    pluginPath = prop->getValue("pluginPath", "/Applications/SuperCollider.app/Contents/Resources/plugins");
    bootstrapSucessfull = true;    
}


#else
void PluginColliderAudioProcessor::performBootstrap() {
    juce::PropertiesFile *prop = appProp.getUserSettings();
    pluginPath = prop->getValue("pluginPath", "/usr/lib/SuperCollider/plugins");
    bootstrapSucessfull = true;    
}
#endif