////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Timing of tasks.
 */

#include "sequoia/PlatformSpecific/Macros.hpp"

#include <chrono>
#include <concepts>
#include <type_traits>

#if defined(SEQUOIA_MSVC_COMPILER)
  #include <atomic>
  #include <memory>
#endif

namespace sequoia::testing
{
  class [[nodiscard]] timer
  {
  public:
    timer();

    [[nodiscard]]
    std::chrono::nanoseconds time_elapsed() const;
  private:
    using time_point = std::chrono::steady_clock::time_point;

    time_point m_Start;
  };

  /** \brief Prevents the optimizer from removing the computation of `value`.

      The compiler must treat `value` as read at the call, and as possibly
      changed by it. So an optimized build computes `value` before the call,
      even if nothing else reads `value`. And a computation after the call
      which depends on the state of `value` is made after the call.

      The compiler may still compute `value` earlier, at any point after the
      inputs to the computation are known. That may be during compilation.
   */
  template<class T>
  void do_not_optimize_away(const T& value) noexcept
  {
#if defined(SEQUOIA_MSVC_COMPILER)
    // The store lets code outside this function reach `value`. MSVC compiles
    // the fence as a compiler barrier: every write to memory such code can
    // reach completes before the fence, and every read of it after the fence
    // is made afresh. Without either, the computation of `value` may be lost.
    static thread_local const T* volatile observedAddress{};
    observedAddress = std::addressof(value);
    std::atomic_signal_fence(std::memory_order_seq_cst);
#else
    // The memory clobber makes the compiler treat all memory as possibly read
    // and changed: both `value` and memory it points to, such as a vector's
    // elements. Without it, only the bytes of `value` would be read.
    asm volatile("" : : "m"(value) : "memory");
#endif
  }

  /** \brief Returns the duration of a call of `task`.

      The duration includes the computation of any value the call returns,
      even if nothing else reads the value.
   */
  template<class Task>
    requires std::invocable<Task&>
  [[nodiscard]]
  std::chrono::duration<double> profile(Task task)
  {
    const timer t{};
    do_not_optimize_away(task);

    if constexpr(std::is_void_v<std::invoke_result_t<Task&>>)
      task();
    else
      do_not_optimize_away(task());

    return t.time_elapsed();
  }
}
