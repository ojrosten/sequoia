////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "ConcurrencyModelsTest.hpp"
#include "sequoia/Core/Concurrency/ConcurrencyModels.hpp"

#include <queue>
#include <semaphore>
#include <thread>
#include <utility>

namespace sequoia::testing
{
  using namespace concurrency;

  namespace
  {
    /** \brief A queue whose first push stalls until `release_stall` is called.

        When it underlies a `task_queue`, the stall happens with the `task_queue`'s mutex held.
     */
    template<class T>
    class stalling_queue
    {
    public:
      static void await_stall() { m_StallBegun.acquire(); }

      static void release_stall() { m_StallReleased.release(); }

      void push(T&& task)
      {
        // Before the stall, so that a try-pop made during the stall can fail only on the mutex, not on an empty queue
        m_Q.push(std::move(task));

        if(!std::exchange(m_HasStalled, true))
        {
          m_StallBegun.release();
          m_StallReleased.acquire();
        }
      }

      [[nodiscard]]
      bool empty() const noexcept { return m_Q.empty(); }

      [[nodiscard]]
      T& front() { return m_Q.front(); }

      void pop() { m_Q.pop(); }
    private:
      inline static std::binary_semaphore m_StallBegun{0}, m_StallReleased{0};

      std::queue<T> m_Q;
      bool m_HasStalled{};
    };

    using int_task            = std::packaged_task<int()>;
    using stalling_task_queue = task_queue<int, int_task, stalling_queue<int_task>>;

    /** \brief Pushes a task onto a `stalling_task_queue` from another thread.

        Construction completes once the push has stalled, holding the queue's mutex; destruction releases the stall
        and joins the thread.
     */
    class stalled_push
    {
    public:
      stalled_push(stalling_task_queue& q, int_task task)
        : m_Pusher{[&q, task{std::move(task)}]() mutable { q.push(std::move(task)); }}
      {
        stalling_queue<int_task>::await_stall();
      }

      ~stalled_push() { stalling_queue<int_task>::release_stall(); }
    private:
      std::jthread m_Pusher;
    };
  }

  [[nodiscard]]
  std::filesystem::path threading_models_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void threading_models_test::run_tests()
  {
    test_task_queue();
    test_try_lock_failures();

    test_exceptions<thread_pool<void>>("pool_2M", 2u);
    test_exceptions<thread_pool<void, false>>("pool_2", 2u);
    test_exceptions<asynchronous<void>>("async");

    test_exceptions<thread_pool<int>>("pool_2M", 2u);
    test_exceptions<thread_pool<int, false>>("pool_2", 2u);
    test_exceptions<asynchronous<int>>("async");

    test_execution<thread_pool<int>>("pool_2M", 2u);
    test_execution<thread_pool<int, false>>("pool_2", 2u);
    test_execution<asynchronous<int>>("async");

    test_serial_exceptions();
    test_serial_execution();
  }

  void threading_models_test::test_task_queue()
  {
    {
      using q_t = task_queue<void>;
      using task_t = q_t::task_t;

      q_t q{};

      int a{};
      check("", q.push(task_t{[&a](){ a+= 1; }}, std::try_to_lock));
      auto t{q.pop(std::try_to_lock)};

      t();
      check(equality, "", a, 1);

      q.push(task_t{[&a](){ a+= 2; }});
      t = q.pop();

      t();
      check(equality, "", a, 3);

      q.finish();
    }

    {
      using q_t = task_queue<int>;
      using task_t = q_t::task_t;

      q_t q{};

      check("", q.push(task_t{[](){ return 1;}}, std::try_to_lock));
      auto t{q.pop(std::try_to_lock)};

      auto fut{t.get_future()};
      t();

      check(equality, "", fut.get(), 1);

      q.push(task_t{[](){ return 2;}});
      t = q.pop();

      fut = t.get_future();
      t();

      check(equality, "", fut.get(), 2);

      q.finish();
    }
  }

  void threading_models_test::test_try_lock_failures()
  {
    stalling_task_queue q{};

    int_task stalledTask{[](){ return 1; }}, refusedTask{[](){ return 2; }};
    auto stalledFuture{stalledTask.get_future()}, refusedFuture{refusedTask.get_future()};

    {
      const stalled_push stall{q, std::move(stalledTask)};

      check("Try-push fails while the mutex is held", !q.push(std::move(refusedTask), std::try_to_lock));
      check("A task refused by try-push is not moved from", refusedTask.valid());

      const auto poppedTask{q.pop(std::try_to_lock)};
      check("Try-pop yields an empty task while the mutex is held, though a task is queued", !poppedTask.valid());
    }

    check("A task refused by try-push may be pushed again", q.push(std::move(refusedTask), std::try_to_lock));
    q.finish();

    q.pop()();
    q.pop()();

    check(equality, "The stalled push queues its task", stalledFuture.get(), 1);
    check(equality, "The task pushed again runs", refusedFuture.get(), 2);
  }

  template<class ThreadModel, class... Args>
  void threading_models_test::test_exceptions(std::string_view message, Args&&... args)
  {
    ThreadModel model{std::forward<Args>(args)...};
    using R = ThreadModel::return_type;

    auto fut{model.push([]() -> R { throw std::runtime_error{"Error!"}; })};

    check_exception_thrown<std::runtime_error>(message, [&fut]() { return fut.get(); });
  }

  template<class ThreadModel, class... Args>
  void threading_models_test::test_execution(std::string_view message, Args&&... args)
  {
    using R = ThreadModel::return_type;
    ThreadModel model{std::forward<Args>(args)...};

    if constexpr(std::is_void_v<R>)
    {
      int x{};
      auto fut{model.push([&x](){ return ++x; })};
      check(equality, message, fut.get(), 1);
    }
    else
    {
      auto fut{model.push([](){ return 42; })};
      check(equality, message, fut.get(), 42);
    }
  }

  void threading_models_test::test_serial_exceptions()
  {
    check_exception_thrown<std::runtime_error>("", [](){ serial<void>{}.push([]() { throw std::runtime_error{"Error!"}; }); });
    check_exception_thrown<std::runtime_error>("", [](){ return serial<int>{}.push([]() -> int { throw std::runtime_error{"Error!"}; }); });
  }

  void threading_models_test::test_serial_execution()
  {
    {
      serial<int> model{};
      check(equality, "", model.push([](){ return 42; }), 42);
      check(equality, "", model.push([](){ return 43; }), 43);
    }

    {
      serial<void> model{};
      int x{};
      model.push([&x]() { ++x; });
      check(equality, "", x, 1);
    }
  }

}
