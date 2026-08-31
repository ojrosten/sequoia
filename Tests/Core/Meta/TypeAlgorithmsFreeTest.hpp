////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2025.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/PlatformSpecific/Macros.hpp"
#include "sequoia/TestFramework/Macros.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <concepts>
#include <execution>
#include <filesystem>
#include <format>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <scoped_allocator>
#include <source_location>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

import sequoia.test_framework;

/** \file */


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
    void test_find_if();

    template<template<class...> class TT>
    void test_contains();

    template<template<class...> class TT>
    void test_erase();

    template<template<class...> class TT>
    void test_insert();

    template<template<class...> class TT>
    void test_flatten();

    template<template<class...> class TT>
    void test_concat();

    template<template<class...> class TT>
    void test_reverse();
  };
}
