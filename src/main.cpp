#include "hk/gfx/ImGuiBackendNvn.h"
#include "hk/hook/InstrUtil.h"
#include "hk/hook/Trampoline.h"

#include "agl/common/aglDrawContext.h"

#include "game/Sequence/HakoniwaSequence.h"
#include "game/System/Application.h"
#include "game/System/GameSystem.h"

#include "InputHelper.h"
#include "al/Library/LiveActor/LiveActorKit.h"
#include "al/Library/Scene/Scene.h"
#include "al/Library/System/GameSystemInfo.h"
#include "keeper.h"
#include "seed.h"

#include "imgui.h"
#include <cstdio>
#include <cstdlib>
#include <nn/swkbd/swkbd.h>
#include <sead/heap/seadExpHeap.h>

static sead::Heap* sImGuiHeap = nullptr;
static bool showMenu = true;
static char* mResBuf = (char*)malloc(0x400);
static nn::swkbd::String mResultString(0x400, mResBuf);
static bool mIsCancelled = false;

HkTrampoline<void, GameSystem*> gameSystemInit = hk::hook::trampoline([](GameSystem* gameSystem) -> void {
    gameSystemInit.orig(gameSystem);

    auto* imgui = hk::gfx::ImGuiBackendNvn::instance();

    imgui->setAllocator(
        { [](size allocSize, size alignment) -> void* {
             return malloc(allocSize);
         },
            [](void* ptr) -> void {
                free(ptr);
            } });
    imgui->tryInitialize();

    InputHelper::setPort(0);

    nn::hid::InitializeMouse();
});

static void updateImGuiInput() {
    static nn::hid::MouseState state;
    static nn::hid::MouseState lastState;

    lastState = state;
    nn::hid::GetMouseState(&state);

    auto& io = ImGui::GetIO();
    io.AddMousePosEvent(state.mX / 1280.f * io.DisplaySize.x, state.mY / 720.f * io.DisplaySize.y);
    constexpr std::pair<nn::hid::MouseButton, ImGuiMouseButton> buttonMap[] = {
        { nn::hid::MouseButton::Left, ImGuiMouseButton_Left },
        { nn::hid::MouseButton::Right, ImGuiMouseButton_Right },
        { nn::hid::MouseButton::Middle, ImGuiMouseButton_Middle },
    };

    for (const auto& [hidButton, imguiButton] : buttonMap) {
        if (state.mButtons.Test(int(hidButton)) && !lastState.mButtons.Test(int(hidButton))) {
            io.AddMouseButtonEvent(imguiButton, true);
        } else if (!state.mButtons.Test(int(hidButton)) && lastState.mButtons.Test(int(hidButton))) {
            io.AddMouseButtonEvent(imguiButton, false);
        }
    }

    io.AddMouseWheelEvent(state.mWheelDeltaX, state.mWheelDeltaY);

    /* Keyboard missing */

    io.MouseDrawCursor = true;
}

