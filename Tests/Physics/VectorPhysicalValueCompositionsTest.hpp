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
  /** \brief Products and quotients of vector-valued physical values, together with the
      coordinates of the inverse spaces which division brings into being.
   */
  class vector_physical_value_compositions_test final : public regular_test
  {
  public:
    using regular_test::regular_test;

    [[nodiscard]]
    std::filesystem::path source_file() const;

    void run_tests();
  private:
    template<class Quantity>
    void test_compositions();
  };
}
