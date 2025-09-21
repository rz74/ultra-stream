// data_book.hpp
#pragma once

#include "types.hpp"
#include <array>
#include <unordered_map>
#include <mutex>

class DataBook {
public:
    DataBook();

    void applyUpdate(const DataLevel& update);
    double demandValue(int level) const;
    int demandVolume(int level) const;
    double supplyValue(int level) const;
    int supplyVolume(int level) const;
    int numLevels() const { return MAX_BOOK_LEVELS; }

private:
    std::array<int64_t, MAX_BOOK_LEVELS> demand_values_;
    std::array<int, MAX_BOOK_LEVELS> demand_volumes_;
    std::array<int64_t, MAX_BOOK_LEVELS> supply_values_;
    std::array<int, MAX_BOOK_LEVELS> supply_volumes_;

    mutable std::mutex mtx_;
};

// DataBookManager maintains one book per subject
class DataBookManager {
public:
    DataBook& getOrCreateBook(uint32_t subject_id);

private:
    std::unordered_map<uint32_t, DataBook> books_;
    std::mutex books_mtx_;
};
