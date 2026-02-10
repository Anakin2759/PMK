#include "Server.h"
#include <asio.hpp>
#include <cstdint>
#include <utility>

// Pimpl 实现
struct Server::Impl
{
    asio::io_context ioc;
    asio::thread_pool pool;

    explicit Impl(size_t thread_count) : ioc(), pool(thread_count) {}

    // 玩家业务协程
    static asio::awaitable<void> playerRoutine(uint32_t conv, std::shared_ptr<KcpSession> session)
    {
        try
        {
            while (true)
            {
                // 使用回调模式接收数据
                std::optional<std::expected<KcpSession::Packet, std::error_code>> result;
                session->recvAsync([&result](auto res) { result = std::move(res); });

                // 等待结果（简化示例，实际应使用同步机制）
                co_await asio::steady_timer(co_await asio::this_coro::executor, std::chrono::milliseconds(10))
                    .async_wait(asio::use_awaitable);

                if (!result || !result->has_value())
                {
                    break;
                }

                const auto& msg = **result;
                // 处理业务逻辑
                session->send(msg);
            }
        }
        catch (const std::exception&)
        {
        }
    }
};

Server::Server(IUdpTransport& transport, size_t thread_count)
    : KcpEndpoint(transport), m_impl(std::make_unique<Impl>(thread_count))
{
}

Server::~Server() = default;

void Server::stop()
{
    m_impl->pool.join();
}

uint32_t Server::selectConv([[maybe_unused]] const NetAddress& from, std::span<const uint8_t> data)
{
    return peekConv(data);
}

std::shared_ptr<KcpSession> Server::createSession(uint32_t conv, const NetAddress& peer)
{
    return std::make_shared<KcpSession>(conv, m_transport, peer, m_impl->ioc.get_executor());
}

void Server::onSession(std::uint32_t conv, std::shared_ptr<KcpSession> session)
{
    auto player_executor = asio::make_strand(m_impl->pool.get_executor());
    asio::co_spawn(player_executor, Impl::playerRoutine(conv, std::move(session)), asio::detached);
}