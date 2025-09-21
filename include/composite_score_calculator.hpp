// composite_score_calculator.hpp
#pragma once

#include "data_book.hpp"
#include <cstdint>

class CompositeScoreCalculator {
public:
    CompositeScoreCalculator() = default;

    // Returns composite score * 1e9 (already scaled), using integer arithmetic
    int64_t calculateCompositeScore(const DataBook& book);
};
