////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "ContainerFreeDiagnostics.hpp"
#include "ElementaryFreeDiagnosticsUtilities.hpp"
#include "RegularTestDiagnosticsUtilities.hpp"

#include "sequoia/TestFramework/ConcreteTypeCheckers.hpp"

#include <complex>
#include <list>
#include <set>

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
    template<test_mode Mode, class Advisor>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const foo& f, int i, const tutor<Advisor>& advisor)
    {
      check(equality, "Wrapped value", logger, f.i, i, advisor);
    }

    template<test_mode Mode, class Advisor>
    static void test(weak_equivalence_check_t, test_logger<Mode>& logger, const foo& f, double d, const tutor<Advisor>& advisor)
    {
      check(equality, "Wrapped value", logger, f.i, static_cast<int>(d), advisor);
    }
  };

  // Explicit container specialization to test propagation of tutor
  template<>
  struct value_tester<std::vector<foo>>
  {
    template<test_mode Mode>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const std::vector<foo>& obtained, const std::vector<int>& prediction, tutor<bland> advisor)
    {
      check(equivalence, "Vector equivalence", logger, obtained.begin(), obtained.end(), prediction.begin(), prediction.end(), advisor);
    }

    template<test_mode Mode>
    static void test(weak_equivalence_check_t, test_logger<Mode>& logger, const std::vector<foo>& obtained, const std::list<int>& prediction, tutor<bland> advisor)
    {
      check(equivalence, "Vector equivalence", logger, obtained.begin(), obtained.end(), prediction.begin(), prediction.end(), advisor);
    }
  };

  [[nodiscard]]
  std::filesystem::path container_false_negative_free_diagnostics::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void container_false_negative_free_diagnostics::run_tests()
  {
    test_homogeneous();
    test_heterogeneous();
    test_mixed();
  }

  void container_false_negative_free_diagnostics::test_homogeneous()
  {
    check(equality, "Empty vector check which should fail", std::vector<double>{}, std::vector<double>{1});
    check(equality, "One element vector check which should fail due to wrong value", std::vector<double>{1}, std::vector<double>{2});
    check(equality,
         reporter{"Advice for one element vector check which should fail due to wrong value"},
         std::vector<double>{1},
         std::vector<double>{2},
         tutor{[](double, double) { return "Vector element advice"; }});
    check(equality, "One element vector check which should fail due to differing sizes", std::vector<double>{1}, std::vector<double>{1,2});
    check(equality, "Multi-element vector comparison which should fail due to last element", std::vector<double>{1,5}, std::vector<double>{1,4});
    check(equality, "Multi-element vector comparison which should fail due to first element", std::vector<double>{1,5}, std::vector<double>{0,5});
    check(equality, "Multi-element vector comparison which should fail due to middle element", std::vector<double>{1,3.2,5}, std::vector<double>{1,3.3,5});
    check(equality, "Multi-element vector comparison which should fail due to different sizes", std::vector<double>{1,5,3.2}, std::vector<double>{5,3.2});

    std::vector<float> refs{-4.3f, 2.8f, 6.2f, 7.3f}, ans{1.1f, -4.3f, 2.8f, 6.2f, 8.4f, 7.3f};

    check(equality, "Iterators demarcate differing numbers of elements", refs.cbegin(), refs.cend(), ans.cbegin(), ans.cend());
    check(equality, "Iterators demarcate differing elements", refs.cbegin(), refs.cend(), ans.cbegin(), ans.cbegin() + 4);

    check(equality, "Spans over vectors of differing length", std::span<const int>{std::vector<int>{}}, std::span<const int>{std::vector<int>{1}});
    check(equality, "Spans over vectors of differing elements", std::span<const int>{std::vector<int>{42}}, std::span<const int>{std::vector<int>{1}});

    check(equivalence,
          reporter{"Range equivalence, where the containerized form is explicitly specialized"},
          std::vector<foo>{{42}},
          std::vector<int>{41});

    check(equivalence,
          reporter{"Advice for range equivalence, where the containerized form is explicitly specialized"},
          std::vector<foo>{{42}},
          std::vector<int>{41},
          tutor{bland{}});

    check(equivalence,
          reporter{"Range equivalence, where the containerized form is not explicitly specialized"},
          std::set<foo>{{42}},
          std::set<int>{41});

    check(equivalence,
          reporter{"Advice for range equivalence, where the containerized form is not explicitly specialized"},
          std::set<foo>{{42}},
          std::set<int>{41},
          tutor{bland{}});

    check(weak_equivalence,
          reporter{"Range weak equivalence, where the containerized form is explicitly specialized"},
          std::vector<foo>{{42}},
          std::list<int>{41});

    check(weak_equivalence,
          reporter{"Advice for range weak equivalence, where the containerized form is explicitly specialized"},
          std::vector<foo>{{42}},
          std::list<int>{41},
          tutor{bland{}});

    check(weak_equivalence,
          reporter{"Range weak equivalence, where the containerized form is not explicitly specialized"},
          std::list<foo>{{42}},
          std::array<double, 1>{41});

    check(weak_equivalence,
          reporter{"Advice for range weak equivalence, where the containerized form is not explicitly specialized"},
          std::list<foo>{ {42}},
          std::array<double, 1>{41},
          tutor{bland{}});


    using beast = perfectly_normal_beast<int>;
    check(weak_equivalence, "", beast{1, 2}, std::initializer_list<int>{1, 1});
    check(weak_equivalence, "", beast{1, 2}, std::initializer_list<int>{1, 1}, tutor{[](int, int) {
        return "Don't mess with the beast.";
      }});

    using prediction = std::initializer_list<std::initializer_list<int>>;
    check(weak_equivalence, "", std::vector<beast>{ {1, 2}, {3, 4}}, prediction{{1, 2}, {3, 5}});

    check(weak_equivalence,
          reporter{""},
          std::vector<beast>{ {1, 2}, {3, 4}},
          prediction{{1, 2}, {3, 5}},
          tutor{[](int, int) {
            return "Or at least don't mess with a vector of beasts.";
          }});
  }

  void container_false_negative_free_diagnostics::test_heterogeneous()
  {
    {
      using type = std::pair<int, double>;

      check(equality, "Pair for which the first element differs", type{5, 7.8}, type{-5, 7.8});
      check(equality, "Pair for which the second element differs", type{5, 7.8}, type{5, -7.8});
      check(equality, "Pair for which both elements differs", type{5, 7.8}, type{-5, 6.8});
      check(equality, "Pair for which both elements differs with advice", type{5, 7.8}, type{-5, 6.8}, tutor{bland{}});

      check(equivalence,
            reporter{"Pair for which both elements differs via fallback"},
            type{5, 7.8},
            type{-5, 6.8});
      check(equivalence,
           reporter{"Pair for which both elements differs via fallback with advice"},
           type{5, 7.8},
           type{-5, 6.8},
           tutor{bland{}});

      check(weak_equivalence,
            reporter{"Pair for which both elements differs via two fallbacks"},
            type{5, 7.8},
            type{-5, 6.8});
      check(weak_equivalence,
            reporter{"Pair for which both elements differs via two fallbacks with advice"},
            type{5, 7.8},
            type{-5, 6.8},
            tutor{bland{}});

      check(with_best_available, "Best available", type{5, 7.8}, type{5, -7.8});
      check(with_best_available, "Best available with advice", type{5, 7.8}, type{5, -7.8}, tutor{bland{}});
    }

    {
      using type0 = std::pair<const int&, double>;
      using type1 = std::pair<int, const double&>;

      check(equivalence, "", type0{5, 7.8}, type1{-5, 6.8});
      check(weak_equivalence, "", type0{5, 7.8}, type1{-5, 6.8});
    }

    {
      using type = std::tuple<int, double, float>;

      check(equality, "", type{4, 3.4, -9.2f}, type{0, 3.4, -9.2f});
      check(equality, "", type{4, 3.4, -9.2f}, type{4, 0.0, -9.2f}, tutor{bland{}});
      check(equality, "", type{4, 3.4, -9.2f}, type{4, 3.4, -0.0f});
    }

    {
      using type = std::pair<int, only_weakly_checkable>;

      check(with_best_available, "", type{1, {0, 2.0}}, type{0, {0, 2.0}});
      check(with_best_available, "", type{1, {0, 2.0}}, type{1, {0, 2.1}});
      check(with_best_available, "", type{1, {0, 2.0}}, type{0, {0, 2.1}});
      check(with_best_available, "", type{1, {0, 2.0}}, type{1, {0, 2.1}}, tutor{bland{}});
    }

    {
      using type = std::pair<int, only_equivalence_checkable>;

      check(with_best_available, "", type{1, {1.5}}, type{0, {1.5}});
      check(with_best_available, "", type{1, {1.5}}, type{1, {1.4}});
      check(with_best_available, "", type{0, {1.5}}, type{1, {1.4}});
      check(with_best_available, "", type{1, {1.4}}, type{0, {1.4}}, tutor{bland{}});
    }
  }

  void container_false_negative_free_diagnostics::test_mixed()
  {
    using t_0 = std::vector<std::pair<int, float>>;
    using t_1 = std::set<double>;
    using t_2 = std::complex<double>;
    using type = std::tuple<t_0, t_1, t_2>;

    type a{t_0{{1, 2.1f}, {2, 2.8f}}, {3.3, -9.6, 3.2}, {1.1, 0.2}};

    {
      type b{t_0{{2, 2.1f}, {2, 2.8f}}, {3.3, -9.6, 3.2}, {1.1, 0.2}};
      check(equality, "", a, b, tutor{[](int, int){ return "Nested int advice"; }});
    }

    {
      type b{t_0{{1, 2.1f}, {2, 2.8f}}, {3.4, -9.6, 3.2}, {1.1, 0.2}};
      check(equality, "", a, b, tutor{[](const std::set<double>&, const std::set<double>&){
                                       return "Note reordering of elements upon set construction";
                                     }});
    }

    check(equivalence, "", std::vector<std::string>{ {"a"}, {"b"}}, std::initializer_list<std::string_view>{"a", "c"});

    check(equivalence, "", std::vector<std::string>{ {"a"}, {"b"}}, std::initializer_list<std::string_view>{"a", "c"}, tutor{[](char, char) {
        return "Ah, chars. So easy to get wrong.";
    }});
  }

  [[nodiscard]]
  std::filesystem::path container_false_positive_free_diagnostics::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void container_false_positive_free_diagnostics::run_tests()
  {
    test_homogeneous();
    test_heterogeneous();
    test_mixed();
  }

  void container_false_positive_free_diagnostics::test_homogeneous()
  {
    check(equality, "Empty vector check which should pass", std::vector<double>{}, std::vector<double>{});
    check(equality, "One element vector check which should pass", std::vector<double>{2}, std::vector<double>{2});
    check(equality, "Multi-element vector comparison which should pass", std::vector<double>{1, 5}, std::vector<double>{1, 5});

    std::vector<float> refs{-4.3f, 2.8f, 6.2f, 7.3f}, ans{1.1f, -4.3f, 2.8f, 6.2f, 8.4f, 7.3f};

    check(equality, "Iterators demarcate identical elements", refs.cbegin(), refs.cbegin() + 3, ans.cbegin() + 1, ans.cbegin() + 4);

    check(equivalence,
          reporter{"Advice for range equivalence, where the containerized form is explicitly specialized"},
          std::vector<foo>{ {42}},
          std::vector<int>{42},
          tutor{bland{}});

    check(equivalence,
          reporter{"Advice for range equivalence, where the containerized form is not explicitly specialized"},
          std::set<foo>{{42}},
          std::set<int>{42},
          tutor{bland{}});

    using beast = perfectly_normal_beast<int>;
    check(weak_equivalence, "", beast{1, 2}, std::initializer_list<int>{1, 2});

    using prediction = std::initializer_list<std::initializer_list<int>>;
    check(weak_equivalence, "", std::vector<beast>{ {1, 2}, {3, 4}}, prediction{{1, 2}, {3, 4}});

    check(weak_equivalence, "Advice for weak equivalence checking", only_weakly_checkable{42, 3.14}, std::pair<int, double>{42, 3.14}, tutor{bland{}});
  }

  void container_false_positive_free_diagnostics::test_heterogeneous()
  {
    check(equality, "", std::pair<int, double>{5, 7.8}, std::pair<int, double>{5, 7.8});

    check(equality, "", std::tuple<int, double, float>{4, 3.4, -9.2f}, std::tuple<int, double, float>{4, 3.4, -9.2f});

    {
      using type = std::pair<int, double>;

      check(with_best_available, "", type{1, 0.0}, type{1, 0.0});
    }

    {
      using type = std::pair<int, only_weakly_checkable>;

      check(with_best_available, "", type{1, {0, 2.0}}, type{1, {0, 2.0}});
    }

    {
      using type = std::pair<int, only_equivalence_checkable>;

      check(with_best_available, "", type{1, {1.4}}, type{1, {1.4}});
      check(with_best_available, "", type{1, {1.4}}, type{1, {1.4}}, tutor{bland{}});
    }
  }

  void container_false_positive_free_diagnostics::test_mixed()
  {
    using t_0 = std::vector<std::pair<int, float>>;
    using t_1 = std::set<double>;
    using t_2 = std::complex<double>;
    using type = std::tuple<t_0, t_1, t_2>;

    type a{t_0{{1, 2.1f}, {2, 2.8f}}, {3.3, -9.6, 3.2}, {1.1, 0.2}};
    type b{t_0{{1, 2.1f}, {2, 2.8f}}, {3.3, -9.6, 3.2}, {1.1, 0.2}};

    check(equality, "", a, b);

    check(equivalence, "", std::vector<std::string>{ {"a"}, {"b"}}, std::initializer_list<std::string_view>{"a", "b"});
  }
}
