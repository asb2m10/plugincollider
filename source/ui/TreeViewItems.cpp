#pragma once

#include "TreeViewItems.h"

class RootPCGroup : public PCTreeItem {
public:
    RootPCGroup() {
        itemName = "1 - Root group";
        PCTreeItem *group = new PCTreeItem("5 - Fx group");
        group->addSubItem(new PCTreeItem("10 - Midi note group"));
        addSubItem(group);
    }
};

class ProjectItem : public PCTreeItem {
    PluginColliderAudioProcessor &audioProcessor;
public:
    ProjectItem(PluginColliderAudioProcessor &p) : audioProcessor(p) {
        itemName = "Project";

        PCTreeItem *rootGroup = new RootPCGroup();
        PCTreeItem *group = new PCTreeItem("Groups setup");
        group->addSubItem(rootGroup);

        addSubItem(new PCTreeItem("Buffers"));
        addSubItem(group);
        addSubItem(new PCTreeItem("SynthDefs"));
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

class ServerItem : public PCTreeItem {
    PluginColliderAudioProcessor &audioProcessor;
public:
    ServerItem(PluginColliderAudioProcessor &p) : audioProcessor(p) {
        itemName = "Server";

        addSubItem(new PCTreeItem("Buffers"));
        addSubItem(new PCTreeItem("Nodes"));
        addSubItem(new PCTreeItem("SynthDefs"));
        addSubItem(new PCTreeItem("Units"));
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

RootItem::RootItem(PluginColliderAudioProcessor &p) {
    setOpen(true);

    addSubItem(new ProjectItem(p));
    addSubItem(new ServerItem(p));
}