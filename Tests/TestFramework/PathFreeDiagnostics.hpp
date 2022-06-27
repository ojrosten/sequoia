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
  log_summary& postprocess(log_summary& summary, const std::filesystem::path& projectPath);

  class path_false_positive_free_diagnostics final : public free_false_positive_test
  {
  public:
    using free_false_positive_test::free_false_positive_test;

  private:
    [[nodiscard]]
    std::string_view source_file() const noexcept final;

    void run_tests() final;

    [[nodiscard]]
    log_summary summarize(duration delta) const final;

    void test_paths();
  };
  
  class path_false_negative_free_diagnostics final : public free_false_negative_test
  {
  public:
    using free_false_negative_test::free_false_negative_test;

  private:
    [[nodiscard]]
    std::string_view source_file() const noexcept final;

    void run_tests() final;

    [[nodiscard]]
    log_summary summarize(duration delta) const final;

    void test_paths();
  };
}
