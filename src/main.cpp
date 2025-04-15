#include "hk/gfx/ImGuiBackendNvn.h"
#include "hk/hook/InstrUtil.h"
#include "hk/hook/Trampoline.h"

#include "agl/common/aglDrawContext.h"

#include "game/Sequence/HakoniwaSequence.h"
#include "game/System/Application.h"
#include "game/System/GameSystem.h"

#include "InputHelper.h"
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

    InputHelper::setPort(0);
});

static bool showMenu = true;
HkTrampoline<void, GameSystem*> drawMainHook = hk::hook::trampoline([](GameSystem* gameSystem) -> void {
    drawMainHook.orig(gameSystem);

    auto* drawContext = Application::instance()->mDrawSystemInfo->drawContext;
    /* ImGui */
    InputHelper::updatePadState();

    ImGui::NewFrame();

    if (showMenu) {
        ImGui::Begin("Seeded Talkatoo", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
        ImGui::SetWindowSize({ 280, 120 });
        ImGui::Text("ZR + R + L -> Toggle Menu");
        ImGui::Text("LEFT/RIGHT -> Change Seed");
        ImGui::Text("ZL + LEFT/RIGHT -> Change by 10");
        ImGui::Separator();

        ImGui::Text("Seed: %d", getKeeper().origSeed);
        ImGui::End();
    }

    ImGui::Render();

    hk::gfx::ImGuiBackendNvn::instance()->draw(ImGui::GetDrawData(), drawContext->getCommandBuffer()->ToData()->pNvnCommandBuffer);
});

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
