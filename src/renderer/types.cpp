#include "types.h"
#include <cstddef>
#include <cstdint>

struct Range {
    uint32_t start;
    uint32_t end;
};

bool isExtend(Range range, uint32_t num) {
    if (range.end < num && range.end + 1 == num) {
        return true;
    }
    return false;
}

int FreeList::pop_free_slot() {
    if (ci_ranges.size() > 0) {
        auto &lastElement = ci_ranges[ci_ranges.size() - 1];
        int   freeSlot    = lastElement.end;
        lastElement.end--;

        if (lastElement.start > lastElement.end) {
            ci_ranges.pop_back();
        }

        return freeSlot;
    } else {
        return -1;
    }
}

void FreeList::push_free_slot(uint32_t num) {
    int highest = 0;
    if (ci_ranges.empty()) {
        ci_ranges.push_back(Range{num, num});
        return;
    }

    for (int i = 0; i < ci_ranges.size(); i++) {

        if (isExtend(ci_ranges[i], num)) {
            ci_ranges[i].end++;
            size_t nextElementIndex = i + 1;

            if (ci_ranges.size() != nextElementIndex) {
                if (ci_ranges[nextElementIndex].start == ci_ranges[i].end) {
                    ci_ranges[i].end = ci_ranges[nextElementIndex].end;
                    ci_ranges.erase(std::next(ci_ranges.begin(), nextElementIndex));
                }
            }

            return;
        }
    }
}
