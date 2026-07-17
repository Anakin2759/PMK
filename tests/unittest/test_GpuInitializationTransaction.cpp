#include "src/systems/render/GpuInitializationTransaction.hpp"

#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

namespace ui::systems::render::tests
{
namespace
{
using Transaction = GpuInitializationTransaction;
using Node = Transaction::Node;

void CommitThrough(Transaction& transaction, Node lastNode)
{
    ASSERT_TRUE(transaction.Begin());
    ASSERT_TRUE(transaction.Commit(Node::PIPELINE_CACHE));
    if (lastNode == Node::PIPELINE_CACHE)
    {
        return;
    }
    ASSERT_TRUE(transaction.Commit(Node::TEXT_TEXTURE_CACHE));
    if (lastNode == Node::TEXT_TEXTURE_CACHE)
    {
        return;
    }
    ASSERT_TRUE(transaction.Commit(Node::COMMAND_BUFFER));
}

TEST(GpuInitializationTransactionTest, CompletesFixedInitializationOrder)
{
    Transaction transaction;

    CommitThrough(transaction, Node::COMMAND_BUFFER);

    EXPECT_TRUE(transaction.Complete());
    EXPECT_TRUE(transaction.IsReady());
    EXPECT_EQ(transaction.CommittedCount(), 3U);
}

TEST(GpuInitializationTransactionTest, RejectsOutOfOrderCommit)
{
    Transaction transaction;

    ASSERT_TRUE(transaction.Begin());
    EXPECT_FALSE(transaction.Commit(Node::TEXT_TEXTURE_CACHE));
    EXPECT_EQ(transaction.CommittedCount(), 0U);
}

TEST(GpuInitializationTransactionTest, FirstNodeFailureHasNothingToRollback)
{
    Transaction transaction;
    std::vector<Node> released;
    ASSERT_TRUE(transaction.Begin());

    transaction.FailAndRollback([&released](Node node) { released.push_back(node); });

    EXPECT_TRUE(transaction.IsFailed());
    EXPECT_TRUE(released.empty());
    EXPECT_FALSE(transaction.Begin());
}

TEST(GpuInitializationTransactionTest, SecondNodeFailureRollsBackPipeline)
{
    Transaction transaction;
    std::vector<Node> released;
    CommitThrough(transaction, Node::PIPELINE_CACHE);

    transaction.FailAndRollback([&released](Node node) { released.push_back(node); });

    EXPECT_EQ(released, std::vector{Node::PIPELINE_CACHE});
    EXPECT_EQ(transaction.CommittedCount(), 0U);
}

TEST(GpuInitializationTransactionTest, ThirdNodeFailureRollsBackInReverseOrder)
{
    Transaction transaction;
    std::vector<Node> released;
    CommitThrough(transaction, Node::TEXT_TEXTURE_CACHE);

    transaction.FailAndRollback([&released](Node node) { released.push_back(node); });

    EXPECT_EQ(released, (std::vector{Node::TEXT_TEXTURE_CACHE, Node::PIPELINE_CACHE}));
}

TEST(GpuInitializationTransactionTest, ReadyShutdownUsesReverseOrderExactlyOnce)
{
    Transaction transaction;
    std::vector<Node> released;
    CommitThrough(transaction, Node::COMMAND_BUFFER);
    ASSERT_TRUE(transaction.Complete());

    const auto cleanup = [&released](Node node) { released.push_back(node); };
    transaction.Shutdown(cleanup);
    transaction.Shutdown(cleanup);

    EXPECT_EQ(released,
              (std::vector{Node::COMMAND_BUFFER, Node::TEXT_TEXTURE_CACHE, Node::PIPELINE_CACHE}));
    EXPECT_EQ(transaction.GetState(), Transaction::State::SHUTDOWN);
}

TEST(GpuInitializationTransactionTest, ShutdownBeforeBeginIsSafe)
{
    Transaction transaction;
    std::vector<Node> released;

    transaction.Shutdown([&released](Node node) { released.push_back(node); });

    EXPECT_TRUE(released.empty());
    EXPECT_EQ(transaction.GetState(), Transaction::State::SHUTDOWN);
}

TEST(GpuInitializationTransactionTest, CleanupExceptionDoesNotSkipRemainingNodes)
{
    Transaction transaction;
    std::vector<Node> released;
    CommitThrough(transaction, Node::COMMAND_BUFFER);
    ASSERT_TRUE(transaction.Complete());

    transaction.Shutdown(
        [&released](Node node)
        {
            released.push_back(node);
            if (node == Node::TEXT_TEXTURE_CACHE)
            {
                throw std::runtime_error("expected cleanup failure");
            }
        });

    EXPECT_EQ(released,
              (std::vector{Node::COMMAND_BUFFER, Node::TEXT_TEXTURE_CACHE, Node::PIPELINE_CACHE}));
    EXPECT_EQ(transaction.CommittedCount(), 0U);
    EXPECT_TRUE(transaction.HadCleanupFailure());
}

} // namespace
} // namespace ui::systems::render::tests
