////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/RegularTestCore.hpp"

namespace sequoia:: testing
{
  log_summary& postprocess(log_summary& summary, const std::filesystem::path& testRepo);

  class false_positive_diagnostics final : public free_false_positive_test
  {
  public:
    using free_false_positive_test::free_false_positive_test;

    [[nodiscard]]
    std::string_view source_file() const noexcept final;
  private:
    void run_tests() final;

    [[nodiscard]]
    log_summary summarize(duration delta) const override
    {
      auto summary{free_false_positive_test::summarize(delta)};
      return postprocess(summary, this->test_repository());
    }

    void basic_tests();
    void test_exceptions();
    void test_heterogeneous();
    void test_variant();
    void test_optional();
    void test_container_checks();
    template<class String> void test_strings();
    template<class String> void test_wstrings();
    void test_mixed();
    void test_paths();
    void test_function();
    void test_equivalence_checks();
    void test_weak_equivalence_checks();
    void test_with_best_available_checks();
  };

  class false_negative_diagnostics final : public regular_false_negative_test
  {
  public:
    using regular_false_negative_test::regular_false_negative_test;

    [[nodiscard]]
    std::string_view source_file() const noexcept final;
  private:
    void run_tests() final;

    [[nodiscard]]
    log_summary summarize(duration delta) const override
    {
      auto summary{regular_false_negative_test::summarize(delta)};
      return postprocess(summary, this->test_repository());
    }

    void basic_tests();
    void test_exceptions();
    void test_heterogeneous();
    void test_variant();
    void test_optional();
    void test_container_checks();
    void test_strings();
    void test_mixed();
    void test_paths();
    void test_function();
    void test_equivalence_checks();
    void test_weak_equivalence_checks();
    void test_with_best_available_checks();
  };
}
