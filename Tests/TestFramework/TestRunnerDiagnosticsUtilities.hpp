////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/TestFramework/TestCreator.hpp"
#include "sequoia/TestFramework/TestRunner.hpp"
#include "sequoia/TestFramework/CoreInfrastructure.hpp"

namespace sequoia::testing
{
  template<>
  struct value_checker<template_spec>
  {
    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const template_spec& obtained, const template_spec& prediction)
    {
      check_equality("Species", logger, obtained.species, prediction.species);
      check_equality("Symbol",  logger, obtained.symbol,  prediction.symbol);
    }
  };

  class bar_free_test final : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    std::string_view source_file() const noexcept final;
  private:
    void run_tests() final
    {
      check_equality("Phoney equality check", 1, 1);
      throw std::runtime_error{"Throw after check"};
    }
  };
}
