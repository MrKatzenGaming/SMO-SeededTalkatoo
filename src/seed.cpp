#include "seed.h"

#include "keeper.h"
#include <algorithm>
#include <cstring>
#include <game/System/GameDataFunction.h>
#include <game/System/GameDataUtil.h>
#include <random>

Keeper& getKeeper() {
    static Keeper i; // Returns a constant Keeper, can be accessed from anywhere
    return i;
}

namespace Seeded {

    int find(const char* seek) {
        static constexpr const char* stageNames[17] = { "CapWorldHomeStage", "WaterfallWorldHomeStage", "SandWorldHomeStage", "LakeWorldHomeStage", "ForestWorldHomeStage", "CloudWorldHomeStage", "ClashWorldHomeStage", "CityWorldHomeStage", "SnowWorldTownStage", "SeaWorldHomeStage", "LavaWorldHomeStage", "BossRaidWorldHomeStage", "SkyWorldHomeStage", "MoonWorldHomeStage", "PeachWorldHomeStage", "Special1WorldHomeStage", "Special2WorldHomeStage" };
        for (int i = 0; i < 17; ++i) {
            if (strcmp(stageNames[i], seek) == 0)
                return i;
        }
        return -1;
    }

    std::array<std::array<int, 120>, 17> generate_2d_array(unsigned seed) {
        std::array<std::array<int, 120>, 17> array;

        // Initialize the vector with values from 1 to num_cols
        std::array<int, 120> numbers;
        for (int i = 0; i < 120; ++i) {
            numbers[i] = i + 1;
        }

        // Set the seed for reproducibility
        std::mt19937 rng(seed);

        for (int i = 0; i < 17; ++i) {
            // Shuffle the numbers)
            std::shuffle(numbers.begin(), numbers.end(), rng);

            // Assign the shuffled numbers to the 2D array
            for (int j = 0; j < 120; ++j) {
                array[i][j] = numbers[j];
            }
        }

        return array;
    }

    int getRandomHook(int zero, int max) {
        return getKeeper().next_num;
    }

    void tableHook(int* arr, int* num_avalible, al::LiveActor* poetter) {
        if (getKeeper().seed_arr[0][0] == 0) {
            // getKeeper().seed_arr = readNumbersFromFile("sd:/atmosphere/contents/0100000000010000/seed.txt");
            getKeeper().seed_arr = generate_2d_array(getKeeper().origSeed);
        }
        rs::calcShineIndexTableNameUnlockable(arr, num_avalible, poetter);

        getKeeper().tableArr = arr;
        getKeeper().tableNum = *num_avalible;

        int stage_id = find(GameDataFunction::getCurrentStageName(GameDataHolderAccessor(poetter)));
        getKeeper().stageNum = stage_id;

        for (int i = 0; i < 120; ++i) {
            for (int j = 0; j < *num_avalible; ++j) {
                if (arr[j] == getKeeper().seed_arr[stage_id][i]) {
                    getKeeper().next_num = j;
                    return;
                }
            }
        }

        return;
    }

}