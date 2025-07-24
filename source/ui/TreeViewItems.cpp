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

#pragma once

#include "TreeViewItems.h"
#include "SC_OscUtils.hpp"


// class ProjectSynthDefItem : public PCTreeItem {
//     PluginColliderAudioProcessor &audioProcessor;
// public:
//     ProjectSynthDefItem(PluginColliderAudioProcessor &p) : audioProcessor(p) {
//         itemName = "SynthDefs";
//     }

//     void itemOpennessChanged(bool isNowOpen) override {
//         if ( isNowOpen ) {
//             juce::ValueTree vt = audioProcessor.pluginState.getChildWithName(IDs::synths);

//             if ( vt.isValid() ) {
//                 for(int i=0;i<vt.getNumChildren();i++) {
//                     juce::ValueTree synth = vt.getChild(i);
//                     if ( synth.hasType(IDs::synth) ) {
//                         juce::String name = synth.getProperty(IDs::synthName);
//                         addSubItem(new PCTreeItem(name, false));
//                     }
//                 }
//             }
//         } else {
//             clearSubItems();
//         }
//     }
// };


class ScratchpadItem : public PCTreeItem {
    PluginColliderAudioProcessor &audioProcessor;
    DynamicViewPanel &panel;
public:
    ScratchpadItem(PluginColliderAudioProcessor &processor, DynamicViewPanel &panel) : audioProcessor(processor), panel(panel) {
        itemName = "Scratchpad";
        containsSubItems = false;
    }

    void itemSelectionChanged(bool isNowSelected) override {
        if ( isNowSelected ) {
            juce::ValueTree vt = audioProcessor.pluginState.getChildWithName(IDs::scratchpad);
            panel.setEditableItem(vt, audioProcessor);
        } else {
            panel.clearEditableItem();
        }
    }
};


class ControlBusItem : public PCTreeItem {
    PluginColliderAudioProcessor &audioProcessor;
    DynamicViewPanel &panel;
public:
    ControlBusItem(PluginColliderAudioProcessor &processor, DynamicViewPanel &panel) : audioProcessor(processor), panel(panel) {
        itemName = "Control Bus";
        containsSubItems = false;
    }

    void itemSelectionChanged(bool isNowSelected) override {
        if ( isNowSelected ) {
            juce::ValueTree vt = audioProcessor.pluginState.getChildWithName(IDs::controlbuses);
            panel.setEditableItem(vt, audioProcessor);
        } else {
            panel.clearEditableItem();
        }
    }
};

class SynthDefNodeItem : public PCTreeItem, public juce::ValueTree::Listener {
    PluginColliderAudioProcessor &processor;
    juce::ValueTree node;
    DynamicViewPanel &panel;

    void setItemName() {
        juce::String fxPrefix = node.hasType(IDs::fxnode) ? "FX " : "";
        itemName = node.getProperty(IDs::nodeid).toString() + ": " + fxPrefix + node.getProperty(IDs::synthName).toString();
    }
public:
    SynthDefNodeItem(PluginColliderAudioProcessor &processor, juce::ValueTree node, DynamicViewPanel &panel) : processor(processor), node(node), panel(panel) {
        setItemName();
        containsSubItems = false;
        this->node.addListener(this);
    }

    void itemSelectionChanged(bool isNowSelected) override {
        if ( isNowSelected ) {
            panel.setEditableItem(node, processor);
        } else {
            panel.clearEditableItem();
        }
    }

    juce::ValueTree getNodeValueTree() const {
        return node;
    }

    juce::var getDragSourceDescription() override {
        return juce::var("synthdef");
    }

    void itemClicked(const juce::MouseEvent&event) override {
        if (event.mods.isPopupMenu()) {
            juce::PopupMenu menu;
            menu.addItem("Remove Node", true, false, [this] {
                panel.clearEditableItem();
                juce::ValueTree parent = node.getParent();
                parent.removeChild(node, nullptr);
                getParentItem()->clearSubItems();
            });
            menu.showMenuAsync(juce::PopupMenu::Options());
        }
    }

    void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property) override {
        if ( treeWhosePropertyHasChanged == node && property == IDs::synthName ) {
            setItemName();
            repaintItem();
        }
    }
};

