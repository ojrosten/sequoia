////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2025.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/FreeTestCore.hpp"

namespace sequoia::testing
{
  class type_algorithms_free_test final : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    std::filesystem::path source_file() const;

    void run_tests();

    void test_type_comparator();

    template<template<class...> class TT>
    void test_lower_bound();

    template<template<class...> class TT>
    void test_filter();

    template<template<class...> class TT>
    void test_drop();

    template<template<class...> class TT>
    void test_keep();

    template<template<class...> class TT>
    void test_merge();

    template<template<class...> class TT>
    void test_stable_sort();

    template<template<class...> class TT>
    void test_find();

    template<template<class...> class TT>
    void test_erase();

    template<template<class...> class TT>
    void test_insert();
  };
}
