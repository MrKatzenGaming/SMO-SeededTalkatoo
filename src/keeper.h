#pragma once
#include <container/seadPtrArray.h>
#include <game/Player/PlayerActorHakoniwa.h>
#include <game/Scene/StageScene.h>
#include <game/Sequence/HakoniwaSequence.h>

class Keeper {
public:
    HakoniwaSequence* hakoniwaSequenceRef;
    StageScene* stageSceneRef; // Current stage scene
    PlayerActorHakoniwa* p1Ref; // Player

    int status = 0; // Status of the Field

    const char* curHackName;
    const char* curHackAction;

    static bool tryRegisterShine(Shine* shine);
    sead::PtrArray<Shine> mShineArray; // List of all Shines currently in a Stage
    sead::PtrArray<Shine> mPoetterArray; // List of all Shines given by Poetter

    wchar_t* msg;
    int param_3;
    int param_4;

    // int seed_arr[17][120];
    std::array<std::array<int, 120>, 17> seed_arr;

    int next_num;
    int* tableArr;
    int tableNum;

    int origSeed = 44;
    int seed[201] = { 0 };
    int arr[1024];

    // debug
    int stageNum = 0;
};

Keeper& getKeeper();