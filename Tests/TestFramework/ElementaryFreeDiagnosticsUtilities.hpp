////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/FreeCheckers.hpp"

namespace sequoia::testing
{
  struct bland
  {
    [[nodiscard]]
    std::string operator()(int, int) const
    {
      return "Integer advice";
    }

    [[nodiscard]]
    std::string operator()(double, double) const
    {
      return "Double advice";
    }
  };

  struct perfectly_normal_type
  {
    int i{};

    [[nodiscard]]
    friend constexpr auto operator<=>(const perfectly_normal_type&, const perfectly_normal_type&) = default;
  };

  template<>
  struct value_tester<perfectly_normal_type>
  {
    template<test_mode Mode, class Advisor>
    static void test(equality_check_t, test_logger<Mode>& logger, const perfectly_normal_type& obtained, const perfectly_normal_type& prediction, const tutor<Advisor>& advisor)
    {
      check(equality, "Wrapped value", logger, obtained.i, prediction.i, advisor);
    }

    template<test_mode Mode, class Advisor>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const perfectly_normal_type& f, int i, const tutor<Advisor>& advisor)
    {
      check(equality, "Wrapped value", logger, f.i, i, advisor);
    }

    template<test_mode Mode, class Advisor>
    static void test(weak_equivalence_check_t, test_logger<Mode>& logger, const perfectly_normal_type& f, double d, const tutor<Advisor>& advisor)
    {
      check(equality, "Wrapped value", logger, f.i, static_cast<int>(d), advisor);
    }
  };

  struct perfectly_serializable_type
  {
    int i{};

    [[nodiscard]]
    friend constexpr auto operator<=>(const perfectly_serializable_type&, const perfectly_serializable_type&) = default;

    friend std::ostream& operator<<(std::ostream& s, const perfectly_serializable_type& x)
    {
      return s << x.i;      
    }
  };

  struct perfectly_nonserializable_type
  {
    int i{};

    [[nodiscard]]
    friend constexpr auto operator<=>(const perfectly_nonserializable_type&, const perfectly_nonserializable_type&) = default;
  };

  template<>
  struct serializer<perfectly_nonserializable_type>
  {
    [[nodiscard]]
    static std::string make(const perfectly_nonserializable_type& x) { return std::format("{}", x.i); }
  };
  
  struct only_equivalence_checkable
  {
    only_equivalence_checkable(double val) : x{val} {}
    
    double x{};
  };

  struct only_weakly_checkable
  {
    only_weakly_checkable(int j, double y) : i{j}, x{y} {}
    
    int i{};
    double x{};
  };

  template<>
  struct value_tester<only_equivalence_checkable>
  {
    template<test_mode Mode, class Advisor>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const only_equivalence_checkable& obtained, double prediction, const tutor<Advisor>& advisor)
    {
      check(equality, "Wrapped double", logger, obtained.x, prediction, advisor);
    }

    template<test_mode Mode, class Advisor>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const only_equivalence_checkable& obtained, const only_equivalence_checkable& prediction, const tutor<Advisor>& advisor)
    {
      check(equality, "Wrapped double", logger, obtained.x, prediction.x, advisor);
    }
  };

  template<>
  struct value_tester<only_weakly_checkable>
  {
    template<test_mode Mode, class Advisor>
    static void test(weak_equivalence_check_t, test_logger<Mode>& logger, const only_weakly_checkable& obtained, const std::pair<int, double>& prediction, const tutor<Advisor>& advisor)
    {
      check(equality, "Wrapped int", logger, obtained.i, prediction.first, advisor);
      check(equality, "Wrapped double", logger, obtained.x, prediction.second, advisor);
    }

    template<test_mode Mode, class Advisor>
    static void test(weak_equivalence_check_t, test_logger<Mode>& logger, const only_weakly_checkable& obtained, const only_weakly_checkable& prediction, const tutor<Advisor>& advisor)
    {
      check(equality, "Wrapped int", logger, obtained.i, prediction.i, advisor);
      check(equality, "Wrapped double", logger, obtained.x, prediction.x, advisor);
    }
  };
}
