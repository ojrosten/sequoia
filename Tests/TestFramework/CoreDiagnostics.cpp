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
  namespace fs = std::filesystem;

  namespace
  {
    struct dummy_file_comparer
    {
      template<test_mode Mode>
      void operator()(test_logger<Mode>&, const std::filesystem::path&, const std::filesystem::path&) const
      {}
    };

    using bespoke_file_checker_t = general_file_checker<string_based_file_comparer, dummy_file_comparer>;

    inline const bespoke_file_checker_t bespoke_file_checker{{".*", ".ignore"}};

    inline const general_equivalence_check_t<bespoke_file_checker_t>      bespoke_path_equivalence{bespoke_file_checker};
    inline const general_weak_equivalence_check_t<bespoke_file_checker_t> bespoke_path_weak_equivalence{bespoke_file_checker};

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

  log_summary& postprocess(log_summary& summary, const std::filesystem::path& testRepo)
  {
    std::string updatedOutput{summary.diagnostics_output()};

    replace_all(updatedOutput, testRepo.parent_path().generic_string() + "/", "");

    summary.diagnostics_output(updatedOutput);

    return summary;
  }

  [[nodiscard]]
  std::string_view false_positive_diagnostics::source_file() const noexcept
  {
    return __FILE__;
  }

  void false_positive_diagnostics::run_tests()
  {
    basic_tests();
    test_exceptions();
    test_heterogeneous();
    test_variant();
    test_optional();
    test_container_checks();
    test_strings<std::string>();
    test_strings<std::string_view>();
    test_wstrings<std::wstring>();
    test_wstrings<std::wstring_view>();
    test_mixed();
    test_paths();
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

  void false_positive_diagnostics::test_exceptions()
  {
    check_exception_thrown<std::runtime_error>(LINE("Exception expected but nothing thrown"), [](){});
    check_exception_thrown<std::runtime_error>(LINE("Exception thrown but of wrong type"), [](){ throw std::logic_error("Error"); });
    check_exception_thrown<std::runtime_error>(LINE("Exception thrown but of unknown type"), [](){ throw 1; });
  }

  void false_positive_diagnostics::test_heterogeneous()
  {
    check(equality, LINE(""), std::pair<int, double>{5, 7.8}, std::pair<int, double>{5, -7.8});
    check(equality, LINE(""), std::pair<int, double>{5, 7.8}, std::pair<int, double>{-5, 7.8});
    check(equality, LINE(""), std::pair<int, double>{5, 7.8}, std::pair<int, double>{-5, 6.8}, tutor{bland{}});
    check(with_best_available, LINE(""), std::pair<int, double>{5, 7.8}, std::pair<int, double>{5, -7.8});
    check(equivalence, LINE(""), std::pair<const int&, double>{5, 7.8}, std::pair<int, const double&>{-5, 6.8});
    check(weak_equivalence, LINE(""), std::pair<const int&, double>{5, 7.8}, std::pair<int, const double&>{-5, 6.8});

    check(equality, LINE(""), std::tuple<int, double, float>{4, 3.4, -9.2f}, std::tuple<int, double, float>{0, 3.4, -9.2f});
    check(equality, LINE(""), std::tuple<int, double, float>{4, 3.4, -9.2f}, std::tuple<int, double, float>{4, 0.0, -9.2f}, tutor{bland{}});
    check(equality, LINE(""), std::tuple<int, double, float>{4, 3.4, -9.2f}, std::tuple<int, double, float>{4, 3.4, -0.0f});
    check(equivalence, LINE(""), std::tuple<const int&, double>{5, 7.8}, std::tuple<int, const double&>{-5, 6.8});
  }

  void false_positive_diagnostics::test_variant()
  {
    using var = std::variant<int, double>;

    check(equality, LINE(""), var{0}, var{0.0});
    check(equality, LINE(""), var{1}, var{2});
    check(equality, LINE(""), var{-0.1}, var{0.0});
  }

  void false_positive_diagnostics::test_optional()
  {
    using opt = std::optional<int>;
    check(equality, LINE(""), opt{}, opt{0});
    check(equality, LINE(""), opt{0}, opt{});
    check(equality, LINE(""), opt{2}, opt{0});
  }

  void false_positive_diagnostics::test_container_checks()
  {
    check(equality, LINE("Empty vector check which should fail"), std::vector<double>{}, std::vector<double>{1});
    check(equality, LINE("One element vector check which should fail due to wrong value"), std::vector<double>{1}, std::vector<double>{2},
                   tutor{[](double, double) { return "Vector element advice"; }});
    check(equality, LINE("One element vector check which should fail due to differing sizes"), std::vector<double>{1}, std::vector<double>{1,2});
    check(equality, LINE("Multi-element vector comparison which should fail due to last element"), std::vector<double>{1,5}, std::vector<double>{1,4});
    check(equality, LINE("Multi-element vector comparison which should fail due to first element"), std::vector<double>{1,5}, std::vector<double>{0,5});
    check(equality, LINE("Multi-element vector comparison which should fail due to middle element"), std::vector<double>{1,3.2,5}, std::vector<double>{1,3.3,5});
    check(equality, LINE("Multi-element vector comparison which should fail due to different sizes"), std::vector<double>{1,5,3.2}, std::vector<double>{5,3.2});

    std::vector<float> refs{-4.3f, 2.8f, 6.2f, 7.3f}, ans{1.1f, -4.3f, 2.8f, 6.2f, 8.4f, 7.3f};

    check(equality, LINE("Iterators demarcate differing numbers of elements"), refs.cbegin(), refs.cend(), ans.cbegin(), ans.cend());
    check(equality, LINE("Iterators demarcate differing elements"), refs.cbegin(), refs.cend(), ans.cbegin(), ans.cbegin() + 4);
  }

  template<class String>
  void false_positive_diagnostics::test_strings()
  {
    check(equality, LINE("Empty and non-empty strings"), String{""}, String{"Hello, World!"});
    check(equality, LINE("Empty and non-empty strings"), String{"Hello, World!"}, String{""});
    check(equality, LINE("Strings of differing length"), String{"what?!"}, String{"Hello, World!"});
    check(equality, LINE("Differing strings of same length"), String{"Hello, world?"}, String{"Hello, World!"});
    check(equality, LINE("Advice"), String{"Foo"}, String{"Bar"}, tutor{[](const String&, const String&) { return "Foo, my old nemesis"; }});

    const String longMessage{"This is a message which is sufficiently long for only a segment to be included when a string diff is performed"};
    const String longMessageWithDiffNearMiddle{"This is a message which is sufficiently long for only a segment to be incluxed when a string diff is performed"};
    const String longMessageWithDiffNearEnd{"This is a message which is sufficiently long for only a segment to be included when a string diff isxperformed"};

    check(equality, LINE("Empty string compared with long string"), String{""}, longMessage);
    check(equality, LINE("Long string with empty string"), longMessage, String{""});

    check(equality, LINE("Short string compared with long string"), String{"This is a mess"}, longMessage);
    check(equality, LINE("Long string with short string"), longMessage, String{"This is a mess"});

    check(equality, LINE("Strings differing by a newline"), String{"Hello\nWorld"}, String{"Hello, World"});
    check(equality, LINE("Strings differing by a newline"), String{"Hello, World"}, String{"Hello\nWorld"});
    check(equality, LINE("Strings differing by a newline at the start"), String{"\nHello, World"}, String{"Hello, World"});
    check(equality, LINE("Strings differing by a newline at the start"), String{"Hello, World"}, String{"\nHello, World"});
    check(equality, LINE("Empty string compared with newline"), String{""}, String{"\n"});
    check(equality, LINE("Empty string compared with newline"), String{"\n"}, String{""});
    check(equality, LINE("Strings differing from newline onwards"), String{"Hello, World"}, String{"Hello\n"});
    check(equality, LINE("Strings differing from newline onwards"), String{"Hello\n"}, String{"Hello, World"});
    check(equality, LINE("Strings differing from newline onwards"), String{"Hello, World"}, String{"Hello\nPeople"});
    check(equality, LINE("Strings differing from newline onwards"), String{"Hello\nPeople"}, String{"Hello, World"});
    check(equality, LINE("Output suppressed by a new line"), String{"Hello  World\nAnd so forth"}, String{"Hello, World\nAnd so forth"});
    check(equality, LINE("Difference on the second line"), String{"Hello, World\nAnd so furth"}, String{"Hello, World\nAnd so forth"});
    check(equality, LINE("Missing line"), String{"Hello, World\nAnd so forth"}, String{"Hello, World\n\nAnd so forth"});
    check(equality, LINE("Extra line"), String{"Hello, World\n\nAnd so forth"}, String{"Hello, World\nAnd so forth"});

    check(equality, LINE("Long strings compared with difference near middle"), longMessageWithDiffNearMiddle, longMessage);
    check(equality, LINE("Long strings compared with difference near middle"), longMessage, longMessageWithDiffNearMiddle);

    check(equality, LINE("Long strings compared with difference near end"), longMessageWithDiffNearEnd, longMessage);
    check(equality, LINE("Long strings compared with difference near end"), longMessage, longMessageWithDiffNearEnd);

    check(equivalence, LINE(""), String{"foo"}, "fo");
    check(equivalence, LINE(""), String{"foo"}, "fob", tutor{[](char, char) {
        return "Sort your chars out!";
      }});
  }

  template<class String>
  void false_positive_diagnostics::test_wstrings()
  {
    check(equality, LINE("Differing strings of same length"), String{L"Hello, world?"}, String{L"Hello, World!"});
    check(equality, LINE("Advice"), String{L"Foo"}, String{L"Bar"}, tutor{[](const String&, const String&) { return "Foo, my old nemesis"; }});
    check(equality, LINE("Missing line"), String{L"Hello, World\nAnd so forth"}, String{L"Hello, World\n\nAnd so forth"});
    check(equality, LINE("Extra line"), String{L"Hello, World\n\nAnd so forth"}, String{L"Hello, World\nAnd so forth"});
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

  void false_positive_diagnostics::test_paths()
  {
    check(equivalence,
          LINE("Inequivalence of two different paths, neither of which exists"),
          fs::path{working_materials()}.append("Stuff/Blah"),
          fs::path{working_materials()}.append("Stuff/Blurg"));

    check(equivalence,
          LINE("Inequivalence of two different paths, one of which exists"),
          fs::path{working_materials()}.append("Stuff/Blah"),
          fs::path{working_materials()}.append("Stuff/A"));

    check(equivalence,
          LINE("Inequivalence of directory/file"),
          fs::path{working_materials()}.append("Stuff/A"),
          fs::path{working_materials()}.append("Stuff/A/foo.txt"));

    check(equivalence,
          LINE("Inequivalence of differently named files"),
          fs::path{working_materials()}.append("Stuff/B/foo.txt"),
          fs::path{working_materials()}.append("Stuff/B/bar.txt"));

    check(equivalence,
          LINE("Inequivalence of file contents"),
          fs::path{working_materials()}.append("Stuff/A/foo.txt"),
          fs::path{working_materials()}.append("Stuff/B/foo.txt"));

    check(equivalence,
          LINE("Inequivalence of differently named directories with the same contents"),
          fs::path{working_materials()}.append("Stuff/A"),
          fs::path{working_materials()}.append("Stuff/C"));

    check(equivalence,
          LINE("Inequivalence of directories with the same files but different contents"),
          fs::path{working_materials()}.append("Stuff/A"),
          fs::path{working_materials()}.append("MoreStuff/A"));

    check(equivalence,
          LINE("Inequivalence of directories with some common files"),
          fs::path{working_materials()}.append("Stuff/B"),
          fs::path{working_materials()}.append("MoreStuff/B"));

    check(equivalence,
          LINE("Inequivalence of directories with some common files"),
          fs::path{working_materials()}.append("MoreStuff/B"),
          fs::path{working_materials()}.append("Stuff/B"));

    check(equivalence,
          LINE("File inequivalence when default file checking is used"),
          fs::path{working_materials()}.append("CustomComparison/A/DifferingContent.ignore"),
          fs::path{working_materials()}.append("CustomComparison/B/DifferingContent.ignore"));

    check(equivalence,
          LINE("Range inequivalence when default file checking us used"),
          std::vector<fs::path>{fs::path{working_materials()}.append("CustomComparison/A/DifferingContent.ignore")},
          std::vector<fs::path>{fs::path{working_materials()}.append("CustomComparison/B/DifferingContent.ignore")});
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

    check(equivalence, LINE("Advice for range equivalence, where the containerized form is explicitly specialized"),
                      std::vector<foo>{{42}}, std::vector<int>{41}, tutor{bland{}});

    check(equivalence, LINE("Advice for range equivalence, where the containerized form is not explicitly specialized"),
                      std::set<foo>{{42}}, std::set<int>{41}, tutor{bland{}});
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
          LINE("Weak inequivalence of directories with some common files"),
          fs::path{working_materials()}.append("MoreStuff/B"),
          fs::path{working_materials()}.append("Stuff/B"));

    check(weak_equivalence,
          LINE("Directory weak inequivalence when default file checking is used"),
          fs::path{working_materials()}.append("CustomComparison/A"),
          fs::path{working_materials()}.append("CustomComparison/B"));

    check(weak_equivalence,
          LINE("Weak inequivalence of range when default file checking is used"),
          std::vector<fs::path>{{fs::path{working_materials()}.append("CustomComparison/A")}},
          std::vector<fs::path>{{fs::path{working_materials()}.append("CustomComparison/B")}});

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
    test_exceptions();
    test_heterogeneous();
    test_variant();
    test_optional();
    test_container_checks();
    test_strings();
    test_mixed();
    test_paths();
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

  void false_negative_diagnostics::test_exceptions()
  {
    check_exception_thrown<std::runtime_error>(LINE(""), [](){ throw std::runtime_error("Error");});

    check_exception_thrown<int>(LINE(""), [](){ throw 1; });
  }

  void false_negative_diagnostics::test_heterogeneous()
  {
    check(equality, LINE(""), std::pair<int, double>{5, 7.8}, std::pair<int, double>{5, 7.8});

    check(equality, LINE(""), std::tuple<int, double, float>{4, 3.4, -9.2f}, std::tuple<int, double, float>{4, 3.4, -9.2f});
  }

  void false_negative_diagnostics::test_variant()
  {
    using var = std::variant<int, double>;

    check(equality, LINE(""), var{0}, var{0});
    check(equality, LINE(""), var{0.0}, var{0.0});
    check(equality, LINE(""), var{3}, var{3});
    check(equality, LINE(""), var{4.0}, var{4.0});
  }

  void false_negative_diagnostics::test_optional()
  {
    using opt = std::optional<int>;
    check(equality, LINE(""), opt{}, opt{});
    check(equality, LINE(""), opt{0}, opt{0});
    check(equality, LINE(""), opt{-1}, opt{-1});
  }

  void false_negative_diagnostics::test_container_checks()
  {
    check(equality, LINE("Empty vector check which should pass"), std::vector<double>{}, std::vector<double>{});
    check(equality, LINE("One element vector check which should pass"), std::vector<double>{2}, std::vector<double>{2});
    check(equality, LINE("Multi-element vector comparison which should pass"), std::vector<double>{1,5}, std::vector<double>{1,5});

    std::vector<float> refs{-4.3f, 2.8f, 6.2f, 7.3f}, ans{1.1f, -4.3f, 2.8f, 6.2f, 8.4f, 7.3f};

    check(equality, LINE("Iterators demarcate identical elements"), refs.cbegin(), refs.cbegin()+3, ans.cbegin()+1, ans.cbegin()+4);
  }


  void false_negative_diagnostics::test_strings()
  {
    check(equality, LINE("Differing strings"), std::string{"Hello, World!"}, std::string{"Hello, World!"});
    check(equivalence, LINE(""), std::string{"foo"}, "foo");
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

  void false_negative_diagnostics::test_paths()
  {
    check(equivalence,
          LINE("Equivalence of a file to itself"),
          fs::path{working_materials()}.append("Stuff/A/foo.txt"),
          fs::path{working_materials()}.append("Stuff/A/foo.txt"));

    check(equivalence,
          LINE("Equivalence of a directory to itself"),
          fs::path{working_materials()}.append("Stuff/A"),
          fs::path{working_materials()}.append("Stuff/A"));

    check(equivalence,
          LINE("Equivalence of a directory, with sub-directories to itself"),
          fs::path{working_materials()}.append("Stuff"),
          fs::path{working_materials()}.append("Stuff"));

    check(equivalence,
          LINE("Equivalence of identical directories in different locations"),
          fs::path{working_materials()}.append("Stuff/C"),
          fs::path{working_materials()}.append("SameStuff/C"));

    check(bespoke_path_equivalence,
          LINE("File equivalence when .ignore is ignored"),
          fs::path{working_materials()}.append("CustomComparison/A/DifferingContent.ignore"),
          fs::path{working_materials()}.append("CustomComparison/B/DifferingContent.ignore"));

    check(bespoke_path_equivalence,
          LINE("Range equivalence when .ignore is ignored"),
          std::vector<fs::path>{fs::path{working_materials()}.append("CustomComparison/A/DifferingContent.ignore")},
          std::vector<fs::path>{fs::path{working_materials()}.append("CustomComparison/B/DifferingContent.ignore")});
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

    check(weak_equivalence,
         LINE("Weak equivalence of directories in with the same contents but different names"),
         fs::path{working_materials()}.append("Stuff"),
         fs::path{working_materials()}.append("SameStuff"));

    check(bespoke_path_weak_equivalence,
          LINE("Weak equivalence when .ignore is ignored"),
          fs::path{working_materials()}.append("CustomComparison/A"),
          fs::path{working_materials()}.append("CustomComparison/B"));

    check(bespoke_path_weak_equivalence,
          LINE("Weak equivalence of range when .ignore is ignored"),
          std::vector<fs::path>{{fs::path{working_materials()}.append("CustomComparison/A")}},
          std::vector<fs::path>{{fs::path{working_materials()}.append("CustomComparison/B")}});

    check(weak_equivalence, LINE("Advice for weak equivalence checking"), only_weakly_checkable{42, 3.14}, std::pair<int, double>{42, 3.14}, tutor{bland{}});

    check(weak_equivalence,
          LINE("Advice for range weak equivalence, where the containerized form is explicitly specialized"),
          std::vector<only_weakly_checkable>{{42, 3.14}}, std::vector<std::pair<int, double>>{{42, 3.14}}, tutor{bland{}});

    check(weak_equivalence,
          LINE("Advice for range weak equivalence, where the containerized form is not explicitly specialized"),
          std::list<only_weakly_checkable>{{42, 3.14}}, std::list<std::pair<int, double>>{{42, 3.14}}, tutor{bland{}});
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
