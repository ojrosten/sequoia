////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "PhysicalValueTestingUtilities.hpp"

namespace sequoia::testing
{
  /*! \brief Conversion between units, which is a coordinate transform on a fixed space and so
      independent of whether that space is absolute, affine or vector.
   */
  class physical_value_conversions_free_test final : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    std::filesystem::path source_file() const;

    void run_tests();
  private:
    void test_mass_conversions();

    void test_length_conversions();

    void test_area_conversions();

    template<std::floating_point T>
    void test_angle_conversions();
  };
}
