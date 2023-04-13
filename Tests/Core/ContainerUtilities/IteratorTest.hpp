////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/RegularTestCore.hpp"

namespace sequoia::testing
{
  class iterator_test final : public regular_test
  {
  public:
    using regular_test::regular_test;

    [[nodiscard]]
    std::filesystem::path source_file() const noexcept final;
  private:
    void run_tests() final;

    template<
      std::input_or_output_iterator CustomIter,
      std::input_or_output_iterator Iter,
      std::sentinel_for<Iter> Sentinel,
      class... Args,
      class Pointer=typename CustomIter::pointer
    >
    void basic_checks(Iter begin, Sentinel end, Pointer pBegin, std::string_view message, Args... args);

    void test_iterator();
    void test_const_iterator();
    void test_reverse_iterator();
    void test_const_reverse_iterator();

    void test_const_scaling_iterator();
    void test_const_reverse_scaling_iterator();
  };
}
