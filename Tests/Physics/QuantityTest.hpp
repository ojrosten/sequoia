////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "QuantityTestingUtilities.hpp"

namespace sequoia::testing
{
  class quantity_test final : public regular_test
  {
  public:
    using regular_test::regular_test;

    [[nodiscard]]
    std::filesystem::path source_file() const;

    void run_tests();

    template<class Quantity>
    void test_absolute_quantity();

    template<class Quantity>
    void test_affine_quantity();

    template<class Quantity>
    void test_vector_quantity();

    void test_temperatures();
    void test_mixed();
  };
}
