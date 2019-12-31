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
  struct argument_error : std::runtime_error
  {
    using std::runtime_error::runtime_error;
  };

  [[nodiscard]]
  std::string summarize(const log_summary& log, std::string_view prefix, const log_verbosity suppression);

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
    std::map<std::string, std::function<void (const arg_list&)>> m_FunctionMap;
    std::map<std::string, bool> m_SpecificTests{};
    std::vector<nascent_test> m_NewFiles{};
    
    bool m_Asynchronous{}, m_Verbose{}, m_Pause{};

    const static std::array<std::string, 5> st_TestNameStubs;

    log_summary process_family(const std::vector<log_summary>& summaries);

    void build_map();

    void run_diagnostics();

    void run_tests();

    void check_for_missing_tests();

    enum class file_comparison {failed, same, different};

    
    static std::string to_camel_case(std::string text);
    
    static std::string warning(std::string_view message);

    static std::string error(std::string_view message);

    static std::string report_arg_num(const std::size_t n);

    static void replace_all(std::string& text, std::string_view from, const std::string& to);

    static bool file_exists(const std::string& path);

    template<class Iter>
    static void create_files(Iter beginFiles, Iter endFiles, std::string_view message, const bool overwrite);

    static void create_file(const nascent_test& data, std::string_view partName, const bool overwrite);

    static auto compare_files(const std::string& referenceFile, const std::string& generatedFile) -> file_comparison;

    template<class Iter>
    static void compare_files(Iter beginFiles, Iter endFiles, std::string_view message);

    static void compare_files(const nascent_test& data, const std::string& partName);

    static void false_positive_check(const nascent_test& data);
  };
}
