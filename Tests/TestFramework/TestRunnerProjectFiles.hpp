////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "sequoia/TestFramework/FreeTestCore.hpp"

namespace sequoia::testing
{
  /** \brief Checks the project files a created project's build system generates.

      What there is to check depends on the generator: a `.vcxproj` under Visual Studio,
      compared against a prediction; a `build.ninja` under Ninja, which must have an edge
      for the test target. The generator is this build tree's, and the generated project -
      configured with the preset this tree is named after - is checked to agree. Under any
      other generator the test checks nothing.

      It lives here, alone, rather than inside the end-to-end test, so that the far
      larger and more valuable test of project creation and incremental building stays
      free of a summary discriminator.
   */
  class test_runner_project_files final : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    static std::filesystem::path source_file();

    /** The check count varies with the generator, and with nothing else. */
    [[nodiscard]]
    static std::string summary_discriminator(const cmake_cache& cache);

    void run_tests();
  private:
    [[nodiscard]]
    build_paths configure_generated_project(const cmake_cache& cache);

    void check_visual_studio_project_files(const build_paths& build);

    void check_ninja_project_files(const build_paths& build);

    [[nodiscard]]
    std::filesystem::path generated_project() const;
  };
}
