////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "BasicTestInterfaceFreeTest.hpp"
#include "Parsing/CommandLineArgumentsTestingUtilities.hpp"
#include "Utilities/TestUtilities.hpp"

#include "sequoia/PlatformSpecific/Macros.hpp"
#include "sequoia/TestFramework/Macros.hpp"

import std;
import sequoia.test_framework;

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    class fake_test : public free_test {
    public:
      using free_test::free_test;
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
    test_discarded_materials(runner.proj_paths());
  }

  void basic_test_interface_free_test::test_file_paths(const project_paths& projPaths)
  {
    const auto rebasedSource{rebase_from(source_file(), get_project_paths().project_root())};

    {
      const fake_test t{test_name<fake_test>(), source_file(), projPaths, {}, {}, null_discriminator};

      check(equality,
            reporter{"Exceptions File Path"},
            t.diagnostics_file_paths().caught_exceptions_file_path(),
            projPaths.output().diagnostics() / rebasedSource.parent_path() / "fake_test_Exceptions.txt");
    }

    {
      const fake_test t{test_name<fake_test>(), source_file(), projPaths, {}, {}, {""}};

      check(equality,
            reporter{"Exceptions File Path"},
            t.diagnostics_file_paths().caught_exceptions_file_path(),
            projPaths.output().diagnostics() / rebasedSource.parent_path() / "fake_test_Exceptions.txt");
    }

    {
      const fake_test_with_discriminated_exceptions t{
        test_name<fake_test_with_discriminated_exceptions>(),
        source_file(),
        projPaths,
        {},
        {},
        {"baz"}
      };

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

    discarded_materials_remover remover{};

    const auto preparedTest{
      [&projPaths, &remover](std::string_view sourceStem) {
        const auto source{projPaths.tests().repo() / "Materials" / std::format("{}.cpp", sourceStem)};
        const individual_materials_paths materials{source, "fake_test", projPaths, null_discriminator};
        std::ignore = prepare_materials(materials, remover);
        return std::pair{fake_test{"fake_test", source, projPaths, materials, {}, null_discriminator}, materials};
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

      std::ignore = prepare_materials(materials, remover);
      check("Scratchpad emptied by preparing again", fs::is_empty(temporaryRoot("WithNone")));
    }

    {
      const auto [test, materials]{preparedTest("WithInputs")};
      write_to_file(test.working_materials() / "input.txt", "Changed", std::ios_base::out);
      write_to_file(test.working_materials() / "extra.txt", "", std::ios_base::out);

      std::ignore = prepare_materials(materials, remover);
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
      [&remover]() { return prepare_materials(individual_materials_paths{}, remover); });

    {
      const fake_test test{
        "fake_test",
        projPaths.tests().repo() / "Materials/WithNone.cpp",
        projPaths,
        individual_materials_paths{},
        {},
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
    discarded_materials_remover remover{};

    auto prepareMaterials{
      [&projPaths, &remover](std::string_view sourceStem, std::string discriminator) {
        const auto source{projPaths.tests().repo() / "Materials" / std::format("{}.cpp", sourceStem)};
        return prepare_materials(individual_materials_paths{source, "fake_test", projPaths, std::move(discriminator)},
                                 remover);
      }
    };

    std::ignore = prepareMaterials("Discriminated", "Platypus");
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
        std::ignore = prepare_materials(materials, remover);
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

  /** Each test here has no original materials, and paths of its own, so that
      no other preparation touches them.
      -# A remover which has been joined removes nothing, so the discarded root
         keeps the moved temporary root;
      -# `prepare_materials` removes a discarded root which an earlier run left
         behind, then moves the temporary root;
      -# A leftover which cannot be removed is the failure which the returned
         future holds, and the temporary root is removed in place;
      -# If the removal in place fails too, the error names both roots. Only
         its first line is checked, since the lines after it hold the
         platform's messages;
      -# With no temporary root, nothing is moved, and the future holds no
         failure;
      -# A directory which cannot be removed is a failure, which the future of
         its removal holds.
   */
  void basic_test_interface_free_test::test_discarded_materials(const project_paths& projPaths)
  {
    const auto source{projPaths.tests().repo() / "Materials/WithNone.cpp"};

    {
      const individual_materials_paths materials{source, "moved_test", projPaths, null_discriminator};
      fs::create_directories(materials.temporary_materials_root());
      write_to_file(materials.temporary_materials_root() / "Previous.txt", "", std::ios_base::out);

      discarded_materials_remover remover{};
      remover.join();

      const auto removalFailureFuture{prepare_materials(materials, remover)};
      // The remover has joined, so an enqueued removal never runs, whereas a
      // ready future would be one which prepare_materials made itself
      check("The moved temporary root's removal is enqueued",
            removalFailureFuture.wait_for(std::chrono::seconds{}) == std::future_status::timeout);
      check("The temporary root is moved to the discarded root",
            fs::exists(materials.discarded_materials_root() / "Previous.txt"));
      check("The fresh temporary root holds nothing of the moved one",
            fs::is_empty(materials.temporary_materials_root()));
    }

    {
      const individual_materials_paths materials{source, "leftover_test", projPaths, null_discriminator};
      fs::create_directories(materials.temporary_materials_root());
      fs::create_directories(materials.discarded_materials_root());
      write_to_file(materials.temporary_materials_root() / "Previous.txt", "", std::ios_base::out);
      write_to_file(materials.discarded_materials_root() / "Leftover.txt", "", std::ios_base::out);

      discarded_materials_remover remover{};
      remover.join();

      const auto removalFailureFuture{prepare_materials(materials, remover)};
      check("A leftover discarded root is removed", !fs::exists(materials.discarded_materials_root() / "Leftover.txt"));
      check("The temporary root is then moved", fs::exists(materials.discarded_materials_root() / "Previous.txt"));
    }

    {
      const individual_materials_paths materials{source, "unremovable_leftover_test", projPaths, null_discriminator};
      fs::create_directories(materials.temporary_materials_root());
      write_to_file(materials.temporary_materials_root() / "Previous.txt", "", std::ios_base::out);
      const unremovable_directory leftover{materials.discarded_materials_root()};

      discarded_materials_remover remover{};
      auto removalFailureFuture{prepare_materials(materials, remover)};

      // An invalid future's `get` is undefined, so a regression to one must
      // fail the check rather than crash the run
      const auto leftoverFailure{removalFailureFuture.valid() ? removalFailureFuture.get() : std::nullopt};
      check(equality,
            "A leftover which cannot be removed is the failure",
            leftoverFailure.transform([](const removal_failure& failure){ return failure.dir; }),
            std::optional{materials.discarded_materials_root()});

      check("The temporary root is then removed in place", fs::is_empty(materials.temporary_materials_root()));
    }

    {
      const individual_materials_paths materials{source, "unremovable_twice_test", projPaths, null_discriminator};
      const unremovable_directory leftover{materials.discarded_materials_root()},
                                  stuck{materials.temporary_materials_root() / "Stuck"};

      discarded_materials_remover remover{};
      check_exception_thrown<std::runtime_error>(
        "A removal in place which fails after the leftover's names both roots",
        [&materials, &remover]() { return prepare_materials(materials, remover); },
        [](const project_paths& paths, std::string message) {
          return default_exception_message_postprocessor{}(paths, message.substr(0, message.find('\n')));
        }
      );
    }

    {
      const individual_materials_paths materials{source, "unprepared_test", projPaths, null_discriminator};
      fs::remove_all(materials.temporary_materials_root());

      discarded_materials_remover remover{};
      auto removalFailureFuture{prepare_materials(materials, remover)};
      check("With no temporary root, the future holds no failure",
            removalFailureFuture.valid() && !removalFailureFuture.get());
    }

    {
      const unremovable_directory unremovable{projPaths.output().tests_temporary_data() / "Unremovable"};

      discarded_materials_remover remover{};
      auto removalFailureFuture{remover.enqueue_removal(unremovable.path())};

      check(equality,
            "The directory which could not be removed is the failure",
            removalFailureFuture.get().transform([](const removal_failure& failure){ return failure.dir; }),
            std::optional{unremovable.path()});
    }
  }
}
