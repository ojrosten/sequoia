////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "BasicTestInterfaceFreeTest.hpp"
#include "Parsing/CommandLineArgumentsTestingUtilities.hpp"

#include "sequoia/TestFramework/FreeTestCore.hpp"
#include "sequoia/TestFramework/TestRunner.hpp"

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    class fake_test : public free_test {
    public:
      using free_test::free_test;
    };

    class fake_test_with_discriminated_summary : public free_test {
    public:
      using free_test::free_test;

      [[nodiscard]]
      std::string summary_discriminator() const { return "bar"; }
    };

    class fake_test_with_discriminated_exceptions : public free_test {
    public:
      using free_test::free_test;

      [[nodiscard]]
      std::string output_discriminator() const { return "baz"; }
    };
  }

  [[nodiscard]]
  std::filesystem::path basic_test_interface_free_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  [[nodiscard]]
  std::filesystem::path basic_test_interface_free_test::fake_project() const
  {
    return working_materials() /= "FakeProject";
  }

  [[nodiscard]]
  fs::path basic_test_interface_free_test::minimal_fake_path() const
  {
    return fake_project().append("build/CMade");
  }

  void basic_test_interface_free_test::run_tests()
  {
    commandline_arguments args{{(minimal_fake_path()).generic_string()}};

    std::stringstream outputStream{};
    test_runner runner{args.size(),
                       args.get(),
                       "Oliver J. Rosten",
                       "  ",
                       {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}},
                       outputStream};

    const auto& projPaths{runner.proj_paths()};
    const auto rebasedSource{rebase_from(source_file(), get_project_paths().project_root())};

    {
      fake_test t{"fake test", "foo suite", source_file(), projPaths, {}, {}, {}, {}};

      check(equality,
            reporter{"Summary File Path"},
            t.summary_file_path().file_path(),
            (projPaths.output().test_summaries() / rebasedSource).replace_extension(".txt"));
      
      check(equality,
            reporter{"Exceptions File Path"},
            t.diagnostics_file_paths().caught_exceptions_file_path(),
            projPaths.output().diagnostics() / "foo_suite" / source_file().filename().stem().concat("_Exceptions.txt"));
    }

    {
      fake_test t{"fake test", "foo suite", source_file(), projPaths, {}, {}, {""}, {""}};

      check(equality,
        reporter{"Summary File Path"},
        t.summary_file_path().file_path(),
        (projPaths.output().test_summaries() / rebasedSource).replace_extension(".txt"));

      check(equality,
        reporter{"Exceptions File Path"},
        t.diagnostics_file_paths().caught_exceptions_file_path(),
        projPaths.output().diagnostics() / "foo_suite" / source_file().filename().stem().concat("_Exceptions.txt"));
    }

    {
      fake_test_with_discriminated_summary t{"fake test", "foo suite", source_file(), projPaths, {}, {}, {}, {"bar"}};

      check(equality,
            reporter{"Summary File Path"},
            t.summary_file_path().file_path(),
            (projPaths.output().test_summaries() / rebasedSource).replace_filename(source_file().stem().concat("_bar.txt")));

      check(equality,
            reporter{"Exceptions File Path"},
            t.diagnostics_file_paths().caught_exceptions_file_path(),
            projPaths.output().diagnostics() / "foo_suite" / source_file().filename().stem().concat("_Exceptions.txt"));
    }

    {
      fake_test_with_discriminated_exceptions t{"fake test", "foo suite", source_file(), projPaths, {}, {}, {"baz"}, {}};

      check(equality,
            reporter{"Summary File Path"},
            t.summary_file_path().file_path(),
            (projPaths.output().test_summaries() / rebasedSource).replace_extension(".txt"));

      check(equality,
            reporter{"Exceptions File Path"},
            t.diagnostics_file_paths().caught_exceptions_file_path(),
            projPaths.output().diagnostics() / "foo_suite" / source_file().filename().stem().concat("_Exceptions_baz.txt"));
    }
  }
}
