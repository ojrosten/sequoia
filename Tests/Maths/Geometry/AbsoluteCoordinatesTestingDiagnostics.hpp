////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "GeometryTestingUtilities.hpp"

#include "sequoia/PlatformSpecific/Preprocessor.hpp"

namespace sequoia::testing
{
  class absolute_coordinates_false_negative_test final : public regular_false_negative_test
  {
  public:
    using regular_false_negative_test::regular_false_negative_test;

    [[nodiscard]]
    std::filesystem::path source_file() const;

    void run_tests();

    [[nodiscard]]
    std::string output_discriminator() const
    {
      return compiler_name();
    }
  private:
    template<std::floating_point T, std::size_t D>
    void test_absolute();
  };
}
