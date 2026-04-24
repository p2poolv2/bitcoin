// Copyright (c) 2015-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <pow.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <util/chaintype.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(pow_tests, BasicTestingSetup)

/* Test calculation of next difficulty target with no constraints applying */
BOOST_AUTO_TEST_CASE(get_next_work)
{
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::MAIN);
    int64_t nLastRetargetTime = 1261130161; // Block #30240
    CBlockIndex pindexLast;
    pindexLast.nHeight = 32255;
    pindexLast.nTime = 1262152739;  // Block #32255
    pindexLast.nBits = 0x1d00ffff;

    // Here (and below): expected_nbits is calculated in
    // CalculateNextWorkRequired(); redoing the calculation here would be just
    // reimplementing the same code that is written in pow.cpp. Rather than
    // copy that code, we just hardcode the expected result.
    unsigned int expected_nbits = 0x1d00d86aU;
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, chainParams->GetConsensus()), expected_nbits);
    BOOST_CHECK(PermittedDifficultyTransition(chainParams->GetConsensus(), pindexLast.nHeight+1, pindexLast.nBits, expected_nbits));
}

/* Test the constraint on the upper bound for next work */
BOOST_AUTO_TEST_CASE(get_next_work_pow_limit)
{
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::MAIN);
    int64_t nLastRetargetTime = 1231006505; // Block #0
    CBlockIndex pindexLast;
    pindexLast.nHeight = 2015;
    pindexLast.nTime = 1233061996;  // Block #2015
    pindexLast.nBits = 0x1d00ffff;
    unsigned int expected_nbits = 0x1d00ffffU;
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, chainParams->GetConsensus()), expected_nbits);
    BOOST_CHECK(PermittedDifficultyTransition(chainParams->GetConsensus(), pindexLast.nHeight+1, pindexLast.nBits, expected_nbits));
}

/* Test the constraint on the lower bound for actual time taken */
BOOST_AUTO_TEST_CASE(get_next_work_lower_limit_actual)
{
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::MAIN);
    int64_t nLastRetargetTime = 1279008237; // Block #66528
    CBlockIndex pindexLast;
    pindexLast.nHeight = 68543;
    pindexLast.nTime = 1279297671;  // Block #68543
    pindexLast.nBits = 0x1c05a3f4;
    unsigned int expected_nbits = 0x1c0168fdU;
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, chainParams->GetConsensus()), expected_nbits);
    BOOST_CHECK(PermittedDifficultyTransition(chainParams->GetConsensus(), pindexLast.nHeight+1, pindexLast.nBits, expected_nbits));
    // Test that reducing nbits further would not be a PermittedDifficultyTransition.
    unsigned int invalid_nbits = expected_nbits-1;
    BOOST_CHECK(!PermittedDifficultyTransition(chainParams->GetConsensus(), pindexLast.nHeight+1, pindexLast.nBits, invalid_nbits));
}

