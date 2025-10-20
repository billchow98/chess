#include "tt.h"

#include <bit>
#include <vector>

#include "common.h"

namespace tuna::tt {

u32 to_entry_hash(Hash hash) {
    return static_cast<u32>(hash);
}

void Entry::invalidate() {
    hash_ = 0;
}

bool Entry::is_valid() const {
    return hash_ != 0;
}

Score Entry::search_score(Ply ply) const {
    if (score::is_mate(score_)) {
        auto depth = score::mate_distance(score_);
        auto mate_ply = depth + ply;
        auto sign = score_ >= 0 ? 1 : -1;
        return sign * score::mate(mate_ply);
    }
    return score_;
}

// depth: depth searched
// ply: plies from root
void Entry::update(Hash hash, Move move, Score score, Ply depth, Bound bound,
                   Ply ply) {
    auto eh = to_entry_hash(hash);
    if (eh != this->hash_ || depth >= this->depth_) {
        this->hash_ = eh;
        this->move_ = move;
        this->score_ = to_tt_score(score, ply);
        this->depth_ = depth;
        this->bound_ = bound;
    }
}

u32 Entry::hash() const {
    return hash_;
}

Move Entry::move() const {
    return move_;
}

Ply Entry::depth() const {
    return depth_;
}

Bound Entry::bound() const {
    return bound_;
}

Score Entry::to_tt_score(Score score, Ply ply) const {
    if (score::is_mate(score)) {
        auto mate_ply = score::mate_distance(score);
        auto depth = mate_ply - ply;
        auto sign = score >= 0 ? 1 : -1;
        return sign * score::mate(depth);
    }
    return score;
}

Tt::Tt() {
    init();
}

void Tt::clear() {
    init_buckets();
}

// Will clear all entries!
void Tt::resize(u64 mb) {
    init(mb);
}

Entry &Tt::find(Hash hash) {
    auto &b = get_bucket(hash);
    Entry *entry = nullptr;
    for (auto &e : b.entries) {
        if (e.hash() == to_entry_hash(hash)) {
            return e;
        }
        if (!entry || e.depth() < entry->depth()) {
            entry = &e;
        }
    }
    entry->invalidate();
    return *entry;
}

i32 Tt::hashfull() const {
    i32 cnt = 0;
    for (auto i = 0; i < 1000; i++) {
        for (auto e : buckets_[i].entries) {
            cnt += e.is_valid();
        }
    }
    return cnt / BUCKET_SIZE;
}

void Tt::init_buckets() {
    buckets_ = std::vector<Bucket>(size_);
}

void Tt::set_power_two_size(u64 mb) {
    size_ = mb * 1024 * 1024 / sizeof(Bucket);
    size_ = 1_u64 << (63 - std::countl_zero(size_));
}

void Tt::init(u64 mb) {
    set_power_two_size(mb);
    init_buckets();
}

i32 Tt::hash_index(Hash hash) const {
    // Must use & because compiler doesn't know size is always
    // a power of 2
    return hash >> 32 & size_ - 1;
}

Bucket &Tt::get_bucket(Hash hash) {
    auto index = hash_index(hash);
    return buckets_[index];
}

}  // namespace tuna::tt
