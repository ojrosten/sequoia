////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "TestRunner.hpp"

#include "CoreInfrastructure.hpp"

namespace sequoia::testing
{
  template<>
  struct detailed_equality_checker<template_spec>
  {    
    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const template_spec& obtained, const template_spec& prediction)
    {
      check_equality("Species", logger, obtained.species, prediction.species);
      check_equality("Symbol",  logger, obtained.symbol,  prediction.symbol);
    }
  };
}
