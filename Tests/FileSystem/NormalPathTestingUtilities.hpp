////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2025.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/RegularTestCore.hpp"
#include "sequoia/FileSystem/FileSystem.hpp"

namespace sequoia::testing
{
  template<> struct value_tester<normal_path>
  {
    using type = normal_path;

    template<test_mode Mode>
    static void test(equality_check_t, test_logger<Mode>& logger, const type& actual, const type& prediction)
    {
      check(equality, "Wrapped Path", logger, actual.path(), prediction.path());
    }
    
    template<test_mode Mode>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& actual, const std::filesystem::path& prediction)
    {
      check(equality, "Wrapped Path", logger, actual.path(), prediction);
      check(equality, "Conversion to Path", logger, static_cast<std::filesystem::path>(actual), prediction);
    }
  };
}
