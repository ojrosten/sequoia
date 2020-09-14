////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "CoreDiagnostics.hpp"
#include "CoreDiagnosticsUtilities.hpp"
#include "FileSystem.hpp"

#include <complex>
#include <vector>
#include <set>
#include <filesystem>

namespace sequoia::testing
{  
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

  struct bland
  {
    std::string operator()(int, int) const
    {
      return {"Integer advice"};
    }

    std::string operator()(double, double) const
    {
      return {"Double advice"};
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
    test_container_checks();
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

    check_equality(LINE(""), std::pair<int, double>{5, 7.8}, std::pair<int, double>{5, -7.8});
    check_equality(LINE(""), std::pair<int, double>{5, 7.8}, std::pair<int, double>{-5, 7.8});
    check_equality(LINE(""), std::pair<int, double>{5, 7.8}, std::pair<int, double>{-5, 6.8}, tutor{bland{}});

    check_equality(LINE(""), std::tuple<int, double, float>{4, 3.4, -9.2f}, std::tuple<int, double, float>{0, 3.4, -9.2f});
    check_equality(LINE(""), std::tuple<int, double, float>{4, 3.4, -9.2f}, std::tuple<int, double, float>{4, 0.0, -9.2f}, tutor{bland{}});
    check_equality(LINE(""), std::tuple<int, double, float>{4, 3.4, -9.2f}, std::tuple<int, double, float>{4, 3.4, -0.0f});

    check_exception_thrown<std::runtime_error>(LINE("Exception expected but nothing thrown"), [](){});
    check_exception_thrown<std::runtime_error>(LINE("Exception thrown but of wrong type"), [](){ throw std::logic_error("Error"); });
    check_exception_thrown<std::runtime_error>(LINE("Exception thrown but of unknown type"), [](){ throw 1; });
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

    std::vector<float> refs{-4.3, 2.8, 6.2, 7.3}, ans{1.1, -4.3, 2.8, 6.2, 8.4, 7.3};

    check_range(LINE("Iterators demarcate differing numbers of elements"), refs.cbegin(), refs.cend(), ans.cbegin(), ans.cend());
    check_range(LINE("Iterators demarcate differing elements"), refs.cbegin(), refs.cend(), ans.cbegin(), ans.cbegin() + 4);

    using namespace std::string_literals;
    check_equality(LINE("Strings of differing length"), "what?!"s, "Hello, World!"s);
    check_equality(LINE("Differing strings of same length"), "Hello, world?"s, "Hello, World!"s);
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
    check_semantics(LINE("Broken equality"), broken_equality{1}, broken_equality{2});
    check_semantics(LINE("Broken inequality"), broken_inequality{1}, broken_inequality{2});
    check_semantics(LINE("Broken copy"), broken_copy{1}, broken_copy{2});
    check_semantics(LINE("Broken move"), broken_move{1}, broken_move{2});
    check_semantics(LINE("Broken copy assignment"), broken_copy_assignment{1}, broken_copy_assignment{2});
    check_semantics(LINE("Broken move assignment"), broken_move_assignment{1}, broken_move_assignment{2});
    check_semantics(LINE("Broken swap"), broken_swap{1}, broken_swap{2});
    check_semantics(LINE("Broken copy value semantics"), broken_copy_value_semantics{1}, broken_copy_value_semantics{2}, [](auto& b){ *b.x.front() = 3; });
    check_semantics(LINE("Broken copy assignment value semantics"),
                            broken_copy_assignment_value_semantics{1}, broken_copy_assignment_value_semantics{2}, [](auto& b){ *b.x.front() = 3; });
    check_semantics(LINE("Broken check invariant"), perfectly_normal_beast{1}, perfectly_normal_beast{1});
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
                      aux_path("UnitTestCodeTemplates").append("Blah"),
                      aux_path("UnitTestCodeTemplates").append("Blurg"));

    check_equivalence(LINE("Inequivalence of two different paths, one of which exists"),
                      aux_path("UnitTestCodeTemplates").append("Blah"),
                      aux_path("UnitTestCodeTemplates").append("CodeTemplates"));

    check_equivalence(LINE("Inequivalence of directory/file"),
                      aux_path("UnitTestCodeTemplates").append("CodeTemplates"),
                      aux_path("UnitTestCodeTemplates").append("CodeTemplates").append("MyClassTest.hpp"));

