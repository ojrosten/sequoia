////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "Parsing/CommandLineArgumentsTestingUtilities.hpp"

#include "sequoia/TestFramework/TestExecutionRecord.hpp"
#include "sequoia/TestFramework/TestRunner.hpp"

#include <format>
#include <sstream>
#include <stdexcept>

namespace sequoia::testing
{
  [[nodiscard]]
  inline project_paths::customizer fake_project_customization()
  {
    return {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}};
  }

  [[nodiscard]]
  inline test_runner make_fake_project_runner(commandline_arguments& args, std::stringstream& outputStream)
  {
    return test_runner{args.size(),
                       args.get(),
                       "Oliver J. Rosten",
                       "  ",
                       fake_project_customization(),
                       outputStream};
  }

  /** Reads the completion of the last record `runner` keeps for `Test`. */
  template<concrete_test Test>
  [[nodiscard]]
  test_execution_completion get_recorded_completion(const test_runner& runner)
  {
    const test_execution_record_path record{Test::source_file(), test_name<Test>(), runner.proj_paths()};
    if(auto records{read_test_execution_records(record.file_path())}; !records.empty() && records.back().completion)
      return std::move(records.back().completion).value();

    throw std::runtime_error{std::format("No completion in: {}", record.file_path().generic_string())};
  }
}
