#include "InputHelper.h"

static const char* styleNames[] = {
    "Pro Controller",
    "Joy-Con controller in handheld mode",
    "Joy-Con controller in dual mode",
    "Joy-Con left controller in single mode",
    "Joy-Con right controller in single mode",
    "GameCube controller",
    "Poké Ball Plus controller",
    "NES/Famicom controller",
    "NES/Famicom controller in handheld mode",
    "SNES controller",
    "N64 controller",
    "Sega Genesis controller",
    "generic external controller",
    "generic controller",
};

nn::hid::NpadBaseState InputHelper::prevControllerState {};
nn::hid::NpadBaseState InputHelper::curControllerState {};

ulong InputHelper::selectedPort = -1;
bool InputHelper::isReadInput = true;
bool InputHelper::toggleInput = false;
bool InputHelper::disableMouse = false;

const char* getStyleName(nn::hid::NpadStyleSet style) {
    s32 index = -1;

    if (style.Test((int)nn::hid::NpadStyleTag::NpadStyleFullKey)) {
        index = 0;
    }
    if (style.Test((int)nn::hid::NpadStyleTag::NpadStyleHandheld)) {
        index = 1;
    }
    if (style.Test((int)nn::hid::NpadStyleTag::NpadStyleJoyDual)) {
        index = 2;
    }
    if (style.Test((int)nn::hid::NpadStyleTag::NpadStyleJoyLeft)) {
        index = 3;
    }
    if (style.Test((int)nn::hid::NpadStyleTag::NpadStyleJoyRight)) {
        index = 4;
    }
    if (style.Test((int)nn::hid::NpadStyleTag::NpadStyleSystemExt)) {
        index = 12;
    }
    if (style.Test((int)nn::hid::NpadStyleTag::NpadStyleSystem)) {
        index = 13;
    }

    if (index != -1) {
        return styleNames[index];
    } else {
        return "Unknown";
    }
}

void InputHelper::updatePadState() {
    prevControllerState = curControllerState;
    tryGetContState(&curControllerState, selectedPort);
}

bool InputHelper::tryGetContState(nn::hid::NpadBaseState* state, ulong port) {
    nn::hid::NpadStyleSet styleSet = nn::hid::GetNpadStyleSet(port);
    isReadInput = true;
    bool result = true;

    if (styleSet.Test((int)nn::hid::NpadStyleTag::NpadStyleFullKey)) {
        nn::hid::GetNpadState((nn::hid::NpadFullKeyState*)state, port);
    } else if (styleSet.Test((int)nn::hid::NpadStyleTag::NpadStyleHandheld)) {
        nn::hid::GetNpadState((nn::hid::NpadHandheldState*)state, port);
    } else if (styleSet.Test((int)nn::hid::NpadStyleTag::NpadStyleJoyDual)) {
        nn::hid::GetNpadState((nn::hid::NpadJoyDualState*)state, port);
    } else if (styleSet.Test((int)nn::hid::NpadStyleTag::NpadStyleJoyLeft)) {
        nn::hid::GetNpadState((nn::hid::NpadJoyLeftState*)state, port);
    } else if (styleSet.Test((int)nn::hid::NpadStyleTag::NpadStyleJoyRight)) {
        nn::hid::GetNpadState((nn::hid::NpadJoyRightState*)state, port);
    } else {
        result = false;
    }

    isReadInput = false;

    return result;
}

bool InputHelper::isButtonHold(nn::hid::NpadButton button) {
    return curControllerState.mButtons.Test((int)button);
}

bool InputHelper::isButtonPress(nn::hid::NpadButton button) {
    return curControllerState.mButtons.Test((int)button) && !prevControllerState.mButtons.Test((int)button);
}

bool InputHelper::isButtonRelease(nn::hid::NpadButton button) {
    return !curControllerState.mButtons.Test((int)button) && prevControllerState.mButtons.Test((int)button);
}