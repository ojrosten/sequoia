////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "UnitTestUtils.hpp"

namespace sequoia::unit_testing
{
  class type_traits_test : public unit_test
  {
  public:
    using unit_test::unit_test;

  private:
    void run_tests() override;

    void test_variadic_traits();

    void test_resolve_to_copy_constructor();
  };
}
