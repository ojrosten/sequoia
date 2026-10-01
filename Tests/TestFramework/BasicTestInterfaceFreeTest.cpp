////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "BasicTestInterfaceFreeTest.hpp"
#include "Parsing/CommandLineArgumentsTestingUtilities.hpp"
#include "Utilities/TestUtilities.hpp"

#include "sequoia/TestFramework/FreeTestCore.hpp"
#include "sequoia/TestFramework/TestRunner.hpp"
#include "sequoia/TestFramework/FileSystemUtilities.hpp"

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
      static std::string summary_discriminator(const cmake_cache&) { return "bar"; }
    };

    class fake_test_with_discriminated_exceptions : public free_test {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static std::string output_discriminator(const cmake_cache&) { return "baz"; }
    };
  }

  [[nodiscard]]
  fs::path basic_test_interface_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  [[nodiscard]]
  fs::path basic_test_interface_free_test::fake_project() const
  {
    return working_materials() /= "FakeProject";
  }

  [[nodiscard]]
  fs::path basic_test_interface_free_test::minimal_fake_path() const
  {
    return fake_project().append("build/CMade/FakeExe.txt");
  }

  void basic_test_interface_free_test::run_tests()
  {
    date_after_every_edit(minimal_fake_path());

    commandline_arguments args{{(minimal_fake_path()).generic_string()}};

    std::stringstream outputStream{};
    test_runner runner{args.size(),
                       args.get(),
                       "Oliver J. Rosten",
                       "  ",
                       {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}},
                       outputStream};

    test_file_paths(runner.proj_paths());
    test_materials(runner.proj_paths());
    test_discriminated_materials(runner.proj_paths());
  }

  void basic_test_interface_free_test::test_file_paths(const project_paths& projPaths)
  {
    const auto rebasedSource{rebase_from(source_file(), get_project_paths().project_root())};

    {
      fake_test t{test_name<fake_test>(), source_file(), projPaths, {}, {}, null_discriminator, null_discriminator};

      check(equality,
            reporter{"Summary File Path"},
            t.summary_file_path().file_path(),
            projPaths.output().test_summaries() / rebasedSource.parent_path() / "fake_test.txt");
      
      check(equality,
            reporter{"Exceptions File Path"},
            t.diagnostics_file_paths().caught_exceptions_file_path(),
            projPaths.output().diagnostics() / rebasedSource.parent_path() / "fake_test_Exceptions.txt");
    }

    {
      fake_test t{test_name<fake_test>(), source_file(), projPaths, {}, {}, {""}, {""}};

      check(equality,
        reporter{"Summary File Path"},
        t.summary_file_path().file_path(),
        projPaths.output().test_summaries() / rebasedSource.parent_path() / "fake_test.txt");

      check(equality,
        reporter{"Exceptions File Path"},
        t.diagnostics_file_paths().caught_exceptions_file_path(),
        projPaths.output().diagnostics() / rebasedSource.parent_path() / "fake_test_Exceptions.txt");
    }

    {
      fake_test_with_discriminated_summary t{test_name<fake_test_with_discriminated_summary>(), source_file(), projPaths, {}, {}, null_discriminator, {"bar"}};

      check(equality,
            reporter{"Summary File Path"},
            t.summary_file_path().file_path(),
            projPaths.output().test_summaries() / rebasedSource.parent_path() / "fake_test_with_discriminated_summary_bar.txt");

      check(equality,
            reporter{"Exceptions File Path"},
            t.diagnostics_file_paths().caught_exceptions_file_path(),
            projPaths.output().diagnostics() / rebasedSource.parent_path() / "fake_test_with_discriminated_summary_Exceptions.txt");
    }

    {
      fake_test_with_discriminated_exceptions t{test_name<fake_test_with_discriminated_exceptions>(), source_file(), projPaths, {}, {}, {"baz"}, null_discriminator};

      check(equality,
            reporter{"Summary File Path"},
            t.summary_file_path().file_path(),
            projPaths.output().test_summaries() / rebasedSource.parent_path() / "fake_test_with_discriminated_exceptions.txt");

      check(equality,
            reporter{"Exceptions File Path"},
            t.diagnostics_file_paths().caught_exceptions_file_path(),
            projPaths.output().diagnostics() / rebasedSource.parent_path() / "fake_test_with_discriminated_exceptions_Exceptions_baz.txt");
    }
  }

  /** Fake tests in the fake project, whose original materials are inputs alone, predictions alone,
      auxiliary materials alone, none at all, and inputs beside a stray file. Each has its materials
      prepared as the runner prepares them.
   */
  void basic_test_interface_free_test::test_materials(const project_paths& projPaths)
  {
    const auto temporaryRoot{
      [&projPaths](std::string_view sourceStem) {
        return projPaths.output().tests_temporary_data() / "Materials" / sourceStem / "fake_test";
      }
    };

    const auto preparedTest{
      [&projPaths](std::string_view sourceStem) {
        const auto source{projPaths.tests().repo() / "Materials" / std::format("{}.cpp", sourceStem)};
        const individual_materials_paths materials{source, "fake_test", projPaths, null_discriminator};
        prepare_materials(materials);
        return std::pair{fake_test{"fake_test", source, projPaths, materials, {}, null_discriminator, null_discriminator}, materials};
      }
    };

    {
      const auto [test, materials]{preparedTest("WithInputs")};

      check(equality,
            "Working copy of inputs alone",
            test.working_materials(),
            temporaryRoot("WithInputs") / "WorkingCopy");

      check("Committed input copied", fs::exists(temporaryRoot("WithInputs") / "WorkingCopy" / "input.txt"));
      check(equality, "Scratchpad beside the materials", test.scratchpad_materials(), temporaryRoot("WithInputs"));

      check_exception_thrown<std::runtime_error>(
        "No predictions",
        [&test]() { return test.predictive_materials(); });

      check_exception_thrown<std::runtime_error>(
        "No auxiliary materials",
        [&test]() { return test.auxiliary_materials(); });
    }

    {
      const auto [test, materials]{preparedTest("WithPredictions")};

      check(equality,
            "Working copy of predictions alone",
            test.working_materials(),
            temporaryRoot("WithPredictions") / "WorkingCopy");

      check("Working copy made empty", fs::is_empty(test.working_materials()));
      check(equality, "Predictions", test.predictive_materials(), materials.original_materials_root() / "Prediction");
      check("Predictions not copied", !fs::exists(temporaryRoot("WithPredictions") / "Prediction"));
    }

    {
      const auto [test, materials]{preparedTest("WithAuxiliary")};

      const auto temporaryAuxiliary{temporaryRoot("WithAuxiliary") / "Auxiliary"};
      check(equality, "Auxiliary materials alone", test.auxiliary_materials(), temporaryAuxiliary);
      check("Committed auxiliary material copied", fs::exists(temporaryAuxiliary / "auxiliary.txt"));
    }

    {
      const auto [test, materials]{preparedTest("WithNone")};

      check_exception_thrown<std::runtime_error>("No materials", [&test]() { return test.working_materials(); });
      check("Scratchpad prepared empty", fs::is_empty(temporaryRoot("WithNone")));

      // Where the file lands is checked without asking the scratchpad where it is
      write_to_file(test.scratchpad_materials() / "scratch.txt", "", std::ios_base::out);
      check("Scratch file beneath the temporary data", fs::exists(temporaryRoot("WithNone") / "scratch.txt"));
      check("Scratch file not in the current directory", !fs::exists(fs::current_path() / "scratch.txt"));

      prepare_materials(materials);
      check("Scratchpad emptied by preparing again", fs::is_empty(temporaryRoot("WithNone")));
    }

    {
      const auto [test, materials]{preparedTest("WithInputs")};
      write_to_file(test.working_materials() / "input.txt", "Changed", std::ios_base::out);
      write_to_file(test.working_materials() / "extra.txt", "", std::ios_base::out);

      prepare_materials(materials);
      check(equivalence,
            "Working copy restored by preparing again",
            temporaryRoot("WithInputs") / "WorkingCopy",
            materials.original_working());
    }

    check_exception_thrown<std::runtime_error>(
      "Stray original materials",
      [&preparedTest]() { return preparedTest("WithStray"); });

    check_exception_thrown<std::logic_error>(
      "Preparing the materials of no test",
      []() { prepare_materials(individual_materials_paths{}); });

    {
      const fake_test test{
        "fake_test",
        projPaths.tests().repo() / "Materials/WithNone.cpp",
        projPaths,
        individual_materials_paths{},
        {},
        null_discriminator,
        null_discriminator
      };

      check_exception_thrown<std::logic_error>("Working copy of a test with no materials paths",
                                               [&test]() { return test.working_materials(); });
      check_exception_thrown<std::logic_error>("Predictions of a test with no materials paths",
                                               [&test]() { return test.predictive_materials(); });
      check_exception_thrown<std::logic_error>("Auxiliary materials of a test with no materials paths",
                                               [&test]() { return test.auxiliary_materials(); });
      check_exception_thrown<std::logic_error>("Scratchpad of a test with no materials paths",
                                               [&test]() { return test.scratchpad_materials(); });
    }
  }

  /** The original materials of a discriminated test hold one directory per discriminator: `Platypus`
      and `Echidna`. The discriminator must be a portable name for one directory. Nothing else may sit
      beside the directories.
   */
  void basic_test_interface_free_test::test_discriminated_materials(const project_paths& projPaths)
  {
    auto prepareMaterials{
      [&projPaths](std::string_view sourceStem, std::string discriminator) {
        const auto source{projPaths.tests().repo() / "Materials" / std::format("{}.cpp", sourceStem)};
        prepare_materials(individual_materials_paths{source, "fake_test", projPaths, std::move(discriminator)});
      }
    };

    prepareMaterials("Discriminated", "Platypus");
    const auto temporaryWorkingCopy{
      projPaths.output().tests_temporary_data() / "Materials/Discriminated/fake_test/WorkingCopy"
    };

    check(equality,
          "The declared discriminator's materials copied",
          read_to_string(temporaryWorkingCopy / "input.txt", std::ios_base::in).value_or(""),
          std::string{"Platypus\n"});

    for(const auto& [description, discriminator] : std::to_array<std::pair<std::string_view, std::string_view>>({
          {"An empty discriminator",                                ""},
          {"A discriminator naming the parent",                     ".."},
          {"An absolute discriminator",                             "/Platypus"},
          {"A discriminator holding a separator",                   "Platypus/Echidna"},
          {"A discriminator holding a colon",                       "Platypus:Echidna"},
          {"A discriminator ending in a dot",                       "Platypus."},
          {"A discriminator naming a Windows device",               "COM1"},
          {"A Windows device numbered with superscript one",        "COM\xC2\xB9"},
          {"A Windows device numbered with superscript two",        "Lpt\xC2\xB2"},
          {"A Windows device numbered with superscript three",      "lpt\xC2\xB3.txt"},
          {"A discriminator naming a kind of material",             "prediction"},
          {"A discriminator differing only in case from a sibling", "platypus"}}))
    {
      check_exception_thrown<std::runtime_error>(
        description,
        [&prepareMaterials, discriminator]() { prepareMaterials("Discriminated", std::string{discriminator}); });
    }

    {
      // Only a case-sensitive filesystem holds directories differing only in case, so the variants
      // listed depend on the filesystem. The sort is the implementation's choice, checked so that a
      // change to it shows.
      const individual_materials_paths materials{
        projPaths.tests().repo() / "Materials/CaseVariants.cpp", "fake_test", projPaths, "platypus"
      };

      const auto& root{materials.original_test_root()};
      for(const auto variant : {"Platypus", "PLATYPUS", "platyPUS", "PlatyPus", "pLATYPUS"})
        fs::create_directories(root / variant);

      const bool caseSensitive{!fs::equivalent(root / "PLATYPUS", root / "Platypus")};

      std::string message{};
      try
      {
        prepare_materials(materials);
      }
      catch(const std::runtime_error& e)
      {
        message = e.what();
      }

      check(equality,
            "Every case variant of the discriminator, sorted",
            message,
            std::format("The materials discriminator \"platypus\" must not differ only in case from {}",
                        caseSensitive ? "PLATYPUS, PlatyPus, Platypus, pLATYPUS, platyPUS" : "Platypus"));
    }

    check_exception_thrown<std::runtime_error>(
      "Materials beside the discriminated directories",
      [&prepareMaterials]() { prepareMaterials("DiscriminatedBeside", "Platypus"); });
  }
}
