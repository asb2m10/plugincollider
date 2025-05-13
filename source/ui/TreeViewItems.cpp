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

class ProjectSynthDefItem : public PCTreeItem {
    PluginColliderAudioProcessor &audioProcessor;
public:
    ProjectSynthDefItem(PluginColliderAudioProcessor &p) : audioProcessor(p) {
        itemName = "SynthDefs";
    }

    void itemOpennessChanged(bool isNowOpen) {
        if ( isNowOpen ) {
            juce::ValueTree vt = audioProcessor.pluginState.getChildWithName(IDs::synths);

            if ( vt.isValid() ) {
                for(int i=0;i<vt.getNumChildren();i++) {
                    juce::ValueTree synth = vt.getChild(i);
                    if ( synth.hasType(IDs::synth) ) {
                        juce::String name = synth.getProperty(IDs::synthName);
                        addSubItem(new PCTreeItem(name, false));
                    }
                }
            }
        } else {
            clearSubItems();
        }
    }
};

class ControlBusItem : public PCTreeItem {
    PluginColliderAudioProcessor &audioProcessor;
    DynamicViewPanel &panel;
public:
    ControlBusItem(PluginColliderAudioProcessor &processor, DynamicViewPanel &panel) : audioProcessor(processor), panel(panel) {
        itemName = "Control Busses";
        containsSubItems = false;
    }

    void itemSelectionChanged(bool isNowSelected) {
        if ( isNowSelected ) {
            juce::ValueTree vt = audioProcessor.pluginState.getChildWithName(IDs::controlbuses);
            panel.setEditableItem(vt);
        } else {
            panel.clearEditableItem();
        }
    }
};

class ProjectItem : public PCTreeItem {
    PluginColliderAudioProcessor &audioProcessor;
    DynamicViewPanel &panel;
public:
    ProjectItem(PluginColliderAudioProcessor &processor, DynamicViewPanel &panel) : audioProcessor(processor), panel(panel) {
        itemName = "Project";

        PCTreeItem *group = new PCTreeItem("Node 1 - Root Group");
        PCTreeItem *fx = new PCTreeItem("Node 5 - Fx group");
        group->addSubItem(fx);
        fx->addSubItem(new PCTreeItem("Node 10 - Midi note group"));

        addSubItem(new PCTreeItem("Buffers"));
        addSubItem(new ControlBusItem(processor, panel));
        addSubItem(group);
        addSubItem(new ProjectSynthDefItem(processor));
    }

    void itemClicked(const juce::MouseEvent&event) override {
        if (event.mods.isPopupMenu()) {
            juce::PopupMenu menu;
            menu.addItem("Reset Project", true, false, [this] {
                audioProcessor.superCollider.reboot();
                setOpen(false);
            });
            menu.addSeparator();
            menu.addItem("Sync project with server", true, false, [this] {
            });
            menu.showMenuAsync(juce::PopupMenu::Options());
        }
    }
};    

class UnitItem : public PCTreeItem {
    PluginColliderAudioProcessor &audioProcessor;
public:
    UnitItem(PluginColliderAudioProcessor &p) : audioProcessor(p) {
        itemName = "Unit";
    }

    void itemOpennessChanged(bool isNowOpen) {
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

            juce::String synthDefName = walker.getString();
            walker.next();
            addSubItem(new PCTreeItem(synthDefName, false));
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
            menu.addItem("Set value...", true, false, [this] {
            });
            menu.addItem("Map to control bus...", true, false, [this] {
            });
            menu.addSeparator();
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

    void itemOpennessChanged(bool isNowOpen) {
        if ( isNowOpen ) {
            ASyncReply<big_scpacket> reply;
            audioProcessor.command.push([this, &reply](PluginColliderAudioProcessor &proc) {
                reply.rc = proc.superCollider.rt_queryTree(0, &reply.content, false);
                reply.notify();
            });
            reply.wait();
            if ( reply.rc == 0 ) {
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

        addSubItem(new PCTreeItem("Buffers"));
        addSubItem(new NodeTreeRoot(p));
        addSubItem(new PCTreeItem("SynthDefs"));
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
    addSubItem(new ProjectItem(processor, panel));
    addSubItem(new ServerItem(processor));
}