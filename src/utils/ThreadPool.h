#pragma once

#include <cstddef>
#include <thread>

// 防止 Windows.h 宏污染 ASIO 的 execution 命名空间
#include <asio.hpp>

namespace utils
{

class ThreadPool
{
public:
    template <typename F>
        requires std::invocable<F>
    static auto enqueue(F&& func) -> std::future<std::invoke_result_t<F>>
    {
        using R = std::invoke_result_t<F>;

        std::promise<R> promise;
        auto future = promise.get_future();

        // C++23：使用 move_only_function，语义明确
        std::move_only_function<void()> task = [pro = std::move(promise), fn = std::forward<F>(func)]() mutable
        {
            try
            {
                if constexpr (std::is_void_v<R>)
                {
                    std::invoke(fn);
                    pro.set_value();
                }
                else
                {
                    pro.set_value(std::invoke(fn));
                }
            }
            catch (...)
            {
                pro.set_exception(std::current_exception());
            }
        };

        asio::post(instance(), std::move(task));
        return future;
    }

    static void shutdown() noexcept
    {
        instance().stop();
        instance().join();
    }

    // 禁止拷贝和移动
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

private:
    // 获取全局线程池实例(单例模式)
    static asio::thread_pool& instance(size_t threadCount = std::thread::hardware_concurrency())
    {
        static asio::thread_pool instance(threadCount);
        return instance;
    }
    ThreadPool() = default;
    ~ThreadPool() = default;
};
} // namespace utils