class GroupNodeItem : public PCTreeItem, public juce::ValueTree::Listener {
    PluginColliderAudioProcessor &processor;
    juce::ValueTree node;
    DynamicViewPanel &panel;
public:
    bool nowDiscared = false;

    GroupNodeItem(PluginColliderAudioProcessor &processor, juce::ValueTree node, DynamicViewPanel &panel) : processor(processor), node(node), panel(panel) {
        if ( node.hasType(IDs::rootnode) ) {
            itemName = "1: Root Node";
        } else {
            itemName = node.getProperty(IDs::nodeid).toString() + ": " + node.getProperty(IDs::nodename).toString();
        }
        containsSubItems = true;
        this->node.addListener(this);
    }

    bool isInterestedInDragSource (const juce::DragAndDropTarget::SourceDetails& dragSourceDetails) override {
        return dragSourceDetails.description == "group" || dragSourceDetails.description == "synthdef";
    }
    
    void itemDropped(const juce::DragAndDropTarget::SourceDetails& source, int insertIndex) override {
        juce::TreeView *owner = getOwnerView();
        juce::TreeViewItem *sourceItem = owner->getSelectedItem(0);
        juce::ValueTree sourceNode;
        if ( source.description == "synthdef" ) {
            SynthDefNodeItem* source = dynamic_cast<SynthDefNodeItem*>(sourceItem);
            if ( source != nullptr ) {
                sourceNode = source->getNodeValueTree();
            }
        }
        if ( source.description == "group" ) {
            GroupNodeItem* source = dynamic_cast<GroupNodeItem*>(sourceItem);
            if ( source != nullptr ) {
                sourceNode = source->node;
            } 
        }

        if ( ! sourceNode.isValid() )
            return;

        if ( sourceNode.getParent().isValid() && node != sourceNode && ! node.isAChildOf(sourceNode) ) {
            if ( sourceNode.getParent() == node && node.indexOf(sourceNode) < insertIndex )
                --insertIndex;

            sourceNode.getParent().removeChild(sourceNode, nullptr);
            node.addChild(sourceNode, insertIndex, nullptr);
        }
    }

    juce::var getDragSourceDescription() override {
        if ( node.hasType(IDs::rootnode) )
            return juce::var();
        return juce::var("group");
    }

    void itemOpennessChanged(bool isNowOpen) {
        if ( isNowOpen ) {
            for(int i=0; i < node.getNumChildren(); i++) {
                juce::ValueTree child = node.getChild(i);
                if ( child.hasType(IDs::groupnode) ) {
                    addSubItem(new GroupNodeItem(processor, child, panel), -1);
                } else {
                    addSubItem(new SynthDefNodeItem(processor, child, panel), -1);
                }
            }
        } else {
            clearSubItems();
        }
    }

    void itemClicked(const juce::MouseEvent&event) override {
        if (event.mods.isPopupMenu()) {
            juce::PopupMenu menu;
            menu.addItem("Add SynthDef effect", true, false, [this] {
                setOpen(true);                
                juce::ValueTree newSynth = juce::ValueTree(IDs::fxnode);
                newSynth.setProperty(IDs::nodename, "FX Node", nullptr);
                newSynth.setProperty(IDs::nodeid, processor.getFreeNodeId(), nullptr);
                node.addChild(newSynth, -1, nullptr);

            });
            menu.addItem("Add SynthDef trigger by midi notes", true, false, [this] {
                setOpen(true);                
                juce::ValueTree newSynth = juce::ValueTree(IDs::notenode);
                newSynth.setProperty(IDs::nodename, "Midi Note Node", nullptr);
                newSynth.setProperty(IDs::nodeid, processor.getFreeNodeId(), nullptr);
                node.addChild(newSynth, -1, nullptr);
            });
            menu.addItem("Add Group", true, false, [this] {
                setOpen(true);                
                juce::ValueTree newSynth = juce::ValueTree(IDs::groupnode);
                newSynth.setProperty(IDs::nodename, "Group", nullptr);
                newSynth.setProperty(IDs::nodeid, processor.getFreeNodeId(), nullptr);  
                node.addChild(newSynth, -1, nullptr);
            });
            menu.addSeparator();
            if ( ! node.hasType(IDs::rootnode) ) {
                menu.addItem("Remove Group", true, false, [this] {
                    panel.clearEditableItem();
                    juce::ValueTree parent = node.getParent();
                    parent.removeChild(node, nullptr);        
                });
                menu.addSeparator();
            }
            menu.addItem("Re-sync configuration with server", true, false, [this] {
                processor.reloadNodeContainer();
            });

            menu.showMenuAsync(juce::PopupMenu::Options());
        }
    }

