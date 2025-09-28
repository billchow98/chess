#ifndef TUNA_TT_H
#define TUNA_TT_H

#include <array>
#include <vector>

#include "common.h"

namespace tuna::tt {

enum {
    Lower = 0b01,
    Upper = 0b10,
    Exact = Lower | Upper,
};

struct __attribute__((packed)) Entry {
    u32 hash;
    Move move;
    Score score;
    Ply depth;  // Will only store non-negative depth
    Bound bound;

    void invalidate();

    bool is_valid();

    Score to_tt_score(Score score, Ply ply);

    Score search_score(Ply ply);

    void update(Hash hash, Move move, Score score, Ply depth, Bound bound,
                Ply ply);
};

static_assert(sizeof(Entry) == 10);

const auto BUCKET_SIZE = 3;

struct alignas(32) Bucket {
    std::array<Entry, BUCKET_SIZE> entries;
};

static_assert(sizeof(Bucket) == 32);

struct Tt {
    std::vector<Bucket> buckets;
    u64 size;

    Tt();

    void init_buckets();

    void set_power_two_size(u64 mb);

    void init(u64 mb = 16);

    i32 hash_index(Hash hash);

    Bucket &get_bucket(Hash hash);

    Entry &find(Hash hash);

    void clear();

    void resize(u64 mb);

    i32 hashfull();
};

}  // namespace tuna::tt

#endif
