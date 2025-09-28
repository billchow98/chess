#include "tt.h"

#include <bit>
#include <vector>

#include "common.h"

namespace tuna::tt {

u32 to_entry_hash(Hash hash) {
    return static_cast<u32>(hash);
}

void Entry::invalidate() {
    hash = 0;
}

bool Entry::is_valid() {
    return hash != 0;
}

Score Entry::to_tt_score(Score score, Ply ply) {
    if (score::is_mate(score)) {
        auto mate_ply = score::mate_distance(score);
        auto depth = mate_ply - ply;
        auto sign = score >= 0 ? 1 : -1;
        return sign * score::mate(depth);
    }
    return score;
}

Score Entry::search_score(Ply ply) {
    if (score::is_mate(score)) {
        auto depth = score::mate_distance(score);
        auto mate_ply = depth + ply;
        auto sign = score >= 0 ? 1 : -1;
        return sign * score::mate(mate_ply);
    }
    return score;
}

// depth: depth searched
// ply: plies from root
void Entry::update(Hash hash, Move move, Score score, Ply depth, Bound bound,
                   Ply ply) {
    auto eh = to_entry_hash(hash);
    if (eh != this->hash || depth >= this->depth) {
        this->hash = eh;
        this->move = move;
        this->score = to_tt_score(score, ply);
        this->depth = depth;
        this->bound = bound;
    }
}

Tt::Tt() {
    init();
}

void Tt::init_buckets() {
    buckets = std::vector<Bucket>(size);
}

void Tt::set_power_two_size(u64 mb) {
    size = mb * 1024 * 1024 / sizeof(Bucket);
    size = 1_u64 << (63 - std::countl_zero(size));
}

void Tt::init(u64 mb) {
    set_power_two_size(mb);
    init_buckets();
}

i32 Tt::hash_index(Hash hash) {
    // Must use & because compiler doesn't know size is always
    // a power of 2
    return hash >> 32 & size - 1;
}

Bucket &Tt::get_bucket(Hash hash) {
    auto index = hash_index(hash);
    return buckets[index];
}

Entry &Tt::find(Hash hash) {
    auto &b = get_bucket(hash);
    Entry *entry = nullptr;
    for (auto &e : b.entries) {
        if (e.hash == to_entry_hash(hash)) {
            return e;
        }
        if (!entry || e.depth < entry->depth) {
            entry = &e;
        }
    }
    entry->invalidate();
    return *entry;
}

void Tt::clear() {
    init_buckets();
}

// Will clear all entries!
void Tt::resize(u64 mb) {
    init(mb);
}

i32 Tt::hashfull() {
    i32 cnt = 0;
    for (auto i = 0; i < 1000; i++) {
        for (auto e : buckets[i].entries) {
            cnt += e.is_valid();
        }
    }
    return cnt / BUCKET_SIZE;
}

}  // namespace tuna::tt
