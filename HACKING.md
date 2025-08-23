PluginCollider uses the JUCE ValueTree model to monitor and control the state of the plugin.  The ValueTree model is 
defined in [PluginModel.h](source/PluginModel.h) and [PluginModel.cpp](source/PluginModel.cpp). 

In the UI you can see the complete ValueTree structure under "Tools/Internal Plugin State". Carful when "Tool/Internal 
Plugin State" is used ; it can change the binary ValueTree type and unable to load afterward.  

There is some lock in PluginCollider, everything is design so the audio thread executes commands from a FIFO queue if there is no reply required by the SuperCollider engine.
