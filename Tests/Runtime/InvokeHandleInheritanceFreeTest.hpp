////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "sequoia/TestFramework/FreeTestCore.hpp"

namespace sequoia::testing
{
  /** \brief Checks that a process launched by `invoke` does not inherit the caller's open files.

      On Windows an inherited handle keeps a file undeletable for as long as the process holding
      it lives, so a spawn which snapshots the handle table while a stream is open can defeat a
      delete issued long after that stream has closed. POSIX unlinks open files, so there the
      checks are satisfied whatever `invoke` does.
   */
  class invoke_handle_inheritance_free_test final : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    static std::filesystem::path source_file();

    void run_tests();
  };
}
