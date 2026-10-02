////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "ConcurrencyModelsTest.hpp"
#include "sequoia/Core/Concurrency/ConcurrencyModels.hpp"

#include <future>
#include <queue>
#include <semaphore>
#include <thread>
#include <utility>

namespace sequoia::testing
{
  using namespace concurrency;

  namespace
  {
    using int_task = std::packaged_task<int()>;

    /** \brief A queue whose first push stalls until `release_stall` is called.

        \pre At most one push is stalled at a time, across every instance.
     */
    class stalling_queue
    {
    public:
      static void await_stall() { st_StallBegun.acquire(); }

      static void release_stall() { st_StallReleased.release(); }

      void push(int_task&& task)
      {
        // The task is queued first, so a try-pop during the stall can fail only on the mutex, not on an empty queue
        m_Q.push(std::move(task));

        if(!std::exchange(m_HasStalled, true))
        {
          st_StallBegun.release();
          st_StallReleased.acquire();
        }
      }

      [[nodiscard]]
      bool empty() const noexcept { return m_Q.empty(); }

      [[nodiscard]]
      int_task& front() { return m_Q.front(); }

      void pop() { m_Q.pop(); }
    private:
      // The semaphores are static: a task_queue default-constructs its queue and gives no access to the queue
      inline static std::binary_semaphore st_StallBegun{0}, st_StallReleased{0};

      std::queue<int_task> m_Q;
      bool m_HasStalled{};
    };

    // A task_queue holds its mutex while pushing onto its underlying queue. So the first push onto a
    // stalling_task_queue stalls with the mutex held.
    using stalling_task_queue = task_queue<int, int_task, stalling_queue>;

    /** \brief An RAII wrapper to push a task onto a `stalling_task_queue` from another thread.

        The constructor returns once the push has stalled with the queue's mutex held. The destructor releases the
        stall, then joins the thread.
     */
    class stalled_push
    {
    public:
      stalled_push(stalling_task_queue& q, int_task task)
        : m_Pusher{[&q, task{std::move(task)}]() mutable { q.push(std::move(task)); }}
      {
        stalling_queue::await_stall();
      }

      stalled_push(const stalled_push&)            = delete;
      stalled_push& operator=(const stalled_push&) = delete;

      ~stalled_push() { stalling_queue::release_stall(); }
    private:
      std::jthread m_Pusher;
    };

    /** \brief A queue which signals the first time it is found empty.

        \pre At most one signal is unawaited at a time, across every instance.
     */
    class wait_signalling_queue
    {
    public:
      static void await_found_empty() { st_FoundEmpty.acquire(); }

      void push(int_task&& task) { m_Q.push(std::move(task)); }

      [[nodiscard]]
      bool empty()
      {
        const bool isEmpty{m_Q.empty()};
        if(isEmpty && !std::exchange(m_HasSignalled, true))
        {
          st_FoundEmpty.release();
        }

        return isEmpty;
      }

      [[nodiscard]]
      int_task& front() { return m_Q.front(); }

      void pop() { m_Q.pop(); }
    private:
      // The semaphore is static: a task_queue default-constructs its queue and gives no access to the queue
      inline static std::binary_semaphore st_FoundEmpty{0};

      std::queue<int_task> m_Q;
      bool m_HasSignalled{};
    };

    // A blocking pop from a task_queue holds the mutex from finding the queue empty until the pop waits. So once a
    // pop from a wait_signalling_task_queue has signalled, a push can take the mutex only after the pop has begun
    // waiting.
    using wait_signalling_task_queue = task_queue<int, int_task, wait_signalling_queue>;

    /** \brief An RAII wrapper to pop a task from a `wait_signalling_task_queue` on another thread.

        The constructor returns once the pop has found the queue empty. The destructor finishes the queue, so that a
        pop still waiting returns, then waits for the pop to return.
     */
    class waiting_pop
    {
    public:
      explicit waiting_pop(wait_signalling_task_queue& q)
        : m_Queue{q}
        , m_Pop{std::async(std::launch::async, [&q](){ return q.pop(); })}
      {
        wait_signalling_queue::await_found_empty();
      }

      waiting_pop(const waiting_pop&)            = delete;
      waiting_pop& operator=(const waiting_pop&) = delete;

      ~waiting_pop() { m_Queue.finish(); }

      [[nodiscard]]
      int_task wait_for_task() { return m_Pop.get(); }
    private:
      wait_signalling_task_queue& m_Queue;
      std::future<int_task> m_Pop;
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
    test_try_lock_successes();
    test_try_lock_failures();
    test_pushes_wake_a_waiting_pop();

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
      using task_t = q_t::task_type;

      q_t q{};

      int a{};
      q.push(task_t{[&a](){ a+= 1; }});
      auto t{q.pop()};

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
      using task_t = q_t::task_type;

      q_t q{};

      q.push(task_t{[](){ return 1;}});
      auto t{q.pop()};

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

  void threading_models_test::test_try_lock_successes()
  {
    // std::mutex::try_lock may fail spuriously, so each try-pop and try-push below is repeated until it succeeds
    task_queue<int> q{};

    {
      int_task queuedTask{[](){ return 1; }};
      auto queuedFuture{queuedTask.get_future()};
      q.push(std::move(queuedTask));

      int_task poppedTask{};
      while(!poppedTask.valid())
      {
        poppedTask = q.pop(std::try_to_lock);
      }

      poppedTask();
      check(equality, "A successful try-pop yields the queued task", queuedFuture.get(), 1);
    }

    {
      int_task tryPushedTask{[](){ return 2; }};
      auto tryPushedFuture{tryPushedTask.get_future()};
      while(!q.push(std::move(tryPushedTask), std::try_to_lock)) {}

      // Finishing the queue turns a missing task into an exception, not a hang. A pop on a finished, empty queue
      // returns an empty task, and invoking that task throws.
      q.finish();

      q.pop()();
      check(equality, "A successful try-push queues its task", tryPushedFuture.get(), 2);
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

    q.push(std::move(refusedTask));

    // Finishing the queue turns a missing task into an exception, not a hang. A pop on a finished, empty queue
    // returns an empty task, and invoking that task throws.
    q.finish();

    q.pop()();
    q.pop()();

    check(equality, "The stalled push queues its task", stalledFuture.get(), 1);
    check(equality, "The refused task runs once pushed again", refusedFuture.get(), 2);
  }

  void threading_models_test::test_pushes_wake_a_waiting_pop()
  {
    // A push which does not wake the waiting pop leaves wait_for_task blocked, so the test hangs rather than fails
    {
      wait_signalling_task_queue q{};
      waiting_pop waitingPop{q};

      int_task pushedTask{[](){ return 1; }};
      auto pushedFuture{pushedTask.get_future()};
      q.push(std::move(pushedTask));

      waitingPop.wait_for_task()();
      check(equality, "The waiting pop yields the pushed task", pushedFuture.get(), 1);
    }

    {
      wait_signalling_task_queue q{};
      waiting_pop waitingPop{q};

      int_task tryPushedTask{[](){ return 2; }};
      auto tryPushedFuture{tryPushedTask.get_future()};

      // The try-push fails until the pop waits, and may also fail spuriously
      while(!q.push(std::move(tryPushedTask), std::try_to_lock)) {}

      waitingPop.wait_for_task()();
      check(equality, "The waiting pop yields the try-pushed task", tryPushedFuture.get(), 2);
    }
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