HkTrampoline<void, GameSystem*> drawMainHook = hk::hook::trampoline([](GameSystem* gameSystem) -> void {
    drawMainHook.orig(gameSystem);

    auto* drawContext = Application::instance()->mDrawSystemInfo->drawContext;
    /* ImGui */
    InputHelper::updatePadState();
    // updateImGuiInput();

    ImGui::NewFrame();

    if (showMenu) {
        ImGui::Begin("Seeded Talkatoo", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
        ImGui::SetWindowSize({ 250, 140 });
        ImGui::Text("ZR + R + L -> Toggle Menu");
        ImGui::Text("LEFT/RIGHT -> Change Seed");
        ImGui::Text("ZL + LEFT/RIGHT -> Change by 10");
        ImGui::Text("ZL + UP -> Open Keyboard");
        ImGui::Separator();

        ImGui::Text("Seed: %d", getKeeper().origSeed);
        ImGui::End();
    }

    ImGui::Render();

    hk::gfx::ImGuiBackendNvn::instance()->draw(ImGui::GetDrawData(), drawContext->getCommandBuffer()->ToData()->pNvnCommandBuffer);
});

void openKeyboard(nn::swkbd::String* resultString) {
    nn::swkbd::ShowKeyboardArg keyboardArg = nn::swkbd::ShowKeyboardArg();
    nn::swkbd::MakePreset(&keyboardArg.keyboardConfig, nn::swkbd::Preset::Default);

    keyboardArg.keyboardConfig.keyboardMode = nn::swkbd::KeyboardMode::ModeNumeric;
    keyboardArg.keyboardConfig.textMaxLength = 9;
    keyboardArg.keyboardConfig.textMinLength = 1;
    keyboardArg.keyboardConfig.isUseUtf8 = true;
    keyboardArg.keyboardConfig.inputFormMode = nn::swkbd::InputFormMode::OneLine;
    keyboardArg.keyboardConfig.rightOptionalSymbolKey = u'-';

    nn::swkbd::SetHeaderText(&keyboardArg.keyboardConfig, u"Header");
    nn::swkbd::SetSubText(&keyboardArg.keyboardConfig, u"SubText");
    int mWorkBufSize = nn::swkbd::GetRequiredWorkBufferSize(false);
    char* mWorkBuf = (char*)malloc(mWorkBufSize);

    int mTextCheckSize = 0x400;
    char* mTextCheckBuf = (char*)malloc(mTextCheckSize);

    int mCustomizeDicSize = 0x400;
    char* mCustomizeDicBuf = (char*)malloc(mCustomizeDicSize);

    keyboardArg.workBufSize = mWorkBufSize;
    keyboardArg.textCheckWorkBufSize = mTextCheckSize;
    keyboardArg._customizeDicBufSize = mCustomizeDicSize;

    keyboardArg.workBuf = mWorkBuf;
    keyboardArg.textCheckWorkBuf = mTextCheckBuf;
    keyboardArg._customizeDicBuf = mCustomizeDicBuf;

    char mInitialText[16];
    snprintf(mInitialText, sizeof(mInitialText), "%d", getKeeper().origSeed);

    if (strlen(mInitialText) > 0) {
        nn::swkbd::SetInitialTextUtf8(&keyboardArg, mInitialText);
    }

    mIsCancelled = nn::swkbd::ShowKeyboard(resultString, keyboardArg) == 671; // no idea what 671 could be
}

HkTrampoline<void, HakoniwaSequence*> hakoniwaSequenceUpdate = hk::hook::trampoline([](HakoniwaSequence* hakoniwaSequence) -> void {
    hakoniwaSequenceUpdate.orig(hakoniwaSequence);

    if (InputHelper::isHoldZR() && InputHelper::isHoldR() && InputHelper::isPressL()) {
        showMenu = !showMenu;
    }

    if (showMenu) {
        if (InputHelper::isPressPadRight()) {
            getKeeper().origSeed += InputHelper::isHoldZL() ? 10 : 1;
            getKeeper().seed_arr = Seeded::generate_2d_array(getKeeper().origSeed);
        } else if (InputHelper::isPressPadLeft()) {
            getKeeper().origSeed -= InputHelper::isHoldZL() ? 10 : 1;
            getKeeper().seed_arr = Seeded::generate_2d_array(getKeeper().origSeed);
        } else if (InputHelper::isHoldZL() && InputHelper::isPressPadUp()) {
            openKeyboard(&mResultString);
            if (!mIsCancelled) {
                getKeeper().origSeed = std::atoi(mResultString.strBuf);
                getKeeper().seed_arr = Seeded::generate_2d_array(getKeeper().origSeed);
            }
        }
    }
});

extern "C" void hkMain() {
    drawMainHook.installAtSym<"_ZN10GameSystem8drawMainEv">();
    gameSystemInit.installAtSym<"_ZN10GameSystem4initEv">();
    hakoniwaSequenceUpdate.installAtSym<"_ZN16HakoniwaSequence6updateEv">();
    hk::hook::writeBranchLinkAtSym<"GetRandomHookSym">(Seeded::getRandomHook);
    hk::hook::writeBranchLinkAtSym<"TableHookSym">(Seeded::tableHook);

    hk::gfx::ImGuiBackendNvn::instance()->installHooks(false);
}
