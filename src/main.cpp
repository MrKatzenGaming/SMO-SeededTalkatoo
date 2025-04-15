#include "hk/gfx/ImGuiBackendNvn.h"
#include "hk/hook/InstrUtil.h"
#include "hk/hook/Trampoline.h"

#include "agl/common/aglDrawContext.h"

#include "game/Sequence/HakoniwaSequence.h"
#include "game/System/Application.h"
#include "game/System/GameSystem.h"

#include "InputHelper.h"
#include "al/Library/Controller/InputFunction.h"
#include "al/Library/LiveActor/ActorPoseKeeper.h"
#include "al/Library/LiveActor/ActorPoseUtil.h"
#include "al/Library/LiveActor/LiveActor.h"
#include "al/Library/LiveActor/LiveActorKit.h"
#include "al/Library/Memory/HeapUtil.h"
#include "al/Library/Player/PlayerHolder.h"
#include "al/Library/Scene/Scene.h"
#include "al/Library/System/GameSystemInfo.h"
#include "keeper.h"
#include "seed.h"

#include <sead/heap/seadExpHeap.h>
#include <utility>

#include "nn/hid.h"

#include "imgui.h"

static sead::Heap* sImGuiHeap = nullptr;

HkTrampoline<void, GameSystem*> gameSystemInit = hk::hook::trampoline([](GameSystem* gameSystem) -> void {
    sImGuiHeap = sead::ExpHeap::create(2_MB, "ImGuiHeap", al::getStationedHeap(), 8, sead::Heap::cHeapDirection_Forward, false);

    gameSystemInit.orig(gameSystem);

    auto* imgui = hk::gfx::ImGuiBackendNvn::instance();

    imgui->setAllocator(
        { [](size allocSize, size alignment) -> void* {
             return sImGuiHeap->tryAlloc(allocSize, alignment);
         },
            [](void* ptr) -> void {
                sImGuiHeap->free(ptr);
            } });
    imgui->tryInitialize();

    nn::hid::InitializeMouse();
    InputHelper::setPort(0);
});

HkTrampoline<void, al::LiveActor*> marioControl = hk::hook::trampoline([](al::LiveActor* player) -> void {
    // if (al::isPadHoldA(-1)) {
    //     player->getPoseKeeper()->getVelocityPtr()->y = 0;
    //     al::getTransPtr(player)->y += 20;
    // }

    marioControl.orig(player);
});

static void updateImGuiInput() {
    static nn::hid::MouseState state;
    static nn::hid::MouseState lastState;

    lastState = state;
    nn::hid::GetMouseState(&state);

    auto& io = ImGui::GetIO();
    io.AddMousePosEvent(state.x / 1280.f * io.DisplaySize.x, state.y / 720.f * io.DisplaySize.y);
    constexpr std::pair<nn::hid::MouseButton, ImGuiMouseButton> buttonMap[] = {
        { nn::hid::MouseButton::Left, ImGuiMouseButton_Left },
        { nn::hid::MouseButton::Right, ImGuiMouseButton_Right },
        { nn::hid::MouseButton::Middle, ImGuiMouseButton_Middle },
    };

    for (const auto& [hidButton, imguiButton] : buttonMap) {
        if (state.buttons.Test(int(hidButton)) && !lastState.buttons.Test(int(hidButton))) {
            io.AddMouseButtonEvent(imguiButton, true);
        } else if (!state.buttons.Test(int(hidButton)) && lastState.buttons.Test(int(hidButton))) {
            io.AddMouseButtonEvent(imguiButton, false);
        }
    }

    io.AddMouseWheelEvent(state.wheelDeltaX, state.wheelDeltaY);

    /* Keyboard missing */

    io.MouseDrawCursor = true;
}
static bool showMenu;
HkTrampoline<void, GameSystem*> drawMainHook = hk::hook::trampoline([](GameSystem* gameSystem) -> void {
    drawMainHook.orig(gameSystem);

    auto* drawContext = Application::instance()->mDrawSystemInfo->drawContext;

    HakoniwaSequence* sequence = static_cast<HakoniwaSequence*>(gameSystem->mSequence);
    if (sequence == nullptr)
        return;

    al::Scene* scene = sequence->mCurrentScene;
    al::LiveActor* player = nullptr;
    if (scene && scene->mLiveActorKit && scene->mLiveActorKit->mPlayerHolder)
        player = scene->mLiveActorKit->mPlayerHolder->tryGetPlayer(0);

    /* ImGui */

    updateImGuiInput();

    ImGui::NewFrame();
    ImGui::Begin("Seeded Talkatoo", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
    ImGui::SetWindowSize({ 200, 100 });
    if (ImGui::InputInt("Seed", &getKeeper().origSeed)) {
        getKeeper().seed_arr = Seeded::generate_2d_array(getKeeper().origSeed);
    }
    ImGui::Text("Show Menu: %d", showMenu);
    ImGui::Text("Seeded: %d", getKeeper().origSeed);
    ImGui::End();

    ImGui::Render();

    hk::gfx::ImGuiBackendNvn::instance()->draw(ImGui::GetDrawData(), drawContext->getCommandBuffer()->ToData()->pNvnCommandBuffer);

    InputHelper::updatePadState();
});

HkTrampoline<void, HakoniwaSequence*> hakoniwaSequenceUpdate = hk::hook::trampoline([](HakoniwaSequence* hakoniwaSequence) -> void {
    hakoniwaSequenceUpdate.orig(hakoniwaSequence);

    StageScene* stageScene = (StageScene*)hakoniwaSequence->mCurrentScene;

    if (InputHelper::isHoldZL() && InputHelper::isPressPadUp()) {
        showMenu = !showMenu;
    }

    if (showMenu) {
        if (InputHelper::isPressPadRight()) {
            getKeeper().origSeed++;
            getKeeper().seed_arr = Seeded::generate_2d_array(getKeeper().origSeed);
        } else if (InputHelper::isPressPadLeft()) {
            getKeeper().origSeed--;
            getKeeper().seed_arr = Seeded::generate_2d_array(getKeeper().origSeed);
        }
    }
});

extern "C" void hkMain() {
    marioControl.installAtSym<"_ZN19PlayerActorHakoniwa7controlEv">();
    drawMainHook.installAtSym<"_ZN10GameSystem8drawMainEv">();
    gameSystemInit.installAtSym<"_ZN10GameSystem4initEv">();
    hakoniwaSequenceUpdate.installAtSym<"_ZN16HakoniwaSequence6updateEv">();
    hk::hook::writeBranchLinkAtSym<"GetRandomHookSym">(Seeded::getRandomHook);
    hk::hook::writeBranchLinkAtSym<"TableHookSym">(Seeded::tableHook);

    hk::gfx::ImGuiBackendNvn::instance()->installHooks(false);
}
