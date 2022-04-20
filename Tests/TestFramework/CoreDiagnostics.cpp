////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "CoreDiagnostics.hpp"
#include "CoreDiagnosticsUtilities.hpp"
#include "ContainerDiagnosticsUtilities.hpp"
#include "sequoia/TextProcessing/Substitutions.hpp"

#include <complex>
#include <list>
#include <filesystem>
#include <optional>
#include <set>
#include <variant>
#include <vector>

namespace sequoia::testing
{
  namespace
  {
    struct foo
    {
      int i{};

      [[nodiscard]]
      friend constexpr auto operator<=>(const foo&, const foo&) = default;
    };
  }

  template<>
  struct value_tester<foo>
  {
    template<test_mode Mode>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const foo& f, int i, tutor<bland> advisor)
    {
      check(equality, "Wrapped value", logger, f.i, i, advisor);
    }
  };

  // Explicit container specialization to testing propagation of tutor through check_range_equivalence
  template<>
  struct value_tester<std::vector<foo>>
  {
    template<test_mode Mode>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const std::vector<foo>& f, const std::vector<int>& i, tutor<bland> advisor)
    {
      check(equivalence, "Vector equivalence", logger, f.begin(), f.end(), i.begin(), i.end(), advisor);
    }

    using prediction_t = std::vector<std::pair<int, double>>;

    template<test_mode Mode>
    static void test(weak_equivalence_check_t, test_logger<Mode>& logger, const std::vector<foo>& f, const prediction_t& p, tutor<bland> advisor)
    {
      check(equivalence, "Vector equivalence", logger, f.begin(), f.end(), p.begin(), p.end(), advisor);
    }
  };

  [[nodiscard]]
  std::string_view false_positive_diagnostics::source_file() const noexcept
  {
    return __FILE__;
  }

  void false_positive_diagnostics::run_tests()
  {
    basic_tests();
    test_mixed();
    test_function();
    test_equivalence_checks();
    test_weak_equivalence_checks();
    test_with_best_available_checks();
  }

  void false_positive_diagnostics::basic_tests()
  {
    check(LINE(""), false);
    check(LINE(""), false, tutor{[](bool, bool){
        return std::string{"I pity the fool who confuses the bool."};
      }});
    check(LINE("Advisor ignored"), false, tutor{[](const std::string&, const std::string&){
        return std::string{"I pity the fool who confuses the bool."};}
      });

    check(equality, LINE("Integers which should compare different"), 5, 4);
    check(equality, LINE(""), 6.5, 5.6, tutor{[](double, double){
        return std::string{"Double, double, toil and trouble"};
      }});
  }

  void false_positive_diagnostics::test_mixed()
  {
    using t_0 = std::vector<std::pair<int, float>>;
    using t_1 = std::set<double>;
    using t_2 = std::complex<double>;
    using type = std::tuple<t_0, t_1, t_2>;

    type a{t_0{{1, 2.1f}, {2, 2.8f}}, {3.3, -9.6, 3.2}, {1.1, 0.2}};

    {
      type b{t_0{{2, 2.1f}, {2, 2.8f}}, {3.3, -9.6, 3.2}, {1.1, 0.2}};
      check(equality, LINE(""), a, b, tutor{[](int, int){ return "Nested int advice";}});
    }

    {
      type b{t_0{{1, 2.1f}, {2, 2.8f}}, {3.4, -9.6, 3.2}, {1.1, 0.2}};
      check(equality, LINE(""), a, b, tutor{[](const std::set<double>&, const std::set<double>&){
                                       return "Note reordering of elements upon set construction";
                                     }});
    }

    check(equivalence, LINE(""), std::vector<std::string>{ {"a"}, {"b"}}, std::initializer_list<std::string_view>{"a", "c"});

    check(equivalence, LINE(""), std::vector<std::string>{ {"a"}, {"b"}}, std::initializer_list<std::string_view>{"a", "c"}, tutor{[](char, char) {
        return "Ah, chars. So easy to get wrong.";
    }});
  }

  void false_positive_diagnostics::test_function()
  {
    {
      using function = std::function<void()>;
      check(weak_equivalence,
        LINE("Obtained bound but prediction not"),
        function{[]() {}},
        function{});

      check(weak_equivalence,
        LINE("Prediction bound but obtained not"),
        function{},
        function{[]() {}});
    }

    {
      using function = std::function<int()>;
      check(weak_equivalence,
            LINE("Obtained bound but prediction not"),
            function{[]() { return 42; }},
            function{});

      check(weak_equivalence,
            LINE("Prediction bound but obtained not"),
            function{},
            function{[]() { return 42; }});
    }

    {
      using function = std::function<void(int)>;
      check(weak_equivalence,
            LINE("Obtained bound but prediction not"),
            function{[](int) {}},
            function{});

      check(weak_equivalence,
            LINE("Prediction bound but obtained not"),
            function{},
            function{[](int) {}});
    }
  }

  void false_positive_diagnostics::test_equivalence_checks()
  {
    using namespace std::string_literals;
    using namespace std::string_view_literals;
    check(equivalence, LINE("string and string_view"), "foo"s, "fob"sv);

    check(equivalence, LINE("Advice for equivalence checking"), foo{42}, 41, tutor{bland{}});

    check(equivalence,
          LINE("Advice for range equivalence, where the containerized form is explicitly specialized"),
          std::vector<foo>{{42}},
          std::vector<int>{41},
          tutor{bland{}});

    check(equivalence,
          LINE("Advice for range equivalence, where the containerized form is not explicitly specialized"),
          std::set<foo>{{42}},
          std::set<int>{41},
          tutor{bland{}});
  }

  void false_positive_diagnostics::test_weak_equivalence_checks()
  {
    using beast = perfectly_normal_beast<int>;
    check(weak_equivalence, LINE(""), beast{1, 2}, std::initializer_list<int>{1, 1});
    check(weak_equivalence, LINE(""), beast{1, 2}, std::initializer_list<int>{1, 1}, tutor{[](int, int){
        return "Don't mess with the beast.";
      }});

    using prediction = std::initializer_list<std::initializer_list<int>>;
    check(weak_equivalence, LINE(""), std::vector<beast>{{1, 2}, {3, 4}}, prediction{{1, 2}, {3, 5}});

    check(weak_equivalence,
          LINE(""),
          std::vector<beast>{{1, 2}, {3, 4}},
          prediction{{1, 2}, {3, 5}},
          tutor{[](int, int){
            return "Or at least don't mess with a vector of beasts.";
          }});

    check(weak_equivalence,
          LINE("Advice for weak equivalence checking"),
          only_weakly_checkable{42, 3.14},
          std::pair<int, double>{41, 3.13},
          tutor{bland{}});

    check(weak_equivalence,
          LINE("Advice for range weak equivalence, where the containerized form is explicitly specialized"),
          std::vector<only_weakly_checkable>{{42, 3.14}},
          std::vector<std::pair<int, double>>{{41, 3.13}}, tutor{bland{}});

    check(weak_equivalence,
          LINE("Advice for range weak equivalence, where the containerized form is not explicitly specialized"),
          std::list<only_weakly_checkable>{{42, 3.14}},
          std::list<std::pair<int, double>>{{41, 3.13}}, tutor{bland{}});
  }


  void false_positive_diagnostics::test_with_best_available_checks()
  {
    {
      using type = std::pair<int, double>;

      check(with_best_available, LINE(""), type{}, type{1, 0.0});
    }

    {
      using type = std::pair<int, only_weakly_checkable>;

      check(with_best_available, LINE(""), type{1, {0, 2.0}}, type{0, {0, 2.0}});
      check(with_best_available, LINE(""), type{1, {0, 2.0}}, type{1, {0, 2.1}});
      check(with_best_available, LINE(""), type{1, {0, 2.0}}, type{0, {0, 2.1}});
      check(with_best_available, LINE(""), type{1, {0, 2.0}}, type{1, {0, 2.1}}, tutor{bland{}});
    }

    {
      using type = std::pair<int, only_equivalence_checkable>;

      check(with_best_available, LINE(""), type{1, {1.5}}, type{0, {1.5}});
      check(with_best_available, LINE(""), type{1, {1.5}}, type{1, {1.4}});
      check(with_best_available, LINE(""), type{0, {1.5}}, type{1, {1.4}});
      check(with_best_available, LINE(""), type{1, {1.4}}, type{0, {1.4}}, tutor{bland{}});
    }
  }

  [[nodiscard]]
  std::string_view false_negative_diagnostics::source_file() const noexcept
  {
    return __FILE__;
  }

  void false_negative_diagnostics::run_tests()
  {
    basic_tests();
    test_mixed();
    test_function();
    test_equivalence_checks();
    test_weak_equivalence_checks();
    test_with_best_available_checks();
  }

  void false_negative_diagnostics::basic_tests()
  {
    check(LINE(""), true);

    check(equality, LINE(""), 5, 5);
    check(equality, LINE(""), 5.0, 5.0);
  }

  void false_negative_diagnostics::test_mixed()
  {
    using t_0 = std::vector<std::pair<int, float>>;
    using t_1 = std::set<double>;
    using t_2 = std::complex<double>;
    using type = std::tuple<t_0, t_1, t_2>;

    type a{t_0{{1, 2.1f}, {2, 2.8f}}, {3.3, -9.6, 3.2}, {1.1, 0.2}};
    type b{t_0{{1, 2.1f}, {2, 2.8f}}, {3.3, -9.6, 3.2}, {1.1, 0.2}};

    check(equality, LINE(""), a, b);

    check(equivalence, LINE(""), std::vector<std::string>{ {"a"}, {"b"}}, std::initializer_list<std::string_view>{"a", "b"});
  }

  void false_negative_diagnostics::test_function()
  {
    {
      using function = std::function<void()>;
      check(weak_equivalence, LINE("Both bound"), function{[]() {}}, function{[]() {}});
      check(weak_equivalence, LINE("Neither bound"), function{}, function{});
    }

    {
      using function = std::function<int()>;
      check(weak_equivalence, LINE("Both bound"), function{[]() { return 42; }}, function{[]() { return 42; }});
      check(weak_equivalence, LINE("Neither bound"), function{}, function{});
    }

    {
      using function = std::function<void(int)>;
      check(weak_equivalence, LINE("Both bound"), function{[](int) {}}, function{[](int) {}});
      check(weak_equivalence, LINE("Neither bound"), function{}, function{});
    }
  }

  void false_negative_diagnostics::test_equivalence_checks()
  {
    check(equivalence, LINE("Advice for equivalence checking"), foo{42}, 42, tutor{bland{}});

    check(equivalence, LINE("Advice for range equivalence, where the containerized for is explicitly specialized"), 
                      std::vector<foo>{{42}}, std::vector<int>{42}, tutor{bland{}});

    check(equivalence, LINE("Advice for range equivalence, where the containerized for is not explicitly specialized"),
      std::set<foo>{{42}}, std::set<int>{42}, tutor{bland{}});

    using namespace std::string_literals;
    using namespace std::string_view_literals;
    check(equivalence, LINE("string and string_view"), "foo"s, "foo"sv);
  }

  void false_negative_diagnostics::test_weak_equivalence_checks()
  {
    using beast = perfectly_normal_beast<int>;
    check(weak_equivalence, LINE(""), beast{1, 2}, std::initializer_list<int>{1, 2});

    using prediction = std::initializer_list<std::initializer_list<int>>;
    check(weak_equivalence, LINE(""), std::vector<beast>{{1, 2}, {3, 4}}, prediction{{1, 2}, {3, 4}});

    check(weak_equivalence, LINE("Advice for weak equivalence checking"), only_weakly_checkable{42, 3.14}, std::pair<int, double>{42, 3.14}, tutor{bland{}});

    check(weak_equivalence,
          LINE("Advice for range weak equivalence, where the containerized form is explicitly specialized"),
          std::vector<only_weakly_checkable>{{42, 3.14}},
          std::vector<std::pair<int, double>>{{42, 3.14}}, tutor{bland{}});

    check(weak_equivalence,
          LINE("Advice for range weak equivalence, where the containerized form is not explicitly specialized"),
          std::list<only_weakly_checkable>{{42, 3.14}},
          std::list<std::pair<int, double>>{{42, 3.14}}, tutor{bland{}});
  }

  void false_negative_diagnostics::test_with_best_available_checks()
  {
    {
      using type = std::pair<int, double>;

      check(with_best_available, LINE(""), type{1, 0.0}, type{1, 0.0});
    }

    {
      using type = std::pair<int, only_weakly_checkable>;

      check(with_best_available, LINE(""), type{1, {0, 2.0}}, type{1, {0, 2.0}});
    }

    {
      using type = std::pair<int, only_equivalence_checkable>;

      check(with_best_available, LINE(""), type{1, {1.4}}, type{1, {1.4}});
      check(with_best_available, LINE(""), type{1, {1.4}}, type{1, {1.4}}, tutor{bland{}});
    }
  }
}