/* Test the constraint on the upper bound for actual time taken */
BOOST_AUTO_TEST_CASE(get_next_work_upper_limit_actual)
{
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::MAIN);
    int64_t nLastRetargetTime = 1263163443; // NOTE: Not an actual block time
    CBlockIndex pindexLast;
    pindexLast.nHeight = 46367;
    pindexLast.nTime = 1269211443;  // Block #46367
    pindexLast.nBits = 0x1c387f6f;
    unsigned int expected_nbits = 0x1d00e1fdU;
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, chainParams->GetConsensus()), expected_nbits);
    BOOST_CHECK(PermittedDifficultyTransition(chainParams->GetConsensus(), pindexLast.nHeight+1, pindexLast.nBits, expected_nbits));
    // Test that increasing nbits further would not be a PermittedDifficultyTransition.
    unsigned int invalid_nbits = expected_nbits+1;
    BOOST_CHECK(!PermittedDifficultyTransition(chainParams->GetConsensus(), pindexLast.nHeight+1, pindexLast.nBits, invalid_nbits));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_negative_target)
{
    const auto consensus = CreateChainParams(*m_node.args, ChainType::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits;
    nBits = UintToArith256(consensus.powLimit).GetCompact(true);
    hash = uint256{1};
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_overflow_target)
{
    const auto consensus = CreateChainParams(*m_node.args, ChainType::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits{~0x00800000U};
    hash = uint256{1};
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_too_easy_target)
{
    const auto consensus = CreateChainParams(*m_node.args, ChainType::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits;
    arith_uint256 nBits_arith = UintToArith256(consensus.powLimit);
    nBits_arith *= 2;
    nBits = nBits_arith.GetCompact();
    hash = uint256{1};
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_biger_hash_than_target)
{
    const auto consensus = CreateChainParams(*m_node.args, ChainType::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits;
    arith_uint256 hash_arith = UintToArith256(consensus.powLimit);
    nBits = hash_arith.GetCompact();
    hash_arith *= 2; // hash > nBits
    hash = ArithToUint256(hash_arith);
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_zero_target)
{
    const auto consensus = CreateChainParams(*m_node.args, ChainType::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits;
    arith_uint256 hash_arith{0};
    nBits = hash_arith.GetCompact();
    hash = ArithToUint256(hash_arith);
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(GetBlockProofEquivalentTime_test)
{
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::MAIN);
    std::vector<CBlockIndex> blocks(10000);
    for (int i = 0; i < 10000; i++) {
        blocks[i].pprev = i ? &blocks[i - 1] : nullptr;
        blocks[i].nHeight = i;
        blocks[i].nTime = 1269211443 + i * chainParams->GetConsensus().nPowTargetSpacing;
        blocks[i].nBits = 0x207fffff; /* target 0x7fffff000... */
        blocks[i].nChainWork = i ? blocks[i - 1].nChainWork + GetBlockProof(blocks[i - 1]) : arith_uint256(0);
    }

    for (int j = 0; j < 1000; j++) {
        CBlockIndex *p1 = &blocks[m_rng.randrange(10000)];
        CBlockIndex *p2 = &blocks[m_rng.randrange(10000)];
        CBlockIndex *p3 = &blocks[m_rng.randrange(10000)];

        int64_t tdiff = GetBlockProofEquivalentTime(*p1, *p2, *p3, chainParams->GetConsensus());
        BOOST_CHECK_EQUAL(tdiff, p1->GetBlockTime() - p2->GetBlockTime());
    }
}

void sanity_check_chainparams(const ArgsManager& args, ChainType chain_type)
{
    const auto chainParams = CreateChainParams(args, chain_type);
    const auto consensus = chainParams->GetConsensus();

    // hash genesis is correct
    BOOST_CHECK_EQUAL(consensus.hashGenesisBlock, chainParams->GenesisBlock().GetHash());

    // target timespan is an even multiple of spacing
    BOOST_CHECK_EQUAL(consensus.nPowTargetTimespan % consensus.nPowTargetSpacing, 0);

    // genesis nBits is positive, doesn't overflow and is lower than powLimit
    arith_uint256 pow_compact;
    bool neg, over;
    pow_compact.SetCompact(chainParams->GenesisBlock().nBits, &neg, &over);
    BOOST_CHECK(!neg && pow_compact != 0);
    BOOST_CHECK(!over);
    BOOST_CHECK(UintToArith256(consensus.powLimit) >= pow_compact);

    // check max target * 4*nPowTargetTimespan doesn't overflow -- see pow.cpp:CalculateNextWorkRequired()
    if (!consensus.fPowNoRetargeting) {
        arith_uint256 targ_max{UintToArith256(uint256{"ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"})};
        targ_max /= consensus.nPowTargetTimespan*4;
        BOOST_CHECK(UintToArith256(consensus.powLimit) < targ_max);
    }
}

/* Test that min-difficulty blocks are allowed before nMinDiffDisableHeight */
BOOST_AUTO_TEST_CASE(mindiff_before_disable_height)
{
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::TESTNET4);
    const auto& consensus = chainParams->GetConsensus();

    // Sanity: testnet4 must have the fork height configured
    BOOST_REQUIRE(consensus.fPowAllowMinDifficultyBlocks);
    BOOST_REQUIRE(consensus.nMinDiffDisableHeight > 0);

    const unsigned int nProofOfWorkLimit = UintToArith256(consensus.powLimit).GetCompact();
    const unsigned int normalBits = 0x1c0fffff; // some non-trivial difficulty

    // Place tip one block before the disable height, so new block height
    // (tip + 1) is still below the threshold. Avoid a retarget boundary.
    CBlockIndex pindexLast;
    pindexLast.pprev = nullptr;
    pindexLast.nHeight = consensus.nMinDiffDisableHeight - 2;
    BOOST_REQUIRE((pindexLast.nHeight + 1) % consensus.DifficultyAdjustmentInterval() != 0);
    pindexLast.nTime = 1714777860;
    pindexLast.nBits = normalBits;

    // Block whose timestamp is 21 minutes after the tip (> 2 * nPowTargetSpacing)
    CBlockHeader block;
    block.nTime = pindexLast.nTime + consensus.nPowTargetSpacing * 2 + 60;

    // Before the fork height, min-difficulty should be returned
    BOOST_CHECK_EQUAL(GetNextWorkRequired(&pindexLast, &block, consensus), nProofOfWorkLimit);
}

/* Test that min-difficulty blocks are rejected at and after nMinDiffDisableHeight */
BOOST_AUTO_TEST_CASE(mindiff_at_disable_height)
{
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::TESTNET4);
    const auto& consensus = chainParams->GetConsensus();

    const unsigned int normalBits = 0x1c0fffff;

    // Tip is at disable height - 1, so new block height == disable height.
    // Avoid retarget boundary.
    CBlockIndex pindexLast;
    pindexLast.pprev = nullptr;
    pindexLast.nHeight = consensus.nMinDiffDisableHeight - 1;
    BOOST_REQUIRE((pindexLast.nHeight + 1) % consensus.DifficultyAdjustmentInterval() != 0);
    pindexLast.nTime = 1714777860;
    pindexLast.nBits = normalBits;

    // Block with 21-minute gap — would have triggered min-difficulty before the fork
    CBlockHeader block;
    block.nTime = pindexLast.nTime + consensus.nPowTargetSpacing * 2 + 60;

    // At the fork height, the previous difficulty must be preserved
    BOOST_CHECK_EQUAL(GetNextWorkRequired(&pindexLast, &block, consensus), normalBits);
}

/* Test that min-difficulty blocks are rejected well after nMinDiffDisableHeight */
BOOST_AUTO_TEST_CASE(mindiff_after_disable_height)
{
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::TESTNET4);
    const auto& consensus = chainParams->GetConsensus();

    const unsigned int normalBits = 0x1c0fffff;

    // Tip is well past the disable height. Avoid retarget boundary.
    CBlockIndex pindexLast;
    pindexLast.pprev = nullptr;
    pindexLast.nHeight = consensus.nMinDiffDisableHeight + 1000;
    BOOST_REQUIRE((pindexLast.nHeight + 1) % consensus.DifficultyAdjustmentInterval() != 0);
    pindexLast.nTime = 1714777860;
    pindexLast.nBits = normalBits;

    // Large gap — 1 hour
    CBlockHeader block;
    block.nTime = pindexLast.nTime + 3600;

    // After the fork, must keep the real difficulty
    BOOST_CHECK_EQUAL(GetNextWorkRequired(&pindexLast, &block, consensus), normalBits);
}

/* Test PermittedDifficultyTransition respects nMinDiffDisableHeight */
BOOST_AUTO_TEST_CASE(permitted_difficulty_transition_mindiff_disable)
{
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::TESTNET4);
    const auto& consensus = chainParams->GetConsensus();

    const unsigned int normalBits = 0x1c0fffff;
    const unsigned int powLimitBits = UintToArith256(consensus.powLimit).GetCompact();

    // Before the fork: any transition is permitted (min-difficulty allowed)
    BOOST_CHECK(PermittedDifficultyTransition(consensus, consensus.nMinDiffDisableHeight - 1, normalBits, powLimitBits));

    // At the fork height: transition to powLimit is no longer blanket-permitted
    BOOST_CHECK(!PermittedDifficultyTransition(consensus, consensus.nMinDiffDisableHeight, normalBits, powLimitBits));

    // After the fork height: same
    BOOST_CHECK(!PermittedDifficultyTransition(consensus, consensus.nMinDiffDisableHeight + 100, normalBits, powLimitBits));
}

BOOST_AUTO_TEST_CASE(ChainParams_MAIN_sanity)
{
    sanity_check_chainparams(*m_node.args, ChainType::MAIN);
}

BOOST_AUTO_TEST_CASE(ChainParams_REGTEST_sanity)
{
    sanity_check_chainparams(*m_node.args, ChainType::REGTEST);
}

BOOST_AUTO_TEST_CASE(ChainParams_TESTNET_sanity)
{
    sanity_check_chainparams(*m_node.args, ChainType::TESTNET);
}

BOOST_AUTO_TEST_CASE(ChainParams_TESTNET4_sanity)
{
    sanity_check_chainparams(*m_node.args, ChainType::TESTNET4);
}

BOOST_AUTO_TEST_CASE(ChainParams_SIGNET_sanity)
{
    sanity_check_chainparams(*m_node.args, ChainType::SIGNET);
}

BOOST_AUTO_TEST_SUITE_END()