    void valueTreeChildRemoved(juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) override {
        if ( getOwnerView() == nullptr )
            return;

        if ( parentTree == node ) {
            // Delete the item from the message thread to avoid deleting the caller
            juce::MessageManager::callAsync([this, childWhichHasBeenRemoved ] {
                setOpen(false);
                setOpen(true);
            });
        }
    }

    void valueTreeChildAdded(juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenAdded) override {
        if ( parentTree == node ) {
            juce::MessageManager::callAsync([this, childWhichHasBeenAdded] {
                // The item might been closed from a delete, ignore this...
                if ( ! isOpen() )
                    return;                
                setOpen(false);
                setOpen(true);
            });
        }
    }

    void valueTreeChildOrderChanged(juce::ValueTree& parentTree, int oldIndex, int newIndex) override {
        if ( ! isOpen() )
            return;

        if ( parentTree == node ) {
            setOpen(false);
            setOpen(true);
        }
    }
};

class ProjectItem : public PCTreeItem {
    PluginColliderAudioProcessor &audioProcessor;
    DynamicViewPanel &panel;

public:
    ProjectItem(PluginColliderAudioProcessor &processor, DynamicViewPanel &panel) : audioProcessor(processor), panel(panel) {
        itemName = "Project";

        //addSubItem(new PCTreeItem("Buffers"));
        addSubItem(new ControlBusItem(processor, panel));
        addSubItem(new GroupNodeItem(processor, processor.pluginState.getChildWithName(IDs::rootnode), panel));
        addSubItem(new ScratchpadItem(processor, panel));
    }

    void itemClicked(const juce::MouseEvent&event) override {
        if (event.mods.isPopupMenu()) {
            juce::PopupMenu menu;
            menu.addItem("Reset Project", true, false, [this] {
                // Clear group nodes before resetting whole node tree
                getSubItem(1)->setOpen(false);
                audioProcessor.resetPluginState();
                audioProcessor.superCollider.reboot();
            });
            menu.showMenuAsync(juce::PopupMenu::Options());
        }
    }
};

/* ------------------------------------------------------------------------- */

class UnitItem : public PCTreeItem {
    PluginColliderAudioProcessor &audioProcessor;
public:
    UnitItem(PluginColliderAudioProcessor &p) : audioProcessor(p) {
        itemName = "Unit";
    }

    void itemOpennessChanged(bool isNowOpen) override {
        if ( isNowOpen ) {
            juce::StringArray units = audioProcessor.superCollider.getRegistredUnits();
            units.sort(true);
            for(int i=0;i<units.size();i++)
                addSubItem(new PCTreeItem(units[i], false));
        } else {
            clearSubItems();
        }
    }
};

class SynthDefsServerItem : public PCTreeItem {
    PluginColliderAudioProcessor &audioProcessor;
public:
    SynthDefsServerItem(PluginColliderAudioProcessor &p) : audioProcessor(p) {
        itemName = "SynthDefs";
    }

    void itemOpennessChanged(bool isNowOpen) override {
        if ( isNowOpen ) {
            ASyncReply<HeapStringList<64,4096>> reply;
            audioProcessor.command.push([this, &reply](PluginColliderAudioProcessor &proc) {
                proc.superCollider.rt_getSynthDef(reply.content);
                reply.notify(0);
            });
            reply.wait();
            for(int i=0;i<reply.content.size();i++) {
                addSubItem(new PCTreeItem(reply.content.getItem(i)));
            }            
        } else {
            clearSubItems();
        }
    }
};

class NodeSynthTreeItem : public PCTreeItem {
    PluginColliderAudioProcessor &processor;
    int nodeId;
public:
    NodeSynthTreeItem(PluginColliderAudioProcessor &processor,  OSCArgumentWalker &walker, int nodeId) : processor(processor), nodeId(nodeId) {
        itemName = juce::String("Synth ") + walker.getString();
        walker.next();

        int numberItems = walker.getInt();
        walker.next();
        for(int i=0;i<numberItems;i++) {
            addSubItem(new PCTreeItem(walker.getString(), false));
            walker.next();
            walker.next();
        }
    }
};

