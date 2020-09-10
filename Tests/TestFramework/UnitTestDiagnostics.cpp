////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "UnitTestDiagnostics.hpp"
#include "UnitTestDiagnosticsUtilities.hpp"
#include "FileSystem.hpp"

#include <complex>
#include <vector>
#include <set>
#include <filesystem>

namespace sequoia::testing
{
  template<>
  struct serializer<std::filesystem::file_type>
  {
    [[nodiscard]]
    static std::string make(const std::filesystem::file_type& val)
    {
      using ft = std::filesystem::file_type;
      switch(val)
      {
      case ft::none:
        return "none";
      case ft::not_found:
        return "not found";
      case ft::regular:
        return "regular";
      case ft::directory:
        return "directory";
      case ft::symlink:
        return "symlink";
      case ft::block:
        return "block";
      case ft::character:
        return "character";
      case ft::fifo:
        return "fifo";
      case ft::socket:
        return "socket";
      case ft::unknown:
        return "unknown";
      default:
        return "unrecognized";
      }
    }
  };
  
  template<>
  struct equivalence_checker<std::filesystem::path>
  {
    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const std::filesystem::path& path, const std::filesystem::path& prediction)
    {      
      namespace fs = std::filesystem;

      const auto pathType{fs::status(path).type()};
      const auto predictionType{fs::status(prediction).type()};

      if(check_equality(preamble("Path type", path, prediction), logger, pathType, predictionType))
      {
        if(!path.empty())
        {
          const auto pathFinalToken{*(--path.end())};
          const auto predictionFinalToken{*(--prediction.end())};
          if(check_equality("Final path token", logger, pathFinalToken, predictionFinalToken))
          {
            switch(pathType)
            {
            case fs::file_type::regular:
              check_file(logger, path, prediction);
              break;
            case fs::file_type::directory:
              check_directory(logger, path, prediction);
              break;
            default:
              throw std::logic_error{std::string{"Detailed equivalance check for paths of type "}
                .append(serializer<fs::file_type>::make(pathType)).append(" not currently implemented")};
            }
          }
        }
      }          
    }

  private:
    template<test_mode Mode>
    static void check_directory(test_logger<Mode>& logger,const std::filesystem::path& dir, const std::filesystem::path& prediction)
    {
      namespace fs = std::filesystem;

      std::vector<fs::path> paths, predictedPaths;
      for(auto& p : fs::directory_iterator(dir))        paths.push_back(p);
      for(auto& p : fs::directory_iterator(prediction)) predictedPaths.push_back(p);

      std::sort(paths.begin(), paths.end());
      std::sort(predictedPaths.begin(), predictedPaths.end());

      check_equality(std::string{"Number of directory entries for "}.append(dir), logger, paths.size(), predictedPaths.size());

      const auto iters{std::mismatch(paths.begin(), paths.end(), predictedPaths.begin(), predictedPaths.end(),
          [&dir,&prediction](const fs::path& lhs, const fs::path& rhs){
            return fs::relative(lhs, dir) == fs::relative(rhs, prediction);
          })};
      if((iters.first != paths.end()) && (iters.second != predictedPaths.end()))
      {
        check_equality("First directory entry mismatch", logger, *iters.first, *iters.second);
      }
      else if(iters.first != paths.end())
      {
        check_equality("First directory entry mismatch", logger, *iters.first, fs::path{});
      }
      else if(iters.second != predictedPaths.end())
      {
        check_equality("First directory entry mismatch", logger, fs::path{}, *iters.second);
      }
      else
      {
        for(std::size_t i{}; i < paths.size(); ++i)
        {
          check(logger, paths[i], predictedPaths[i]);
        }
      } 
    }

