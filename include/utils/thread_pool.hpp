
#ifndef UTILS_THREAD_POOL_HPP
#define UTILS_THREAD_POOL_HPP

#include <cassert>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <variant>

#include <iostream>

namespace utils {
    template <typename Ret, typename... Args>
    class thread_pool {
    public:
        enum Status {
            STARTING,
            READY,
            STOPPING,
            STOPPED
        };

    private:
        std::promise<void> m_start_pro;
        std::shared_future<void> m_sfut = m_start_pro.get_future();

        std::uint32_t m_to_process;
        std::vector<std::thread> m_threads;
        Status m_status;

        std::queue<std::pair<std::promise<Ret>, std::tuple<Args...>>> m_queue;
        std::mutex m_mutex;
        std::condition_variable m_cv;

        std::condition_variable m_start_cv;
        std::uint32_t m_started;

    public:
        thread_pool(Ret (*func)(Args...), std::uint32_t max_threads)
            : m_to_process { 0 }
            , m_threads { init_vec(max_threads, &thread_pool::thread_func, this, func, m_sfut) }
            , m_status { STARTING }
            , m_started { 0 }
        {
            {
                std::unique_lock lock { m_mutex };
                m_start_cv.wait(lock, [this, max_threads]() { return m_started == max_threads; });
                m_status = READY;
            }
            m_start_pro.set_value();
        }

    private:
        template <typename... Ts>
        static std::vector<std::thread> init_vec(std::uint32_t threads, Ts &&... args)
        {
            std::vector<std::thread> result;
            result.reserve(threads);
            for (std::uint32_t i = 0; i < threads; ++i)
                result.emplace_back(std::forward<Ts>(args)...);
            return result;
        }

    public:
        thread_pool(thread_pool const &) = delete;
        thread_pool(thread_pool &&) = delete;
        thread_pool &operator=(thread_pool const &) = delete;
        thread_pool &operator=(thread_pool &&) = delete;

        std::future<Ret> submit(Args &&... args)
        {
            std::future<Ret> future;
            {
                std::scoped_lock lock { m_mutex };

                std::promise<Ret> promise {};
                future = promise.get_future();

                m_queue.emplace(std::make_pair(std::move(promise), std::make_tuple(std::forward<Args>(args)...)));
                ++m_to_process;
            }

            m_cv.notify_one();

            return future;
        }

        void stop()
        {
            {
                std::unique_lock lock { m_mutex };
                while (!m_queue.empty())
                    m_queue.pop();
                m_status = STOPPING;
                m_to_process = m_threads.size();
            }

            m_cv.notify_all();

            for (auto &thread : m_threads)
                thread.join();
            m_threads.clear();
            m_status = STOPPED;
        }

        Status status() const
        {
            std::unique_lock lock { m_mutex };
            return m_status;
        }

        ~thread_pool()
        {
            assert(m_status == STOPPED && "Thread pool must be stopped before it's destructed");
        }

    private:
        struct thread_exit {
        };

        void thread_func(Ret (*func)(Args...), std::shared_future<void> start_fut)
        {
            {
                std::unique_lock lock { m_mutex };
                ++m_started;
            }
            m_start_cv.notify_one();
            start_fut.wait();

            try {
                while (true) {
                    auto [promise, args] = ([this]() -> std::pair<std::promise<Ret>, std::tuple<Args...>> {
                        std::unique_lock lock { m_mutex };
                        m_cv.wait(lock, [this]() { return m_to_process || m_status == STOPPING; });
                        if (m_status == STOPPING)
                            throw thread_exit {};

                        auto args = std::move(m_queue.front());
                        m_queue.pop();
                        --m_to_process;
                        return args;
                    }());
                    try {
                        promise.set_value(std::apply(func, args));
                    } catch (...) {
                        promise.set_exception(std::current_exception());
                    }
                }
            } catch (thread_exit &) {
            }
        }
    };

    template <typename Ret, typename... Args>
    thread_pool(Ret (*)(Args...), std::uint32_t) -> thread_pool<Ret, Args...>;
} // namespace utils

#endif