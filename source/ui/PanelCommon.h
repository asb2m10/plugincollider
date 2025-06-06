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

class RangeEditor : public juce::Component {
    const int GLOBAL_RANGE = 100000;
    juce::Value refValue;

    juce::Slider low;
    juce::Label label;
    juce::Slider high;
    juce::Label stepLabel;
    juce::Slider step;

    void publishRange() {
        juce::String value = juce::String::formatted("%f %f %f", low.getValue(), high.getValue(), step.getValue());
        if ( ! refValue.getValue().isVoid() ) {
            refValue.setValue(value);
        }
    }

public:
    RangeEditor() {
        low.setRange(-GLOBAL_RANGE, GLOBAL_RANGE, 0.1);
        low.setSliderStyle(juce::Slider::SliderStyle::LinearBarVertical);
        low.setSliderSnapsToMousePosition(false);
        low.setColour(juce::Slider::trackColourId, juce::Colours::transparentBlack);
        addAndMakeVisible(low);
        
        label.setText("-", juce::NotificationType::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(label);
        
        high.setRange(-GLOBAL_RANGE, GLOBAL_RANGE, 0.1);
        high.setValue(1, juce::NotificationType::dontSendNotification);
        high.setSliderStyle(juce::Slider::SliderStyle::LinearBarVertical);
        high.setSliderSnapsToMousePosition(false);
        high.setColour(juce::Slider::trackColourId, juce::Colours::transparentBlack);        
        addAndMakeVisible(high);

        stepLabel.setText("Step", juce::NotificationType::dontSendNotification);
        stepLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(stepLabel);
        
        step.setSliderStyle(juce::Slider::SliderStyle::LinearBarVertical);
        step.setSliderSnapsToMousePosition(false);
        step.setColour(juce::Slider::trackColourId, juce::Colours::transparentBlack);                
        step.setRange(0.001, 5.0, 0.001);
        addAndMakeVisible(step);

        low.onValueChange = [this]() {
            if ( low.getValue() > high.getValue() ) {
                low.setValue(high.getValue() - 1, juce::NotificationType::sendNotificationSync);
            }
            publishRange();
        };

        high.onValueChange = [this]() {
            if ( high.getValue() < low.getValue() ) {
                high.setValue(low.getValue() + 1, juce::NotificationType::sendNotificationSync);
            }
            publishRange();
        };

        step.onValueChange = [this]() {
            publishRange();
        };
    }

    void assignValueTree(juce::ValueTree vt, juce::Identifier paramId) {
        if ( vt.isValid() && vt.hasProperty(paramId) ) {
            refValue.referTo(vt.getPropertyAsValue(paramId, nullptr));

            PluginColliderRange range(vt.getProperty(paramId));
            low.setValue(range.start, juce::NotificationType::sendNotificationSync);
            high.setValue(range.end, juce::NotificationType::sendNotificationSync);
            if ( high.getValue() <= low.getValue() ) {
                high.setValue(low.getValue() + 1, juce::NotificationType::sendNotificationSync);
            }
            step.setValue(range.interval, juce::NotificationType::sendNotificationSync);
        }
    }

    void resized() override {
        auto bounds = getLocalBounds();
        low.setBounds(bounds.removeFromLeft(60));
        label.setBounds(bounds.removeFromLeft(20));
        high.setBounds(bounds.removeFromLeft(60));
        stepLabel.setBounds(bounds.removeFromLeft(40));
        step.setBounds(bounds.removeFromLeft(45));
    }
};

class SliderRangeAware : public juce::Slider, public juce::Value::Listener {
    juce::Value value;

    void readRange() {
        PluginColliderRange range(value);
        setRange(range.start, range.end, range.interval);
        float currentValue = getValue();
        if (currentValue < range.start || currentValue > range.end) {
            setValue(range.start, juce::NotificationType::sendNotification);
        }
    }

public:
    void setRangeProperty(juce::ValueTree vt, juce::Identifier paramId) {
        if (vt.isValid() && vt.hasProperty(paramId)) {
            value = vt.getPropertyAsValue(paramId, nullptr);
            value.addListener(this);
            value.referTo(vt.getPropertyAsValue(paramId, nullptr));
            readRange();
        }
    }

    void valueChanged(juce::Value& value) override {
        readRange();
    }
};

/**
 * TableListBox where the content is mapped on a ValueTree content.
 */
class VTTableList : public juce::Component, public juce::TableListBoxModel {
protected:
    std::vector<juce::Identifier> columnIds;
    juce::TableListBox table;
    juce::Font font { juce::FontOptions { 14.0f } };
    int creationFlags = juce::TableHeaderComponent::ColumnPropertyFlags::visible | juce::TableHeaderComponent::ColumnPropertyFlags::resizable;
    juce::ValueTree vt;
public:
    VTTableList() {
        addAndMakeVisible (table);
        table.setModel(this);
        table.getHeader().setStretchToFitActive(true);
    }

    ~VTTableList() override {
        table.setModel(nullptr);
    }

    void setContent(juce::ValueTree vt) {
        // Force empty table to free previous components that was assigned to a preivous ValueTree
        juce::ValueTree emptyTree;
        this->vt = emptyTree;
        table.updateContent();

        this->vt = vt;
        table.updateContent();
    }

    void addColumn(juce::Identifier id, const juce::String& name, int width) {
        columnIds.push_back(id);
        table.getHeader().addColumn(name, columnIds.size(), width, 0.25*width, 3*width, creationFlags);
    }

    int getNumColumns() {
        return table.getHeader().getNumColumns(true);
    }

    int getNumRows() {
        if ( ! vt.isValid() )
            return 0;
        return vt.getNumChildren();
    }

    void resized() override {
        table.setBounds(getLocalBounds());
    }

    juce::String getCellText(const int columnNumber, const int rowNumber) {
        return vt.getChild(rowNumber).getProperty(columnIds[columnNumber - 1]);
    }

    void setCellText(const int columnNumber, const int rowNumber, const juce::String& newText) {
        vt.getChild(rowNumber).setProperty(columnIds[columnNumber - 1], newText, nullptr);
    }

    void paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override {
        g.setColour(getLookAndFeel().findColour(juce::ListBox::textColourId));
        g.setFont(font);
        g.drawText(getCellText(columnId, rowNumber), 2, 0, width - 4, height, juce::Justification::centredLeft, true);
        g.setColour(getLookAndFeel().findColour (juce::ListBox::backgroundColourId));
        g.fillRect(width - 1, 0, 1, height);
    }

    void paintRowBackground(juce::Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected) override {
    }

    void refresh() {
        table.updateContent();
    }

protected:
    class EditableTextCustomComponent final : public juce::Label  {
    public:
        EditableTextCustomComponent(VTTableList& td)  : owner (td)  {
            setEditable (false, true, false);
        }

        void mouseDown(const juce::MouseEvent& event) override {
            owner.table.selectRowsBasedOnModifierKeys(row, event.mods, false);
            juce::Label::mouseDown(event);
        }

        void textWasEdited() override {
            owner.setCellText(columnId, row, getText());
        }

        void setRowAndColumn(const int newRow, const int newColumn) {
            row = newRow;
            columnId = newColumn;
            setText(owner.getCellText(columnId, row), juce::NotificationType::dontSendNotification);
        }

        void paint(juce::Graphics& g) override {
            auto& lf = getLookAndFeel();
            if (! dynamic_cast<juce::LookAndFeel_V4*> (&lf))
                lf.setColour (textColourId, juce::Colours::black);
            Label::paint (g);
        }

    private:
        VTTableList& owner;
        int row, columnId;
        juce::Colour textColour;
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VTTableList)
};
