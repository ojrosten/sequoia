////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file UnitTestRunner.hpp
    \brief Helper for running unit tests from the command line.
*/

#include "UnitTestFamily.hpp"

#include <map>

namespace sequoia::unit_testing
{
  template<class Iter> void pad_right(Iter begin, Iter end, std::string_view suffix)
  {
    auto maxIter{
      std::max_element(begin, end, [](const std::string& lhs, const std::string& rhs) {
          return lhs.size() < rhs.size();
        }
      )
    };

    const auto maxChars{maxIter->size()};

    for(; begin != end; ++begin)
    {
      auto& s{*begin};
      s += std::string(maxChars - s.size(), ' ') += std::string{suffix};
    }
  }  
      
  template<class Iter> void pad_left(Iter begin, Iter end, const std::size_t minChars)
  {
    auto maxIter{std::max_element(begin, end, [](const std::string& lhs, const std::string& rhs) {
          return lhs.size() < rhs.size();
        }
      )
    };

    const auto maxChars{std::max(maxIter->size(), minChars)};

    for(; begin != end; ++begin)
    {
      auto& s{*begin};
      s = std::string(maxChars - s.size(), ' ') + s;
    }
  }
  
  struct argument_error : std::runtime_error
  {
    using std::runtime_error::runtime_error;
  };


  [[nodiscard]]
  std::string error(std::string_view message);

  [[nodiscard]]
  std::string summarize(const log_summary& log, std::string_view prefix, const log_verbosity suppression);

  struct commandline_option_info
  {
    std::vector<std::string> parameters;
    std::vector<std::string> aliases;
  };

  [[nodiscard]]
  std::vector<std::vector<std::string>>
  parse(int argc, char** argv, const std::map<std::string, commandline_option_info>& info);

  class unit_test_runner
  {
  public:
    unit_test_runner(int argc, char** argv);

    unit_test_runner(const unit_test_runner&) = delete;
    unit_test_runner(unit_test_runner&&)      = default;

    void add_test_family(test_family&& f);

    void execute();
  private:
    using arg_list = std::vector<std::string>;

    struct nascent_test
    {
      nascent_test(std::string dir, std::string qualifiedName);
      
      std::string directory, qualified_class_name, class_name;
    };

    const static std::map<std::string, std::size_t> s_ArgCount;
    
    std::vector<test_family> m_Families;
    std::map<std::string, std::function<void (std::string_view, const arg_list&)>> m_FunctionMap;
    std::map<std::string, bool> m_SpecificTests{};
    std::vector<nascent_test> m_NewFiles{};
    
    bool m_Asynchronous{}, m_Verbose{}, m_Pause{}, m_WriteFiles{true};

    const static std::array<std::string_view, 5> st_TestNameStubs;

    log_summary process_family(const std::vector<log_summary>& summaries);

    void build_map();

    void run_diagnostics();

    void run_tests();

    void check_for_missing_tests();

    void check_argument_consistency();

    enum class file_comparison {failed, same, different};

    static std::string to_camel_case(std::string text);
    
    static std::string warning(std::string_view message);

    static std::string report_arg_num(const std::size_t n);

    static argument_error generate_argument_error(std::string_view option, const arg_list& argList, const std::size_t num, std::string_view expectedArgs);

    static void check_zero_args(std::string_view option, const arg_list& argList);

    static void replace_all(std::string& text, std::string_view from, const std::string& to);

    static bool file_exists(const std::string& path);

    template<class Iter>
    static void create_files(Iter beginFiles, Iter endFiles, std::string_view message, const bool overwrite);

    static void create_file(const nascent_test& data, std::string_view partName, const bool overwrite);

    static auto compare_files(std::string_view referenceFile, std::string_view generatedFile) -> file_comparison;

    template<class Iter>
    static void compare_files(Iter beginFiles, Iter endFiles, std::string_view message);

    static void compare_files(const nascent_test& data, std::string_view partName);

    static void false_positive_check(const nascent_test& data);
    
    static void argument_processing_diagnostics();
  };
}
