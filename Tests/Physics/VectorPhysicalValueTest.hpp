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
  class vector_physical_value_test final : public regular_test
  {
  public:
    using regular_test::regular_test;

    [[nodiscard]]
    std::filesystem::path source_file() const;

    void run_tests();
  private:
    template<class Quantity>
    void test_vector_quantity();

    template<std::floating_point T>
    void test_trig();
  };

  // TO DO: consider necessity of this; it's only used by the
  // check of asin within_tolerance; perhaps the latter could
  // be worked to delegate to test(equality...)
  template<class HostSystem, std::floating_point T>
  struct serializer<physics::quantity<physics::angular_space<T, HostSystem>, physics::si::units::radian_t>>
  {
    [[nodiscard]]
    static std::string make(physics::quantity<physics::angular_space<T, HostSystem>, physics::si::units::radian_t> theta)
    {
      return std::format("{}", theta.value());
    }
  };
}
