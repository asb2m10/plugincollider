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
    juce::ValueTree vt;
    DynamicViewPanel &panel;

    void setItemName() {
        juce::String fxPrefix = vt.hasType(IDs::fxnode) ? "FX " : "";
        itemName = vt.getProperty(IDs::nodeid).toString() + ": " + fxPrefix + vt.getProperty(IDs::synthName).toString();
    }
public:
    SynthDefNodeItem(PluginColliderAudioProcessor &processor, juce::ValueTree node, DynamicViewPanel &panel) : processor(processor), vt(node), panel(panel) {
        setItemName();
        containsSubItems = false;
        vt.addListener(this);
    }

    void itemSelectionChanged(bool isNowSelected) override {
        if ( isNowSelected ) {
            panel.setEditableItem(vt, processor);
        } else {
            panel.clearEditableItem();
        }
    }

    juce::ValueTree getNodeValueTree() const {
        return vt;
    }

    juce::var getDragSourceDescription() override {
        return juce::var("synthdef");
    }

    void itemClicked(const juce::MouseEvent&event) override {
        if (event.mods.isPopupMenu()) {
            juce::PopupMenu menu;
            menu.addItem("Remove Node", true, false, [this] {
                panel.clearEditableItem();
                juce::ValueTree parent = vt.getParent();
                parent.removeChild(vt, nullptr);
            });
            menu.showMenuAsync(juce::PopupMenu::Options());
        }
    }

    void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property) override {
        if ( treeWhosePropertyHasChanged == vt && property == IDs::synthName ) {
            setItemName();
            repaintItem();
        }
    }
};

class GroupNodeItem : public PCTreeItem, public juce::ValueTree::Listener {
    PluginColliderAudioProcessor &processor;
    juce::ValueTree vt;
    DynamicViewPanel &panel;
public:
    GroupNodeItem(PluginColliderAudioProcessor &processor, juce::ValueTree node, DynamicViewPanel &panel) : processor(processor), vt(node), panel(panel) {
        if ( node.hasType(IDs::rootnode) ) {
            itemName = "1: Root Node";
        } else {
            itemName = node.getProperty(IDs::nodeid).toString() + ": " + node.getProperty(IDs::nodename).toString();
        }
        containsSubItems = true;
        vt.addListener(this);
    }

    void itemSelectionChanged(bool isNowSelected) override {
        if ( isNowSelected ) {
            panel.setEditableItem(vt, processor);
        } else {
            panel.clearEditableItem();
        }
    }

    void itemOpennessChanged(bool isNowOpen) {
        if ( isNowOpen ) {
            for(int i=0; i < vt.getNumChildren(); i++) {
                juce::ValueTree child = vt.getChild(i);
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
                vt.addChild(processor.createFxNodeVT(), -1, nullptr);

            });
            menu.addItem("Add SynthDef trigger by midi notes", true, false, [this] {
                setOpen(true);
                vt.addChild(processor.createMidiNoteNodeVT(), -1, nullptr);
            });
            menu.addItem("Add Group", true, false, [this] {
                setOpen(true);
                vt.addChild(processor.createGroupNodeVT(), -1, nullptr);
            });
            menu.addSeparator();
            if ( ! vt.hasType(IDs::rootnode) ) {
                menu.addItem("Remove Group", true, false, [this] {
                    panel.clearEditableItem();
                    juce::ValueTree parent = vt.getParent();
                    parent.removeChild(vt, nullptr);
                });
                menu.addSeparator();
            }
            menu.addItem("Re-sync configuration with server", true, false, [this] {
                processor.reloadNodeContainer();
            });

            menu.showMenuAsync(juce::PopupMenu::Options());
        }
    }

    bool isInterestedInDragSource (const juce::DragAndDropTarget::SourceDetails& dragSourceDetails) override {
        return dragSourceDetails.description == "group" || dragSourceDetails.description == "synthdef";
    }

    juce::var getDragSourceDescription() override {
        if ( vt.hasType(IDs::rootnode) )
            return juce::var();
        return juce::var("group");
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
                sourceNode = source->vt;
            }
        }

        if ( sourceNode.getParent().isValid() && vt != sourceNode && ! vt.isAChildOf(sourceNode) ) {
            if ( sourceNode.getParent() == vt ) {
                int currentIndex = vt.indexOf(sourceNode);
                vt.moveChild(currentIndex, insertIndex, nullptr);
                return;
            }
            sourceNode.getParent().removeChild(sourceNode, nullptr);
            vt.addChild(sourceNode, insertIndex, nullptr);
        }
    }

    void valueTreeChildRemoved(juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) override {
        if ( parentTree == vt && isOpen() ) {
            removeSubItem(indexFromWhichChildWasRemoved, true);
        }
    }

    void valueTreeChildAdded(juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenAdded) override {
        if ( parentTree == vt ) {
            juce::MessageManager::callAsync([this] {
                setOpenness(Openness::opennessClosed);
                setOpenness(Openness::opennessOpen);
            });
        }
    }

    void valueTreeChildOrderChanged(juce::ValueTree& parentTree, int oldIndex, int newIndex) override {
        if ( parentTree == vt && isOpen() ) {
            juce::TreeViewItem *moved = getSubItem(oldIndex);
            removeSubItem(oldIndex, false);
            addSubItem(moved, newIndex);
        }
    }
};