    check_equivalence(LINE("Inequivalence of differently named files"),
                      aux_path("UnitTestCodeTemplates").append("CodeTemplates").append("MyClassTest.cpp"),
                      aux_path("UnitTestCodeTemplates").append("CodeTemplates").append("MyClassTest.hpp"));

    namespace fs = std::filesystem;

    check_equivalence(LINE("Inequivalence of file contents"),
                      fs::path{materials()}.append("Stuff").append("A").append("foo.txt"),
                      fs::path{materials()}.append("Stuff").append("B").append("foo.txt"));

    check_equivalence(LINE("Inequivalence of differently named directories with the same contents"),
                      fs::path{materials()}.append("Stuff").append("A"),
                      fs::path{materials()}.append("Stuff").append("C"));

    check_equivalence(LINE("Inequivalence of directories with the same files but different contents"),
                      fs::path{materials()}.append("Stuff").append("A"),
                      fs::path{materials()}.append("MoreStuff").append("A"));

    check_equivalence(LINE("Inequivalence of directories with some common files"),
                      fs::path{materials()}.append("Stuff").append("B"),
                      fs::path{materials()}.append("MoreStuff").append("B"));

    check_equivalence(LINE("Inequivalence of directories with some common files"),
                      fs::path{materials()}.append("MoreStuff").append("B"),
                      fs::path{materials()}.append("Stuff").append("B"));
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
  }

  [[nodiscard]]
  std::string_view false_negative_diagnostics::source_file() const noexcept
  {
    return __FILE__;
  }

  void false_negative_diagnostics::run_tests()
  {
    basic_tests();
    test_container_checks();
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

    check_equality(LINE(""), std::pair<int, double>{5, 7.8}, std::pair<int, double>{5, 7.8});

    check_equality(LINE(""), std::tuple<int, double, float>{4, 3.4, -9.2f}, std::tuple<int, double, float>{4, 3.4, -9.2f});

    check_exception_thrown<std::runtime_error>(LINE(""), [](){ throw std::runtime_error("Error");});      
  }

  void false_negative_diagnostics::test_container_checks()
  {
    check_equality(LINE("Empty vector check which should pass"), std::vector<double>{}, std::vector<double>{});
    check_equality(LINE("One element vector check which should pass"), std::vector<double>{2}, std::vector<double>{2});
    check_equality(LINE("Multi-element vector comparison which should pass"), std::vector<double>{1,5}, std::vector<double>{1,5});

    std::vector<float> refs{-4.3, 2.8, 6.2, 7.3}, ans{1.1, -4.3, 2.8, 6.2, 8.4, 7.3};
      
    check_range(LINE("Iterators demarcate identical elements"), refs.cbegin(), refs.cbegin()+3, ans.cbegin()+1, ans.cbegin()+4);

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
  }

  void false_negative_diagnostics::test_equivalence_checks()
  {
    check_equivalence(LINE(""), std::string{"foo"}, "foo");

    check_equivalence(LINE(""), std::vector<std::string>{{"a"}, {"b"}}, std::initializer_list<std::string_view>{"a", "b"});

    check_equivalence(LINE("Equivalence of a file to itself"),
                      aux_path("UnitTestCodeTemplates").append("CodeTemplates").append("MyClassTest.hpp"),
                      aux_path("UnitTestCodeTemplates").append("CodeTemplates").append("MyClassTest.hpp"));

    check_equivalence(LINE("Equivalence of a directory to itself"),
                      aux_path("UnitTestCodeTemplates").append("CodeTemplates"),
                      aux_path("UnitTestCodeTemplates").append("CodeTemplates"));

    namespace fs = std::filesystem;

    check_equivalence(LINE("Equivalence of a directory, with sub-directories to itself"),
                      fs::path{materials()}.append("Stuff"),
                      fs::path{materials()}.append("Stuff"));

    check_equivalence(LINE("Equivalence of identical directories in different locations"),
                      fs::path{materials()}.append("Stuff").append("C"),
                      fs::path{materials()}.append("Stuff").append("C"));
  }

  void false_negative_diagnostics::test_weak_equivalence_checks()
  {
    using beast = perfectly_normal_beast<int>;
    check_weak_equivalence(LINE(""), beast{1, 2}, std::initializer_list<int>{1, 2});

    using prediction = std::initializer_list<std::initializer_list<int>>;
    check_weak_equivalence(LINE(""), std::vector<beast>{{1, 2}, {3, 4}}, prediction{{1, 2}, {3, 4}});
  }
}
