/*
    PluginCollider Copyright (c) 2025 Pascal Gauthier.
    SuperCollider real time audio synthesis system
    Copyright (c) 2002 James McCartney. All rights reserved.
    http://www.audiosynth.com

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include "PluginProcessor.h"
#include "SC_PlugIn.h"

static InterfaceTable* ft;

struct MouseInputUGen : public Unit {
    float m_y1, m_b1, m_lag;
};

class UIUGenJUCE : public juce::MouseListener {
    float r_width;
    float r_height;
    static UIUGenJUCE* instance;
public:
    static float mouseX, mouseY;
    static bool mouseButton;

    UIUGenJUCE() {
        juce::Rectangle<int> areaSize = juce::Desktop::getInstance().getDisplays().getMainDisplay().totalArea;
        juce::Desktop::getInstance().addGlobalMouseListener(this);
        r_width = 1.0 / (float)areaSize.getWidth();
        r_height = 1.0 / (float)areaSize.getHeight();
    }

    ~UIUGenJUCE() override {
        juce::Desktop::getInstance().removeGlobalMouseListener(instance);
    }

    static void setupListener() {
        juce::MessageManager::getInstance()->callAsync([]() {
            if (instance == nullptr) {
                instance = new UIUGenJUCE();
            }
        });
    }

    static void removeListener() {
        if (instance != nullptr) {
            delete instance;
            instance = nullptr;
        }
    }

    void mouseMove(const juce::MouseEvent &event) override {
        mouseX = (float)event.getScreenX() * r_width;
        mouseY = 1.f - ((float)event.getScreenY() * r_height);
        mouseButton = event.mods.getNumMouseButtonsDown() > 0;
    }
};

float UIUGenJUCE::mouseX = 0, UIUGenJUCE::mouseY = 0;
bool UIUGenJUCE::mouseButton = false;
UIUGenJUCE* UIUGenJUCE::instance = nullptr;

//////////////////////////////////////////////////////////////////////////////////////////////////

void MouseX_next(MouseInputUGen* unit, int inNumSamples) {
    // minval, maxval, warp, lag

    float minval = ZIN0(0);
    float maxval = ZIN0(1);
    float warp = ZIN0(2);
    float lag = ZIN0(3);

    float y1 = unit->m_y1;
    float b1 = unit->m_b1;

    if (lag != unit->m_lag) {
        unit->m_b1 = lag == 0.f ? 0.f : (float)exp(log001 / (lag * unit->mRate->mSampleRate));
        unit->m_lag = lag;
    }
    float y0 = UIUGenJUCE::mouseX;
    if (warp == 0.0) {
        y0 = (maxval - minval) * y0 + minval;
    } else {
        y0 = pow(maxval / minval, y0) * minval;
    }
    ZOUT0(0) = y1 = y0 + b1 * (y1 - y0);
    unit->m_y1 = zapgremlins(y1);
}

void MouseX_Ctor(MouseInputUGen* unit) {
    UIUGenJUCE::setupListener();
    SETCALC(MouseX_next);
    unit->m_b1 = 0.f;
    unit->m_lag = 0.f;
    MouseX_next(unit, 1);
}

void MouseY_next(MouseInputUGen* unit, int inNumSamples) {
    // minval, maxval, warp, lag

    float minval = ZIN0(0);
    float maxval = ZIN0(1);
    float warp = ZIN0(2);
    float lag = ZIN0(3);

    float y1 = unit->m_y1;
    float b1 = unit->m_b1;

    if (lag != unit->m_lag) {
        unit->m_b1 = lag == 0.f ? 0.f : (float)exp(log001 / (lag * unit->mRate->mSampleRate));
        unit->m_lag = lag;
    }
    float y0 = UIUGenJUCE::mouseY;
    if (warp == 0.0) {
        y0 = (maxval - minval) * y0 + minval;
    } else {
        y0 = pow(maxval / minval, y0) * minval;
    }
    ZOUT0(0) = y1 = y0 + b1 * (y1 - y0);
    unit->m_y1 = zapgremlins(y1);
}

void MouseY_Ctor(MouseInputUGen* unit) {
    UIUGenJUCE::setupListener();
    SETCALC(MouseY_next);
    unit->m_b1 = 0.f;
    unit->m_lag = 0.f;
    MouseY_next(unit, 1);
}

void MouseButton_next(MouseInputUGen* unit, int inNumSamples) {
    // minval, maxval, warp, lag

    float minval = ZIN0(0);
    float maxval = ZIN0(1);
    float lag = ZIN0(2);

    float y1 = unit->m_y1;
    float b1 = unit->m_b1;

    if (lag != unit->m_lag) {
        unit->m_b1 = lag == 0.f ? 0.f : (float)exp(log001 / (lag * unit->mRate->mSampleRate));
        unit->m_lag = lag;
    }
    float y0 = UIUGenJUCE::mouseButton ? maxval : minval;
    ZOUT0(0) = y1 = y0 + b1 * (y1 - y0);
    unit->m_y1 = zapgremlins(y1);
}

void MouseButton_Ctor(MouseInputUGen* unit) {
    UIUGenJUCE::setupListener();
    SETCALC(MouseButton_next);
    unit->m_b1 = 0.f;
    unit->m_lag = 0.f;
    MouseButton_next(unit, 1);
}

PluginLoad(UIUGens) {
    ft = inTable;
    DefineUnit("MouseX", sizeof(MouseInputUGen), (UnitCtorFunc)&MouseX_Ctor, 0, 0);
    DefineUnit("MouseY", sizeof(MouseInputUGen), (UnitCtorFunc)&MouseY_Ctor, 0, 0);
    DefineUnit("MouseButton", sizeof(MouseInputUGen), (UnitCtorFunc)&MouseButton_Ctor, 0, 0);
}

PluginUnload(UIUGens) {
    UIUGenJUCE::removeListener();
}