class NodeTreeItem : public PCTreeItem {
    OSCArgumentWalker walker;
    PluginColliderAudioProcessor &processor;
    int nodeId;
public:
    NodeTreeItem(PluginColliderAudioProcessor &processor, OSCArgumentWalker &walker) : processor(processor), walker(walker) {
        nodeId = walker.getInt();
        walker.next();

        int numberOfChild = walker.getInt();
        walker.next();

        if ( numberOfChild < 0 ) {
            itemName = "Node " + juce::String(nodeId);
            addSubItem(new NodeSynthTreeItem(processor, walker, nodeId));
        } else {
            itemName = "Group " + juce::String(nodeId);

            for(int i=0;i<numberOfChild;i++) {
                addSubItem(new NodeTreeItem(processor, walker));
            }
        }
    }

    void itemClicked(const juce::MouseEvent&event) override {
        if (event.mods.isPopupMenu()) {
            juce::PopupMenu menu;
            /*
            menu.addItem("Set value...", true, false, [this] {
            });

            juce::PopupMenu controlBusSelection;
            juce::ValueTree cbVt = processor.pluginState.getChildWithName(IDs::controlbuses);
            for(int i=0;i<cbVt.getNumChildren();i++) {
                juce::ValueTree cb = cbVt.getChild(i);
                juce::String name = cb.getProperty(IDs::cbName);
                int idx = cb.getProperty(IDs::cbIdx);
                controlBusSelection.addItem(name, true, false, [this, idx] {
                    processor.command.push([this, idx](PluginColliderAudioProcessor &proc) {
                        // TODO: set this based on parameter idx
                        proc.superCollider.rt_assignControlBus(nodeId, 1, idx);
                    });
                });
            }
            menu.addSubMenu("Map to control bus...", controlBusSelection);
            menu.addSeparator();
            */
            menu.addItem("Free node", true, false, [this] {
                processor.command.push([this](PluginColliderAudioProcessor &proc) {
                    proc.superCollider.rt_freeNode(nodeId);
                });
                getParentItem()->setOpen(false);
            });
            menu.showMenuAsync(juce::PopupMenu::Options());
        }
    }
};

class NodeTreeRoot : public PCTreeItem {
    PluginColliderAudioProcessor &audioProcessor;
public:
    NodeTreeRoot(PluginColliderAudioProcessor &p) : audioProcessor(p)  {
        itemName = "Nodes";
    }

    void itemOpennessChanged(bool isNowOpen) override {
        if ( isNowOpen ) {
            ASyncReply<big_scpacket> reply;
            audioProcessor.command.push([this, &reply](PluginColliderAudioProcessor &proc) {
                reply.notify(proc.superCollider.rt_queryTree(0, &reply.content, true));
            });
            if ( reply.wait() == 0 ) {
                juce::OSCMessage msg = OSCMemoryBlock::parseMessage(reply.content.data(), reply.content.size());
                OSCArgumentWalker walker(msg);
                // if synthControl value included
                walker.next();
                // node id of the request group
                walker.next();
                // num of child
                walker.next();
                addSubItem(new NodeTreeItem(audioProcessor, walker));
            }
        } else {
            clearSubItems();
        }
    }
};

class ServerItem : public PCTreeItem {
    PluginColliderAudioProcessor &audioProcessor;
public:
    ServerItem(PluginColliderAudioProcessor &p) : audioProcessor(p) {
        itemName = "Server";

        //addSubItem(new PCTreeItem("Buffers"));
        addSubItem(new NodeTreeRoot(p));
        addSubItem(new SynthDefsServerItem(p));
        addSubItem(new UnitItem(p));
    }

    void itemClicked(const juce::MouseEvent&event) override {
        if (event.mods.isPopupMenu()) {
            juce::PopupMenu menu;
            menu.addItem("Reboot SuperCollider server", true, false, [this] {
                audioProcessor.superCollider.reboot();
                setOpen(false);
            });
            menu.showMenuAsync(juce::PopupMenu::Options());
        }
    }
};

RootItem::RootItem(PluginColliderAudioProcessor &processor, DynamicViewPanel &panel) {
    setOpen(true);
    itemName = "PluginCollider";
    addSubItem(new ProjectItem(processor, panel));
    addSubItem(new ServerItem(processor));
}