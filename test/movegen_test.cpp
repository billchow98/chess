#include "movegen.h"

#include <gtest/gtest.h>

#include "hash.h"
#include "lookup.h"
#include "search.h"

using namespace tuna;
using namespace tuna::movegen;

void init() {
    hash::init();
    lookup::init();
    search::init();
}

class MovegenTest : public testing::Test {
protected:
    MovegenTest() {
        init();
        board = Board();  // After hashes are initialized properly
    }

    Board board;
};

TEST_F(MovegenTest, BasicPerft1) {
    // clang-format off
    board.setup_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    EXPECT_EQ(perft(board, 6), 119'060'324);
    // clang-format on
}

TEST_F(MovegenTest, BasicPerft2) {
    // clang-format off
    board.setup_fen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
    EXPECT_EQ(perft(board, 5), 193'690'690);
    // clang-format on
}

TEST_F(MovegenTest, BasicPerft3) {
    // clang-format off
    board.setup_fen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1");
    EXPECT_EQ(perft(board, 7), 178'633'661);
    // clang-format on
}

TEST_F(MovegenTest, BasicPerft4) {
    // clang-format off
    board.setup_fen("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
    EXPECT_EQ(perft(board, 6), 706'045'033);
    // clang-format on
}

TEST_F(MovegenTest, BasicPerft5) {
    // clang-format off
    board.setup_fen("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");
    EXPECT_EQ(perft(board, 5), 89'941'194);
    // clang-format on
}

TEST_F(MovegenTest, BasicPerft6) {
    // clang-format off
    board.setup_fen("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10");
    EXPECT_EQ(perft(board, 5), 164'075'551);
    // clang-format on
}

// Skipping MovegenTest.PseudoLegalPerft* because it takes too long for CI/CD.

TEST_F(MovegenTest, PseudoLegalPerft1) {
    // clang-format off
    GTEST_SKIP();
    board.setup_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    EXPECT_EQ(perft_pseudo_legal(board, 6), 119'060'324);
    // clang-format on
}

TEST_F(MovegenTest, PseudoLegalPerft2) {
    // clang-format off
    GTEST_SKIP();
    board.setup_fen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
    EXPECT_EQ(perft_pseudo_legal(board, 5), 193'690'690);
    // clang-format on
}

TEST_F(MovegenTest, PseudoLegalPerft3) {
    // clang-format off
    GTEST_SKIP();
    board.setup_fen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1");
    EXPECT_EQ(perft_pseudo_legal(board, 7), 178'633'661);
    // clang-format on
}

TEST_F(MovegenTest, PseudoLegalPerft4) {
    // clang-format off
    GTEST_SKIP();
    board.setup_fen("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
    EXPECT_EQ(perft_pseudo_legal(board, 6), 706'045'033);
    // clang-format on
}

TEST_F(MovegenTest, PseudoLegalPerft5) {
    // clang-format off
    GTEST_SKIP();
    board.setup_fen("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");
    EXPECT_EQ(perft_pseudo_legal(board, 5), 89'941'194);
    // clang-format on
}

TEST_F(MovegenTest, PseudoLegalPerft6) {
    // clang-format off
    GTEST_SKIP();
    board.setup_fen("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10");
    EXPECT_EQ(perft_pseudo_legal(board, 5), 164'075'551);
    // clang-format on
}
