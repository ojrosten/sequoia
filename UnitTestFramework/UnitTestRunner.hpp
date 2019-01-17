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

#include "UnitTestUtils.hpp"

#include <map>

namespace sequoia::unit_testing
{
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

    struct new_file
    {
      new_file(std::string dir, std::string qualifiedName);
      
      std::string directory, qualified_class_name, class_name;
    };

    const static std::map<std::string, std::size_t> s_ArgCount;
    
    std::vector<test_family> m_Families;
    std::map<std::string, std::function<void (const arg_list&)>> m_FunctionMap;
    std::set<std::string> m_SpecificTests{};
    std::vector<new_file> m_NewFiles{};
    
    bool m_Asynchronous{}, m_Verbose{};

    log_summary process_family(const std::vector<log_summary>& summaries);

    void build_map();

    void create_files(std::string_view message, const bool overwrite);

    void create_file(const new_file& data, std::string_view firstPart, std::string_view secondPart, const bool overwrite);

    void run_diagnostics();

    void run_tests();

    
    static std::string to_camel_case(std::string text);
    
    static std::string warning(std::string_view message);

    static std::string report_arg_num(const std::size_t n);

    static void replace_all(std::string& text, std::string_view from, const std::string& to);

    static bool file_exists(const std::string& path);
  };
}