class ProjectItem : public PCTreeItem, public juce::ValueTree::Listener {
    PluginColliderAudioProcessor &audioProcessor;
    DynamicViewPanel &panel;

public:
    ProjectItem(PluginColliderAudioProcessor &processor, DynamicViewPanel &panel) : audioProcessor(processor), panel(panel) {
        itemName = "Project";
        processor.pluginState.addListener(this);
    }
    
    ~ProjectItem() {
        audioProcessor.pluginState.removeListener(this);
    }

    void valueTreeChildAdded(juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenAdded) override {
        if ( parentTree == audioProcessor.pluginState ) {
            juce::MessageManager::callAsync([this] {
                setOpenness(Openness::opennessClosed);
                setOpenness(Openness::opennessOpen);
            });
        }
    }

    void itemOpennessChanged(bool isNowOpen) override {
        if ( isNowOpen ) {
            //addSubItem(new PCTreeItem("Buffers"));
            addSubItem(new ControlBusItem(audioProcessor, panel));
            addSubItem(new GroupNodeItem(audioProcessor, audioProcessor.pluginState.getChildWithName(IDs::rootnode), panel));
            addSubItem(new ScratchpadItem(audioProcessor, panel));
        } else {
            clearSubItems();
        }
    }

    void itemClicked(const juce::MouseEvent&event) override {
        if (event.mods.isPopupMenu()) {
            juce::PopupMenu menu;
            menu.addItem("Reset Project", true, false, [this] {
                audioProcessor.resetPluginState();
                audioProcessor.reloadNodeContainer();
            });
            menu.showMenuAsync(juce::PopupMenu::Options());
        }
    }

    void valueTreeChildRemoved(juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) override {
        if ( childWhichHasBeenRemoved.getType() == IDs::rootnode ) {
            juce::MessageManager::callAsync([this] {
                panel.clearEditableItem();
                setOpenness(Openness::opennessClosed);
            });
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
            HeapStringList<64,4096> reply;
            audioProcessor.execSyncWorld([this, &reply]() {
                this->audioProcessor.superCollider.rt_getSynthDef(reply);
            });
            for(int i=0;i<reply.size();i++) {
                addSubItem(new PCTreeItem(reply.getItem(i)));
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
            if ( nodeId != 1 ) {
                menu.addItem("Free node", true, false, [this] {
                    processor.execOnAudioThread([this](PluginColliderAudioProcessor &proc) {
                        proc.superCollider.rt_freeNode(nodeId);
                    });
                    getParentItem()->setOpen(false);
                });
                menu.showMenuAsync(juce::PopupMenu::Options());
            }
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
            big_scpacket packet;
            SCErr err = 1;

            audioProcessor.execSyncWorld([this, &packet, &err]() {
                err = this->audioProcessor.superCollider.rt_queryTree(0, &packet, true);
            });
            
            if ( err == 0 ) {
                juce::OSCMessage msg = OSCMemoryBlock::parseMessage(packet.data(), packet.size());
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
    DynamicViewPanel &panel;
public:
    ServerItem(PluginColliderAudioProcessor &p, DynamicViewPanel &panel) : audioProcessor(p), panel(panel) {
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
                audioProcessor.rebootServer();
                setOpen(false);
            });
            menu.showMenuAsync(juce::PopupMenu::Options());
        }
    }

    void itemSelectionChanged(bool isNowSelected) override {
        if ( isNowSelected ) {
            juce::ValueTree vt = audioProcessor.pluginState.getChildWithName(IDs::srvRoot);
            panel.setEditableItem(vt, audioProcessor);
        } else {
            panel.clearEditableItem();
        }
    }
};

RootItem::RootItem(PluginColliderAudioProcessor &processor, DynamicViewPanel &panel) {
    setOpen(true);
    itemName = "PluginCollider";
    addSubItem(new ProjectItem(processor, panel));
    addSubItem(new ServerItem(processor, panel));
}