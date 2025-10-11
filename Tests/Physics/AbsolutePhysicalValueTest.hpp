////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "PhysicalValueTestingUtilities.hpp"

namespace sequoia::testing
{
  class absolute_physical_value_test final : public regular_test
  {
  public:
    using regular_test::regular_test;

    [[nodiscard]]
    std::filesystem::path source_file() const;

    void run_tests();
  private:
    template<class Quantity>
    void test_absolute_quantity();

    template<class Quantity>
    void test_compositions();

    void test_mass_conversions();

    void test_length_conversions();

    void test_area_conversions();
  };
}
