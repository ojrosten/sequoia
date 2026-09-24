////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "sequoia/TestFramework/TestCreator.hpp"
#include "sequoia/TestFramework/TestRunner.hpp"
#include "sequoia/TestFramework/CoreInfrastructure.hpp"
#include "sequoia/TextProcessing/Substitutions.hpp"

namespace sequoia::testing
{
  /** \brief An exception-message postprocessor which, unlike the default one, makes every path
             beneath the project root relative to it, not only the first.
   */
  [[nodiscard]]
  inline std::string relative_to_root(const project_paths& projPaths, std::string message)
  {
    replace_all(message, projPaths.project_root().generic_string() + "/", "");
    return message;
  }

  template<>
  struct value_tester<template_spec>
  {
    template<test_mode Mode>
    static void test(equality_check_t, test_logger<Mode>& logger, const template_spec& obtained, const template_spec& prediction)
    {
      check(equality, "Species", logger, obtained.species, prediction.species);
      check(equality, "Symbol",  logger, obtained.symbol,  prediction.symbol);
    }
  };

  class bar_free_test final : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    static std::filesystem::path source_file();

    void run_tests()
    {
      check(equality, {"Phoney equality check"}, 1, 1);
      throw std::runtime_error{"Throw after check"};
    }
  };
}
