////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "sequoia/TestFramework/FreeTestCore.hpp"

namespace sequoia::testing
{
  class test_runner_test_creation final : public free_test
  {
    /** \brief Where a fake project's main sits relative to its top-level CMake source directory. */
    enum class main_location { in_source_dir, below_source_dir };

  public:
    using free_test::free_test;

    [[nodiscard]]
    static std::filesystem::path source_file();

    void run_tests();
  private:

    void test_type_handling();

    void test_project_namespace();

    void test_template_data_generation();

    void test_creation(std::string_view projectName,
                       std::optional<std::string> sourceFolder,
                       main_location mainLocation);

    void test_cmake_rerun_failures(std::string_view projectName,
                                   const std::optional<std::string>& sourceFolder,
                                   const std::filesystem::path& cacheFile,
                                   const main_paths& fakeMain);

    void record_cmake_source_dir(const std::filesystem::path& cacheFile, const std::filesystem::path& sourceDir);

    void test_creation_failure();

    [[nodiscard]]
    std::string zeroth_arg(std::string_view projectName) const;

    void check_directory(std::string_view projectName, std::string_view dirName);
  };
}
