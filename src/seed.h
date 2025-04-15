#ifndef SEED_H
#define SEED_H

#include <al/Library/LiveActor/LiveActor.h>
#include <array>

namespace Seeded {
    int find(const char* seek);
    std::array<std::array<int, 120>, 17> generate_2d_array(unsigned seed);
    int getRandomHook(int zero, int max);
    void tableHook(int* arr, int* num_avalible, al::LiveActor* poetter);
};

#endif // SEED_H