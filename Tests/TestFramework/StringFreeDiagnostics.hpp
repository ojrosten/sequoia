////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/FreeTestCore.hpp"

namespace sequoia::testing
{
  class string_false_positive_free_diagnostics final : public free_false_positive_test
  {
  public:
    using free_false_positive_test::free_false_positive_test;

  private:
    [[nodiscard]]
    std::filesystem::path source_file() const noexcept final;

    void run_tests() final;

    template<class String> void test_strings();
    template<class String> void test_wstrings();
    void test_string_equivalences();
    void test_wstring_equivalences();
  };
  
  class string_false_negative_free_diagnostics final : public free_false_negative_test
  {
  public:
    using free_false_negative_test::free_false_negative_test;

  private:
    [[nodiscard]]
    std::filesystem::path source_file() const noexcept final;

    void run_tests() final;

    void test_strings();

    void test_string_equivalences();
    void test_wstring_equivalences();
  };
}