    template<test_mode Mode>
    static void check_file(test_logger<Mode>& logger,const std::filesystem::path& file, const std::filesystem::path& prediction)
    {
      std::ifstream file1{file}, file2{prediction};

      if(   testing::check(report_failed_read(file), logger, static_cast<bool>(file1))
         && testing::check(report_failed_read(prediction), logger, static_cast<bool>(file2)))
      {
        std::stringstream buffer1{}, buffer2{};
        buffer1 << file1.rdbuf();
        buffer2 << file2.rdbuf();

        check_equality(preamble("Contents of", file, prediction), logger, buffer1.str(), buffer2.str());
      }
    }

    [[nodiscard]]
    static std::string report_file_issue(const std::filesystem::path& file, std::string_view description)
    {
      auto mess{std::string{"Unable to open file "}.append(file)};
      if(!description.empty()) mess.append(" ").append(description);
      mess.append("\n");

      return mess;
    }

    [[nodiscard]]
    static std::string report_failed_read(const std::filesystem::path& file)
    {
      return report_file_issue(file, " for reading");
    }

    [[nodiscard]]
    static std::string preamble(std::string_view prefix, const std::filesystem::path& path, const std::filesystem::path& prediction)
    {
      return append_lines(prefix, path.string(), "vs", prediction.string()).append("\n");
    }
  };


  
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

    check_equivalence(LINE("Inequivalence of directory/file"),
                      aux_path("UnitTestCodeTemplates").append("CodeTemplates"),
                      aux_path("UnitTestCodeTemplates").append("CodeTemplates").append("MyClassTest.hpp"));

    check_equivalence(LINE("Inequivalence of differently named files"),
                      aux_path("UnitTestCodeTemplates").append("CodeTemplates").append("MyClassTest.cpp"),
                      aux_path("UnitTestCodeTemplates").append("CodeTemplates").append("MyClassTest.hpp"));

    check_equivalence(LINE("Inequivalence of file contents"),
                      aux_path("FileEditingTestMaterials").append("BeforeEditing").append("FakeMain.cpp"),
                      aux_path("FileEditingTestMaterials").append("AfterEditing").append("FakeMain.cpp"));

    check_equivalence(LINE("Inequivalence of differently named directories with the same contents"),
                      aux_path("PathEquivalenceTestMaterials").append("BeforeEditing"),
                      aux_path("PathEquivalenceTestMaterials").append("AfterEditing"));

    check_equivalence(LINE("Inequivalence of directories with the same files but different contents"),
                      aux_path("FileEditingTestMaterials").append("AfterEditing"),
                      aux_path("PathEquivalenceTestMaterials").append("AfterEditing"));

    namespace fs = std::filesystem;
    fs::copy(aux_path("PathEquivalenceTestMaterials"), self_diag_output_path("PathEquivalenceTestMaterials"), fs::copy_options::recursive);
    fs::remove(self_diag_output_path("PathEquivalenceTestMaterials").append("BeforeEditing").append("FakeMain.cpp"));

    check_equivalence(LINE("Inequivalence of directories with some common files"),
                      aux_path("PathEquivalenceTestMaterials"),                      
                      self_diag_output_path("PathEquivalenceTestMaterials"));
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

    check_equivalence(LINE("Equivalence of a directory, with sub-directories to itself"),
                      aux_path("FileEditingTestMaterials"),
                      aux_path("FileEditingTestMaterials"));

    check_equivalence(LINE("Equivalence of identical directories in different locations"),
                      aux_path("FileEditingTestMaterials").append("BeforeEditing"),
                      aux_path("PathEquivalenceTestMaterials").append("BeforeEditing"));
  }

  void false_negative_diagnostics::test_weak_equivalence_checks()
  {
    using beast = perfectly_normal_beast<int>;
    check_weak_equivalence(LINE(""), beast{1, 2}, std::initializer_list<int>{1, 2});

    using prediction = std::initializer_list<std::initializer_list<int>>;
    check_weak_equivalence(LINE(""), std::vector<beast>{{1, 2}, {3, 4}}, prediction{{1, 2}, {3, 4}});
  }
}
