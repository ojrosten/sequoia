////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

module;

// stderr is a macro, so import std does not supply it
#include <cstdio>

module sequoia.test_framework;

import std;

/** \file
    \brief Definitions for FailureReporting.cppm
 */

namespace sequoia::testing
{
  [[nodiscard]]
  std::string describe_exception(std::exception_ptr e)
  {
    if(!e)
      return "none";

    try
    {
      std::rethrow_exception(e);
    }
    catch(const std::exception& caught)
    {
      return std::format("{}: {}", demangle(typeid(caught)), caught.what());
    }
    catch(...)
    {
      return "an exception not derived from std::exception";
    }
  }

  void report_termination() noexcept
  {
    // Describing the exception may throw, and a throw from a terminate handler ends the process before anything is
    // written
    try
    {
      const auto message{
        std::format("std::terminate called; current exception: {}\n", describe_exception(std::current_exception()))
      };
      std::fputs(message.c_str(), stderr);
    }
    catch(...)
    {
      std::fputs("std::terminate called; the current exception could not be described\n", stderr);
    }

    std::fflush(stderr);
    std::abort();
  }

  scoped_terminate_handler::scoped_terminate_handler(std::terminate_handler handler)
    : m_Replaced{std::set_terminate(handler)}
  {}

  scoped_terminate_handler::~scoped_terminate_handler()
  {
    std::set_terminate(m_Replaced);
  }
}
