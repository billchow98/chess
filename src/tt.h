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

class __attribute__((packed)) Entry {
public:
    void invalidate();

    bool is_valid();

    Score search_score(Ply ply);

    void update(Hash hash, Move move, Score score, Ply depth, Bound bound,
                Ply ply);

    u32 hash();

    Move move();

    Ply depth();

    Bound bound();

private:
    Score to_tt_score(Score score, Ply ply);

    u32 hash_;
    Move move_;
    Score score_;
    Ply depth_;  // Will only store non-negative depth
    Bound bound_;
};

static_assert(sizeof(Entry) == 10);

const auto BUCKET_SIZE = 3;

struct alignas(32) Bucket {
    std::array<Entry, BUCKET_SIZE> entries;
};

static_assert(sizeof(Bucket) == 32);

class Tt {
public:
    Tt();

    void clear();

    void resize(u64 mb);

    Entry &find(Hash hash);

    i32 hashfull();

private:
    void init_buckets();

    void set_power_two_size(u64 mb);

    void init(u64 mb = 16);

    i32 hash_index(Hash hash);

    Bucket &get_bucket(Hash hash);

    std::vector<Bucket> buckets_;
    u64 size_;
};

}  // namespace tuna::tt

#endif
