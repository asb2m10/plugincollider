PluginCollider uses the JUCE ValueTree model to monitor and control the state of the plugin.  The ValueTree model is 
defined in [PluginModel.h](source/PluginModel.h) and [PluginModel.cpp](source/PluginModel.cpp). In the UI you can see 
the complete ValueTree structure under "Tools/Internal Plugin State".

There is no lock in PluginCollider, everything is design so the audio thread executes commands from a FIFO queue
that the UI thread can write to; see [CommandFifo.h](source/CommandFifo.h). From there any APIs that are required to be 
called from the audio thread are prefixed with `rt_` (for real-time).

If the UI thread needs a reply from the audio thread, ASyncReply can be used and it is designed so the memory allocation
is done in the UI thread and the audio thread simply writes to the pre-allocated memory.

For example, when a users changes nodes configuration, the nodes will be re-rendered into a
pre-compiled state where the audio thread can execute the node graph without any further allocations.

```c++
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
    // since we swap the container, the unique_ptr will automatically free the old one
}
```

In this example, the nodes are read from the ValueTree (in the UI thread) to a new `NodeContainer` which is then swapped with the
current one in the audio thread. The old `NodeContainer` is automatically freed when the unique_ptr goes out of scope.    
