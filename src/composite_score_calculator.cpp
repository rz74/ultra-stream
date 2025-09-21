// composite_score_calculator.cpp
#include "composite_score_calculator.hpp"
#include <limits>

int64_t CompositeScoreCalculator::calculateCompositeScore(const DataBook& book) {
    int64_t demand_val = static_cast<int64_t>(book.demandValue(0) * 1e9);
    int64_t supply_val = static_cast<int64_t>(book.supplyValue(0) * 1e9);
    int demand_vol = book.demandVolume(0);
    int supply_vol = book.supplyVolume(0);

    int total_vol = demand_vol + supply_vol;
    if (total_vol == 0) {
        if (demand_val > 0 && supply_val > 0)
            return (demand_val + supply_val) / 2;
        else
            return 0;
    }

    int64_t weighted = (demand_val * supply_vol + supply_val * demand_vol) / total_vol;
    return weighted;
}
