// data_book.cpp
#include "data_book.hpp"
#include <algorithm>

DataBook::DataBook() {
    demand_values_.fill(0);
    demand_volumes_.fill(0);
    supply_values_.fill(0);
    supply_volumes_.fill(0);
}

void DataBook::applyUpdate(const DataLevel& update) {
    if (update.level >= MAX_BOOK_LEVELS) return;
    std::lock_guard<std::mutex> lock(mtx_);

    if (update.side == 0) {
        demand_values_[update.level] = update.value;
        demand_volumes_[update.level] = update.volume;
    } else {
        supply_values_[update.level] = update.value;
        supply_volumes_[update.level] = update.volume;
    }
}

double DataBook::demandValue(int level) const {
    if (level >= MAX_BOOK_LEVELS) return 0.0;
    std::lock_guard<std::mutex> lock(mtx_);
    return static_cast<double>(demand_values_[level]) / 1e9;
}

int DataBook::demandVolume(int level) const {
    if (level >= MAX_BOOK_LEVELS) return 0;
    std::lock_guard<std::mutex> lock(mtx_);
    return demand_volumes_[level];
}

double DataBook::supplyValue(int level) const {
    if (level >= MAX_BOOK_LEVELS) return 0.0;
    std::lock_guard<std::mutex> lock(mtx_);
    return static_cast<double>(supply_values_[level]) / 1e9;
}

int DataBook::supplyVolume(int level) const {
    if (level >= MAX_BOOK_LEVELS) return 0;
    std::lock_guard<std::mutex> lock(mtx_);
    return supply_volumes_[level];
}

DataBook& DataBookManager::getOrCreateBook(uint32_t subject_id) {
    std::lock_guard<std::mutex> lock(books_mtx_);
    return books_[subject_id];
}
