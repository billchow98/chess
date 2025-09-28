#!/usr/bin/env bash

set -euxo pipefail

# -log file=sprt_runner.log level=info realtime=true engine=true
./runner \
    -engine name=new cmd="./all_tests2" \
    -engine name=old cmd="./all_tests" \
    -each tc=10+0.1 proto=uci option.Hash=16 \
    -recover -repeat -rounds 10000 \
    -openings file=8moves_v3.pgn format=pgn \
    -pgnout sprt.pgn \
    -ratinginterval 1 \
    -sprt elo0=0 elo1=10 alpha=0.05 beta=0.05 model=logistic \
    -concurrency "$1" -config \
    | tee sprt.log
