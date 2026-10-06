////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

export module sequoia.test_framework:FailureReporting;

import std;

/** \file
    \brief Describing the current exception when a run ends in `std::terminate`.
 */

export namespace sequoia::testing
{
  /** \brief A description of the exception `e` holds.

      \returns
      -# "none", if `e` is null;
      -# the exception's type and what it says, if it derives from `std::exception`;
      -# "an exception not derived from std::exception", otherwise.
   */
  [[nodiscard]]
  std::string describe_exception(std::exception_ptr e);

  /** \brief A terminate handler: writes `describe_exception` of `std::current_exception()` to standard error, then
             calls `std::abort`.
   */
  [[noreturn]]
  void report_termination() noexcept;

  /** \brief Installs `handler` as the terminate handler; destruction reinstates the previous handler.

      Under MSVC, terminate handlers are per thread, and the calling thread's is the one installed.
   */
  class [[nodiscard]] scoped_terminate_handler
  {
  public:
    explicit scoped_terminate_handler(std::terminate_handler handler);

    scoped_terminate_handler(const scoped_terminate_handler&)            = delete;
    scoped_terminate_handler& operator=(const scoped_terminate_handler&) = delete;

    ~scoped_terminate_handler();
  private:
    std::terminate_handler m_Replaced{};
  };
}
