#pragma once

#include <cstdint>
#include <vector>

struct Range;


struct FreeList {
    std::vector<Range> ci_ranges;

    int pop_free_slot();
    Range pop_free_slots(uint32_t num);

    void push_free_slot(uint32_t num);
};