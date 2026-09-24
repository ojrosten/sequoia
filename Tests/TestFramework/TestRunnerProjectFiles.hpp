////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "sequoia/TestFramework/FreeTestCore.hpp"

#include <vector>

namespace sequoia::testing
{
  /** \brief Checks the project files a created project's build system generates.

      What there is to check depends on the generator. Under Visual Studio the generated
      project is configured once per committed prediction, each with the preset the
      prediction is named after, and its `.vcxproj` compared; under Ninja it is configured
      with this tree's preset and its `build.ninja` must have an edge for the test target.
      The generator is this build tree's, and each configure is checked to agree. Under any
      other generator the test checks nothing.

      The predictions are Visual Studio's alone, so the materials are discriminated by the
      generator family: a run under any other generator neither reads nor updates them.

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

    /** Named for every generator family, so that no family's materials are the undiscriminated ones. */
    [[nodiscard]]
    static std::string materials_discriminator(const cmake_cache& cache);

    void run_tests();
  private:
    void generate_project();

    /** \brief Configures the generated project with a preset, returning its cache directory. */
    [[nodiscard]]
    std::filesystem::path configure_generated_project(const cmake_cache& cache, const std::filesystem::path& preset);

    [[nodiscard]]
    std::vector<std::filesystem::path> predicted_presets() const;

    void check_visual_studio_project_files(const cmake_cache& cache);

    void check_ninja_project_files(const std::filesystem::path& cacheDir);

    [[nodiscard]]
    std::filesystem::path generated_project() const;
  };
}
