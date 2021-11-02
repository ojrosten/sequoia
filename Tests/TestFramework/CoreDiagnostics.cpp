////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "CoreDiagnostics.hpp"
#include "CoreDiagnosticsUtilities.hpp"
#include "sequoia/TextProcessing/Substitutions.hpp"

#include <complex>
#include <vector>
#include <set>
#include <filesystem>
#include <variant>
#include <optional>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    struct bland
    {
      [[nodiscard]]
      std::string operator()(int, int) const
      {
        return {"Integer advice"};
      }

      [[nodiscard]]
      std::string operator()(double, double) const
      {
        return {"Double advice"};
      }
    };

    struct dummy_file_checker
    {
      template<test_mode Mode>
      void operator()(test_logger<Mode>&, const std::filesystem::path&, const std::filesystem::path&) const
      {}
    };

    struct bespoke_file_checker
    {
      template<test_mode Mode>
      static void check_file(test_logger<Mode>& logger, const std::filesystem::path& file, const std::filesystem::path& prediction)
      {
        const auto factory{runtime::factory<default_file_checker, dummy_file_checker>{{"default", ".ignore"}}};
        const auto checker{factory.create_or<default_file_checker>(file.extension().string())};
        std::visit([&logger, &file, &prediction](auto&& fn){ fn(logger, file, prediction); }, checker);
      }
    };

    struct foo
    {
      int x;
    };
  }


  template<>
  struct equivalence_checker<foo>
  {
    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const foo& f, int i, tutor<bland> advisor)
    {
      check_equality("Wrapped value", logger, f.x, i, advisor);
    }
  };

  // Explicit container specialization to testing propagation of tutor through check_range_equivalence
  template<>
  struct equivalence_checker<std::vector<foo>>
  {
    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const std::vector<foo>& f, const std::vector<int>& i, tutor<bland> advisor)
    {
      check_range_equivalence("Vector equivalence", logger, f.begin(), f.end(), i.begin(), i.end(), advisor);
    }
  };

  log_summary& postprocess(log_summary& summary, const std::filesystem::path& testRepo)
  {
    std::string updatedOutput{summary.diagnostics_output()};

    replace_all(updatedOutput, testRepo.parent_path().generic_string() + "/", "");

    summary.diagnostics_output(updatedOutput);

    return summary;
  }

  template<class T>
  struct weak_equivalence_checker<perfectly_normal_beast<T>>
  {
    template<class Logger>
    static void check(Logger& logger, const perfectly_normal_beast<T>& beast, std::initializer_list<T> prediction)
    {
      check_range("", logger, std::begin(beast.x), std::end(beast.x), std::begin(prediction), std::end(prediction));
    }

    template<class Logger, class Advisor>
    static void check(Logger& logger, const perfectly_normal_beast<T>& beast, std::initializer_list<T> prediction, tutor<Advisor> advisor)
    {
      check_range("", logger, std::begin(beast.x), std::end(beast.x), std::begin(prediction), std::end(prediction), std::move(advisor));
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
    test_exceptions();
    test_heterogeneous();
    test_variant();
    test_optional();
    test_container_checks();
    test_strings();
    test_mixed();
    test_regular_semantics();
    test_equivalence_checks();
    test_weak_equivalence_checks();
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

    check_equality(LINE("Integers which should compare different"), 5, 4);
    check_equality(LINE(""), 6.5, 5.6, tutor{[](double, double){
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
    check_equality(LINE(""), std::pair<int, double>{5, 7.8}, std::pair<int, double>{5, -7.8});
    check_equality(LINE(""), std::pair<int, double>{5, 7.8}, std::pair<int, double>{-5, 7.8});
    check_equality(LINE(""), std::pair<int, double>{5, 7.8}, std::pair<int, double>{-5, 6.8}, tutor{bland{}});

    check_equality(LINE(""), std::tuple<int, double, float>{4, 3.4, -9.2f}, std::tuple<int, double, float>{0, 3.4, -9.2f});
    check_equality(LINE(""), std::tuple<int, double, float>{4, 3.4, -9.2f}, std::tuple<int, double, float>{4, 0.0, -9.2f}, tutor{bland{}});
    check_equality(LINE(""), std::tuple<int, double, float>{4, 3.4, -9.2f}, std::tuple<int, double, float>{4, 3.4, -0.0f});
  }

  void false_positive_diagnostics::test_variant()
  {
    using var = std::variant<int, double>;

    check_equality(LINE(""), var{0}, var{0.0});
    check_equality(LINE(""), var{1}, var{2});
    check_equality(LINE(""), var{-0.1}, var{0.0});
  }

  void false_positive_diagnostics::test_optional()
  {
    using opt = std::optional<int>;
    check_equality(LINE(""), opt{}, opt{0});
    check_equality(LINE(""), opt{0}, opt{});
    check_equality(LINE(""), opt{2}, opt{0});
  }

  void false_positive_diagnostics::test_container_checks()
  {
    check_equality(LINE("Empty vector check which should fail"), std::vector<double>{}, std::vector<double>{1});
    check_equality(LINE("One element vector check which should fail due to wrong value"), std::vector<double>{1}, std::vector<double>{2},
                   tutor{[](double, double) { return "Vector element advice"; }});
    check_equality(LINE("One element vector check which should fail due to differing sizes"), std::vector<double>{1}, std::vector<double>{1,2});
    check_equality(LINE("Multi-element vector comparison which should fail due to last element"), std::vector<double>{1,5}, std::vector<double>{1,4});
    check_equality(LINE("Multi-element vector comparison which should fail due to first element"), std::vector<double>{1,5}, std::vector<double>{0,5});
    check_equality(LINE("Multi-element vector comparison which should fail due to middle element"), std::vector<double>{1,3.2,5}, std::vector<double>{1,3.3,5});
    check_equality(LINE("Multi-element vector comparison which should fail due to different sizes"), std::vector<double>{1,5,3.2}, std::vector<double>{5,3.2});

    std::vector<float> refs{-4.3f, 2.8f, 6.2f, 7.3f}, ans{1.1f, -4.3f, 2.8f, 6.2f, 8.4f, 7.3f};

    check_range(LINE("Iterators demarcate differing numbers of elements"), refs.cbegin(), refs.cend(), ans.cbegin(), ans.cend());
    check_range(LINE("Iterators demarcate differing elements"), refs.cbegin(), refs.cend(), ans.cbegin(), ans.cbegin() + 4);
  }

  void false_positive_diagnostics::test_strings()
  {
    using namespace std::string_literals;
    check_equality(LINE("Empty and non-empty strings"), ""s, "Hello, World!"s);
    check_equality(LINE("Empty and non-empty strings"), "Hello, World!"s, ""s);
    check_equality(LINE("Strings of differing length"), "what?!"s, "Hello, World!"s);
    check_equality(LINE("Differing strings of same length"), "Hello, world?"s, "Hello, World!"s);

    const std::string longMessage{"This is a message which is sufficiently long for only a segment to be included when a string diff is performed"};

    check_equality(LINE("Empty string compared with long string"), ""s, longMessage);
    check_equality(LINE("Long string with empty string"), longMessage, ""s);

    check_equality(LINE("Short string compared with long string"), "This is a mess"s, longMessage);
    check_equality(LINE("Long string with short string"), longMessage, "This is a mess"s);

    check_equality(LINE("Strings differing by a newline"), "Hello\nWorld"s, "Hello, World"s);
    check_equality(LINE("Strings differing by a newline"), "Hello, World"s, "Hello\nWorld"s);
    check_equality(LINE("Strings differing by a newline at the start"), "\nHello, World"s, "Hello, World"s);
    check_equality(LINE("Strings differing by a newline at the start"), "Hello, World"s, "\nHello, World"s);
    check_equality(LINE("Empty string compared with newline"), ""s, "\n"s);
    check_equality(LINE("Empty string compared with newline"), "\n"s, ""s);
    check_equality(LINE("Strings differing from newline onwards"), "Hello, World"s, "Hello\n"s);
    check_equality(LINE("Strings differing from newline onwards"), "Hello\n"s, "Hello, World"s);
    check_equality(LINE("Strings differing from newline onwards"), "Hello, World"s, "Hello\nPeople"s);
    check_equality(LINE("Strings differing from newline onwards"), "Hello\nPeople"s, "Hello, World"s);
    check_equality(LINE("Output suppressed by a new line"), "Hello  World\nAnd so forth"s, "Hello, World\nAnd so forth"s);

    std::string corruptedMessage{longMessage};
    corruptedMessage[75] = 'x';

    check_equality(LINE("Long strings compared with difference near middle"), corruptedMessage, longMessage);
    check_equality(LINE("Long strings compared with difference near middle"), longMessage, corruptedMessage);

    corruptedMessage[75] = 'd';
    corruptedMessage[100] = 'x';

    check_equality(LINE("Long strings compared with difference near end"), corruptedMessage, longMessage);
    check_equality(LINE("Long strings compared with difference near end"), longMessage, corruptedMessage);
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
      check_equality(LINE(""), a, b, tutor{[](int, int){ return "Nested int advice";}});
    }

    {
      type b{t_0{{1, 2.1f}, {2, 2.8f}}, {3.4, -9.6, 3.2}, {1.1, 0.2}};
      check_equality(LINE(""), a, b, tutor{[](const std::set<double>&, const std::set<double>&){
                                       return "Note reordering of elements upon set construction";
                                     }});
    }
  }

  void false_positive_diagnostics::test_regular_semantics()
  {
    check_semantics(LINE("Broken check invariant"), perfectly_normal_beast{1}, perfectly_normal_beast{1});
    check_semantics(LINE("Broken equality"), broken_equality{1}, broken_equality{2});
    check_semantics(LINE("Broken inequality"), broken_inequality{1}, broken_inequality{2});
    check_semantics(LINE("Broken copy"), broken_copy{1}, broken_copy{2});
    check_semantics(LINE("Broken move"), broken_move{1}, broken_move{2});
    check_semantics(LINE("Broken copy assignment"), broken_copy_assignment{1}, broken_copy_assignment{2});
    check_semantics(LINE("Broken move assignment"), broken_move_assignment{1}, broken_move_assignment{2});
    check_semantics(LINE("Broken self copy assignment"), broken_self_copy_assignment{1}, broken_self_copy_assignment{2});
    check_semantics(LINE("Broken swap"), broken_swap{1}, broken_swap{2});
    check_semantics(LINE("Broken self swap"), broken_self_swap{1}, broken_self_swap{2});
    check_semantics(LINE("Broken copy value semantics"), broken_copy_value_semantics{1}, broken_copy_value_semantics{2}, [](auto& b){ *b.x.front() = 3; });
    check_semantics(LINE("Broken copy assignment value semantics"),
                            broken_copy_assignment_value_semantics{1}, broken_copy_assignment_value_semantics{2}, [](auto& b){ *b.x.front() = 3; });
    check_semantics(LINE("Broken serialization"), broken_serialization{1}, broken_serialization{2});
    check_semantics(LINE("Broken deserialization"), broken_deserialization{1}, broken_deserialization{2});
  }

  void false_positive_diagnostics::test_equivalence_checks()
  {
    check_equivalence(LINE(""), std::string{"foo"}, "fo");
    check_equivalence(LINE(""), std::string{"foo"}, "fob", tutor{[](char, char){
        return "Sort your chars out!";
      }});

    check_equivalence(LINE(""), std::vector<std::string>{{"a"}, {"b"}}, std::initializer_list<std::string_view>{"a", "c"});

    check_equivalence(LINE(""), std::vector<std::string>{{"a"}, {"b"}}, std::initializer_list<std::string_view>{"a", "c"}, tutor{[](char, char){
        return "Ah, chars. So easy to get wrong.";
    }});

    check_equivalence(LINE(""), std::pair<const int&, double>{5, 7.8}, std::pair<int, const double&>{-5, 6.8});
    check_equivalence(LINE(""), std::tuple<const int&, double>{5, 7.8}, std::tuple<int, const double&>{-5, 6.8});

    check_equivalence(LINE("Inequivalence of two different paths, neither of which exists"),
                      fs::path{working_materials()}.append("Stuff/Blah"),
                      fs::path{working_materials()}.append("Stuff/Blurg"));

    check_equivalence(LINE("Inequivalence of two different paths, one of which exists"),
                      fs::path{working_materials()}.append("Stuff/Blah"),
                      fs::path{working_materials()}.append("Stuff/A"));

    check_equivalence(LINE("Inequivalence of directory/file"),
                      fs::path{working_materials()}.append("Stuff/A"),
                      fs::path{working_materials()}.append("Stuff/A/foo.txt"));

    check_equivalence(LINE("Inequivalence of differently named files"),
                      fs::path{working_materials()}.append("Stuff/B/foo.txt"),
                      fs::path{working_materials()}.append("Stuff/B/bar.txt"));

    check_equivalence(LINE("Inequivalence of file contents"),
                      fs::path{working_materials()}.append("Stuff/A/foo.txt"),
                      fs::path{working_materials()}.append("Stuff/B/foo.txt"));

    check_equivalence(LINE("Inequivalence of differently named directories with the same contents"),
                      fs::path{working_materials()}.append("Stuff/A"),
                      fs::path{working_materials()}.append("Stuff/C"));

    check_equivalence(LINE("Inequivalence of directories with the same files but different contents"),
                      fs::path{working_materials()}.append("Stuff/A"),
                      fs::path{working_materials()}.append("MoreStuff/A"));

    check_equivalence(LINE("Inequivalence of directories with some common files"),
                      fs::path{working_materials()}.append("Stuff/B"),
                      fs::path{working_materials()}.append("MoreStuff/B"));

    check_equivalence(LINE("Inequivalence of directories with some common files"),
                      fs::path{working_materials()}.append("MoreStuff/B"),
                      fs::path{working_materials()}.append("Stuff/B"));

    check_equivalence(LINE("File inequivalence when default file checking is used"),
                      fs::path{working_materials()}.append("CustomComparison/A/DifferingContent.ignore"),
                      fs::path{working_materials()}.append("CustomComparison/B/DifferingContent.ignore"));

    check_equivalence(
      LINE("Range inequivalence when default file checking us used"),
      std::vector<fs::path>{fs::path{working_materials()}.append("CustomComparison/A/DifferingContent.ignore")},
      std::vector<fs::path>{fs::path{working_materials()}.append("CustomComparison/B/DifferingContent.ignore")});

    check_equivalence(LINE("Advice for equivalence checking"), foo{42}, 41, bland{});

    check_equivalence(LINE("Advice for range equivalence, where the containized for is explicitly specialized"),
      std::vector<foo>{{42}}, std::vector<int>{{41}}, bland{});
  }

  void false_positive_diagnostics::test_weak_equivalence_checks()
  {
    using beast = perfectly_normal_beast<int>;
    check_weak_equivalence(LINE(""), beast{1, 2}, std::initializer_list<int>{1, 1});
    check_weak_equivalence(LINE(""), beast{1, 2}, std::initializer_list<int>{1, 1}, tutor{[](int, int){
        return "Don't mess with the beast.";
      }});

    using prediction = std::initializer_list<std::initializer_list<int>>;
    check_weak_equivalence(LINE(""), std::vector<beast>{{1, 2}, {3, 4}}, prediction{{1, 2}, {3, 5}});
    check_weak_equivalence(LINE(""), std::vector<beast>{{1, 2}, {3, 4}}, prediction{{1, 2}, {3, 5}},
                           tutor{[](int, int){
                             return "Or at least don't mess with a vector of beasts.";
                           }});

    check_weak_equivalence(LINE("Weak inequivalence of directories with some common files"),
                           fs::path{working_materials()}.append("MoreStuff/B"),
                           fs::path{working_materials()}.append("Stuff/B"));

    check_weak_equivalence(LINE("Directory weak inequivalence when default file checking is used"),
                           fs::path{working_materials()}.append("CustomComparison/A"),
                           fs::path{working_materials()}.append("CustomComparison/B"));

    check_weak_equivalence(
      LINE("Weak inequivalence of range when default file checking is used"),
      std::vector<fs::path>{{fs::path{working_materials()}.append("CustomComparison/A")}},
      std::vector<fs::path>{{fs::path{working_materials()}.append("CustomComparison/B")}});
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
    test_regular_semantics();
    test_equivalence_checks();
    test_weak_equivalence_checks();
  }

  void false_negative_diagnostics::basic_tests()
  {
    check(LINE(""), true);

    check_equality(LINE(""), 5, 5);
    check_equality(LINE(""), 5.0, 5.0);
  }

  void false_negative_diagnostics::test_exceptions()
  {
    check_exception_thrown<std::runtime_error>(LINE(""), [](){ throw std::runtime_error("Error");});

    check_exception_thrown<int>(LINE(""), [](){ throw 1; });
  }

  void false_negative_diagnostics::test_heterogeneous()
  {
    check_equality(LINE(""), std::pair<int, double>{5, 7.8}, std::pair<int, double>{5, 7.8});

    check_equality(LINE(""), std::tuple<int, double, float>{4, 3.4, -9.2f}, std::tuple<int, double, float>{4, 3.4, -9.2f});
  }

  void false_negative_diagnostics::test_variant()
  {
    using var = std::variant<int, double>;

    check_equality(LINE(""), var{0}, var{0});
    check_equality(LINE(""), var{0.0}, var{0.0});
    check_equality(LINE(""), var{3}, var{3});
    check_equality(LINE(""), var{4.0}, var{4.0});
  }

  void false_negative_diagnostics::test_optional()
  {
    using opt = std::optional<int>;
    check_equality(LINE(""), opt{}, opt{});
    check_equality(LINE(""), opt{0}, opt{0});
    check_equality(LINE(""), opt{-1}, opt{-1});
  }

  void false_negative_diagnostics::test_container_checks()
  {
    check_equality(LINE("Empty vector check which should pass"), std::vector<double>{}, std::vector<double>{});
    check_equality(LINE("One element vector check which should pass"), std::vector<double>{2}, std::vector<double>{2});
    check_equality(LINE("Multi-element vector comparison which should pass"), std::vector<double>{1,5}, std::vector<double>{1,5});

    std::vector<float> refs{-4.3f, 2.8f, 6.2f, 7.3f}, ans{1.1f, -4.3f, 2.8f, 6.2f, 8.4f, 7.3f};

    check_range(LINE("Iterators demarcate identical elements"), refs.cbegin(), refs.cbegin()+3, ans.cbegin()+1, ans.cbegin()+4);
  }


  void false_negative_diagnostics::test_strings()
  {
    check_equality(LINE("Differing strings"), std::string{"Hello, World!"}, std::string{"Hello, World!"});
  }

  void false_negative_diagnostics::test_mixed()
  {
    using t_0 = std::vector<std::pair<int, float>>;
    using t_1 = std::set<double>;
    using t_2 = std::complex<double>;
    using type = std::tuple<t_0, t_1, t_2>;

    type a{t_0{{1, 2.1f}, {2, 2.8f}}, {3.3, -9.6, 3.2}, {1.1, 0.2}};
    type b{t_0{{1, 2.1f}, {2, 2.8f}}, {3.3, -9.6, 3.2}, {1.1, 0.2}};

    check_equality(LINE(""), a, b);
  }

  void false_negative_diagnostics::test_regular_semantics()
  {
    check_semantics(LINE(""), perfectly_normal_beast{1}, perfectly_normal_beast{2});
    check_semantics(LINE(""), perfectly_stringy_beast{}, perfectly_stringy_beast{"Hello, world"});
  }

  void false_negative_diagnostics::test_equivalence_checks()
  {
    check_equivalence(LINE(""), std::string{"foo"}, "foo");

    check_equivalence(LINE(""), std::vector<std::string>{{"a"}, {"b"}}, std::initializer_list<std::string_view>{"a", "b"});

    check_equivalence(LINE("Equivalence of a file to itself"),
                      fs::path{working_materials()}.append("Stuff/A/foo.txt"),
                      fs::path{working_materials()}.append("Stuff/A/foo.txt"));

    check_equivalence(LINE("Equivalence of a directory to itself"),
                      fs::path{working_materials()}.append("Stuff/A"),
                      fs::path{working_materials()}.append("Stuff/A"));

    check_equivalence(LINE("Equivalence of a directory, with sub-directories to itself"),
                      fs::path{working_materials()}.append("Stuff"),
                      fs::path{working_materials()}.append("Stuff"));

    check_equivalence(LINE("Equivalence of identical directories in different locations"),
                      fs::path{working_materials()}.append("Stuff/C"),
                      fs::path{working_materials()}.append("SameStuff/C"));

    check_equivalence(LINE("File equivalence when .ignore is ignored"),
                      value_based_customization<bespoke_file_checker>{},
                      fs::path{working_materials()}.append("CustomComparison/A/DifferingContent.ignore"),
                      fs::path{working_materials()}.append("CustomComparison/B/DifferingContent.ignore"));

    check_equivalence(LINE("Range equivalence when .ignore is ignored"),
                      value_based_customization<bespoke_file_checker>{},
                      std::vector<fs::path>{fs::path{working_materials()}.append("CustomComparison/A/DifferingContent.ignore")},
                      std::vector<fs::path>{fs::path{working_materials()}.append("CustomComparison/B/DifferingContent.ignore")});

    check_equivalence(LINE("Advice for equivalence checking"), foo{42}, 42, bland{});

    check_equivalence(LINE("Advice for range equivalence, where the containized for is explicitly specialized"), 
                      std::vector<foo>{{42}}, std::vector<int>{{42}}, bland{});
  }

  void false_negative_diagnostics::test_weak_equivalence_checks()
  {
    using beast = perfectly_normal_beast<int>;
    check_weak_equivalence(LINE(""), beast{1, 2}, std::initializer_list<int>{1, 2});

    using prediction = std::initializer_list<std::initializer_list<int>>;
    check_weak_equivalence(LINE(""), std::vector<beast>{{1, 2}, {3, 4}}, prediction{{1, 2}, {3, 4}});

    check_weak_equivalence(LINE("Weak equivalence of directories in with the same contents but different names"),
                           fs::path{working_materials()}.append("Stuff"),
                           fs::path{working_materials()}.append("SameStuff"));

    check_weak_equivalence<>(LINE("Weak equivalence when .ignore is ignored"),
                             value_based_customization<bespoke_file_checker>{},
                             fs::path{working_materials()}.append("CustomComparison/A"),
                             fs::path{working_materials()}.append("CustomComparison/B"));

    check_weak_equivalence(LINE("Weak equivalence of range when .ignore is ignored"),
                           value_based_customization<bespoke_file_checker>{},
                           std::vector<fs::path>{{fs::path{working_materials()}.append("CustomComparison/A")}},
                           std::vector<fs::path>{{fs::path{working_materials()}.append("CustomComparison/B")}});
  }
}
