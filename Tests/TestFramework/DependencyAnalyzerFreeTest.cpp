////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "DependencyAnalyzerFreeTest.hpp"
#include "Parsing/CommandLineArgumentsTestingUtilities.hpp"

#include "TestFramework/BuildArtefactsTestingUtilities.hpp"
#include "sequoia/TestFramework/StateTransitionUtilities.hpp"
#include "Utilities/TestUtilities.hpp"
#include "sequoia/Streaming/Streaming.hpp"
#include "sequoia/TextProcessing/Patterns.hpp"
#include "sequoia/TextProcessing/Substitutions.hpp"
#include "sequoia/TestFramework/ChronoCheckers.hpp"
#include "sequoia/TestFramework/SumTypeCheckers.hpp"

#include <fstream>
#include <stdexcept>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    /* A timeline, in seconds either side of m_ResetTime, at which the fake project's files are
       stamped. Every unmodified file sits at the reset time and every modification after the prune
       stamp, so the ordering is what each check is really about.

       The gap either side of the stamp is two seconds because a coarse filesystem forces it to be.
       Where `last_write_time` is truncated to whole seconds - libstdc++ does this on macOS - a
       modification made in the same second as the stamp is indistinguishable from one made during
       the run which wrote it, and `staleness_threshold` resolves that ambiguity by moving the
       threshold a second earlier. So an unmodified file must be a clear second below the threshold,
       and a modification a clear second above it. The boundary itself - a file whose stamp equals
       the threshold exactly - is deliberately not asserted on here: its answer is a property of the
       filesystem rather than of the analyzer.
    */
    constexpr auto earlyExecutableOffset{std::chrono::seconds{-1}};
    constexpr auto resetOffset{std::chrono::seconds{0}};
    constexpr auto pruneStampOffset{std::chrono::seconds{2}};
    constexpr auto earlyPassOffset{std::chrono::seconds{3}}; // very_early
    constexpr auto earlyEditOffset{std::chrono::seconds{4}};  // early
    constexpr auto lateExecutableOffset{std::chrono::seconds{5}};
    constexpr auto latePassOffset{std::chrono::seconds{6}};   // late
    constexpr auto lateEditOffset{std::chrono::seconds{7}};   // very_late

    [[nodiscard]]
    constexpr fs::file_time_type stamp_at(std::chrono::milliseconds sinceEpoch)
    {
      return fs::file_time_type{} + std::chrono::duration_cast<fs::file_time_type::duration>(sinceEpoch);
    }

    enum node_names : std::size_t
    {
      null_fails_null_passes,
      empty_fails_null_passes,
      empty_fails_empty_passes,
      house_fails_null_passes,
      house_fails_empty_passes,
      empty_fails_house_passes,
      house_prob_fails_null_passes,
      house_prob_fails_empty_passes,
      house_fails_prob_passes,
      house_prob_maybe_fails_empty_passes,
      house_prob_fails_maybe_passes,
      house_fails_maybe_prob_passes,
      empty_fails_maybe_house_prob_passes,
      house_fails_late_empty_passes,
      house_prob_fails_late_empty_passes,
    };
  }

  dependency_analyzer_free_test::test_outcomes::test_outcomes(opt_prune_records fail, opt_prune_records pass)
    : failures{std::move(fail)}
    , passes{std::move(pass)}
  {
    if(failures) std::ranges::sort(*failures);
    if(passes)   std::ranges::sort(*passes);
  }

  [[nodiscard]]
  std::chrono::seconds dependency_analyzer_free_test::to_duration(modification_time modTime)
  {
    using enum modification_time;
    switch(modTime)
    {
    case very_early:
      return earlyPassOffset;
    case early:
      return earlyEditOffset;
    case late:
      return latePassOffset;
    case very_late:
      return lateEditOffset;
    }

    throw std::logic_error{"Unrecognized option for modification_time"};
  }

  auto dependency_analyzer_free_test::read(const fs::path& file) -> opt_prune_records
  {
    if(fs::exists(file)) return read_tests(file);

    return std::nullopt;
  }

  void dependency_analyzer_free_test::write_or_remove(const project_paths& projPaths, const fs::path& file, const opt_prune_records& tests)
  {
    if(tests) write_tests(projPaths, file, tests.value());
    else      fs::remove(file);
  }

  void dependency_analyzer_free_test::write_or_remove(const project_paths& projPaths, const fs::path& failureFile, const fs::path& passesFile, const test_outcomes& d)
  {
    write_or_remove(projPaths, failureFile, d.failures);
    write_or_remove(projPaths, passesFile , d.passes);
  }

  [[nodiscard]]
  std::filesystem::path dependency_analyzer_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void dependency_analyzer_free_test::check_tests_to_run(const reporter& description,
                                                         const project_paths& projPaths,
                                                         const file_states& fileStates,
                                                         std::vector<prune_record> failures,
                                                         std::vector<prune_record> passes)
  {
    std::ranges::sort(failures);
    std::ranges::sort(passes);

    const auto prune{projPaths.prune()};
    const auto failureFile{prune.failures(std::nullopt)};
    const auto passesFile{prune.selected_passes(std::nullopt)};
    write_tests(projPaths, failureFile, failures);
    write_tests(projPaths, passesFile, passes);

    if(!passes.empty())
      fs::last_write_time(passesFile, std::ranges::max(passes, {}, [](const prune_record& r){ return r.time_stamp; }).time_stamp);

    for(const auto& f : fileStates.stale)
    {
      fs::last_write_time(f.file, m_ResetTime + to_duration(f.modification));
    }

    opt_test_list prediction{fileStates.to_run};
    std::ranges::sort(*prediction);

    check(equality, description, (tests_to_run(projPaths)), prediction);

    for(const auto& f : fileStates.stale)
    {
      fs::last_write_time(f.file, m_ResetTime);
    }

    fs::remove(failureFile);
    fs::remove(passesFile);

    check(equality, append_lines(description.message(), "Nothing Stale"), (tests_to_run(projPaths)), opt_test_list{test_list{}});
  }

  /* The fake project is never built, so what its build would have recorded is written by hand:
     for each unit, every file the compiler would have read, which is the flattened closure of its
     includes. The sequoia headers the fake sources include lie outside the fake project, as they
     would in a build of it, and are taken from the real one so that they exist.
   */
  void dependency_analyzer_free_test::write_build_artefacts(const fs::path& fake, build_system system)
  {
    struct unit
    {
      std::string_view source;
      std::vector<std::string_view> inputs;
      std::vector<std::string_view> sequoia_inputs{};
    };

    constexpr std::string_view
      freeTestCore{"sequoia/TestFramework/FreeTestCore.hpp"},
      regularTestCore{"sequoia/TestFramework/RegularTestCore.hpp"},
      moveOnlyTestCore{"sequoia/TestFramework/MoveOnlyTestCore.hpp"};

    const std::vector<unit> units{
      {"Source/fakeProject/Maths/Helper.cpp", {"Source/fakeProject/Maths/Helper.hpp"}},
      {"Source/fakeProject/Maths/Probability.cpp", {"Source/fakeProject/Maths/Probability.hpp", "Source/fakeProject/Maths/Helper.hpp", "dependencies/foo/Source/foo/Utilities/Helper.hpp"}},
      {"Source/fakeProject/Utilities/Thing/UniqueThing.cpp", {"Source/fakeProject/Utilities/Thing/UniqueThing.hpp"}},
      {"Source/fakeProject/Utilities/UsefulThings.cpp", {"Source/fakeProject/Utilities/UsefulThings.hpp", "dependencies/foo/Source/foo/Utilities/Helper.hpp"}},
      {"TestUtilities/myLib/Utils.cpp", {"TestUtilities/myLib/Utils.hpp"}},
      {"dependencies/foo/Source/foo/Utilities/Helper.cpp", {"dependencies/foo/Source/foo/Utilities/Helper.hpp"}},
      {"Tests/Cycle/FirstFreeTest.cpp", {"Tests/Cycle/FirstFreeTest.hpp", "Source/fakeProject/Cycle/First.hpp", "Source/fakeProject/Cycle/Second.hpp", "Source/fakeProject/Cycle/FirstLeaf.hpp", "Source/fakeProject/Cycle/SecondLeaf.hpp"}, {freeTestCore}},
      {"Tests/Cycle/SecondFreeTest.cpp", {"Tests/Cycle/SecondFreeTest.hpp", "Source/fakeProject/Cycle/Second.hpp", "Source/fakeProject/Cycle/First.hpp", "Source/fakeProject/Cycle/SecondLeaf.hpp", "Source/fakeProject/Cycle/FirstLeaf.hpp"}, {freeTestCore}},
      {"Tests/HouseAllocationTest.cpp", {"Tests/HouseAllocationTest.hpp"}, {"sequoia/TestFramework/MoveOnlyAllocationTestCore.hpp"}},
      {"Tests/Maths/ProbabilityTest.cpp", {"Tests/Maths/ProbabilityTest.hpp", "Tests/Maths/ProbabilityTestingUtilities.hpp", "Source/fakeProject/Maths/Probability.hpp"}, {regularTestCore}},
      {"Tests/Maths/ProbabilityTestingDiagnostics.cpp", {"Tests/Maths/ProbabilityTestingDiagnostics.hpp", "Tests/Maths/ProbabilityTestingUtilities.hpp", "Source/fakeProject/Maths/Probability.hpp"}, {regularTestCore}},
      {"Tests/Maybe/MaybeTest.cpp", {"Tests/Maybe/MaybeTest.hpp", "Tests/Maybe/MaybeTestingUtilities.hpp", "Tests/Stuff/OldschoolTestingUtilities.hpp", "Source/fakeProject/Stuff/NoTemplate.hpp", "TestUtilities/myLib/Utils.hpp", "Source/fakeProject/Maybe/Maybe.hpp"}, {regularTestCore}},
      {"Tests/Maybe/MaybeTestingDiagnostics.cpp", {"Tests/Maybe/MaybeTestingDiagnostics.hpp", "Tests/Maybe/MaybeTestingUtilities.hpp", "Source/fakeProject/Maybe/Maybe.hpp"}, {regularTestCore}},
      {"Tests/Stuff/BarFreeTest.cpp", {"Tests/Stuff/BarFreeTest.hpp", "Source/fakeProject/Stuff/Bar.hpp", "Source/fakeProject/Stuff/Baz.hpp", "Source/fakeProject/Stuff/Qux.hpp"}, {freeTestCore}},
      {"Tests/Stuff/FooTest.cpp", {"Tests/Stuff/FooTest.hpp", "Tests/Stuff/FooTestingUtilities.hpp", "Source/fakeProject/Stuff/Foo.hpp"}, {"sequoia/TestFramework/FileEditors.hpp", moveOnlyTestCore, "sequoia/TextProcessing/Substitutions.hpp"}},
      {"Tests/Stuff/FooTestingDiagnostics.cpp", {"Tests/Stuff/FooTestingDiagnostics.hpp", "Tests/Stuff/FooTestingUtilities.hpp", "Source/fakeProject/Stuff/Foo.hpp"}, {moveOnlyTestCore}},
      {"Tests/Stuff/OldschoolTest.cpp", {"Tests/Stuff/OldschoolTest.hpp", "Tests/Stuff/OldschoolTestingUtilities.hpp", "Source/fakeProject/Stuff/NoTemplate.hpp", "TestUtilities/myLib/Utils.hpp"}, {regularTestCore}},
      {"Tests/Stuff/OldschoolTestingDiagnostics.cpp", {"Tests/Stuff/OldschoolTestingDiagnostics.hpp", "Tests/Stuff/OldschoolTestingUtilities.hpp", "Source/fakeProject/Stuff/NoTemplate.hpp", "TestUtilities/myLib/Utils.hpp"}, {regularTestCore}},
      {"Tests/Utilities/ContainerAllocationTest.cpp", {"Tests/Utilities/ContainerAllocationTest.hpp"}, {"sequoia/TestFramework/RegularAllocationTestCore.hpp"}},
      {"Tests/Utilities/ContainerPerformanceTest.cpp", {"Tests/Utilities/ContainerPerformanceTest.hpp", "Source/fakeProject/Utilities/Container.hpp"}, {"sequoia/TestFramework/PerformanceTestCore.hpp"}},
      {"Tests/Utilities/Thing/UniqueThingTest.cpp", {"Tests/Utilities/Thing/UniqueThingTest.hpp", "Tests/Utilities/Thing/UniqueThingTestingUtilities.hpp", "Source/fakeProject/Utilities/Thing/UniqueThing.hpp", "Tests/Stuff/FooTestingUtilities.hpp", "Source/fakeProject/Stuff/Foo.hpp"}, {moveOnlyTestCore}},
      {"Tests/Utilities/Thing/UniqueThingTestingDiagnostics.cpp", {"Tests/Utilities/Thing/UniqueThingTestingDiagnostics.hpp", "Tests/Utilities/Thing/UniqueThingTestingUtilities.hpp", "Source/fakeProject/Utilities/Thing/UniqueThing.hpp", "Tests/Stuff/FooTestingUtilities.hpp", "Source/fakeProject/Stuff/Foo.hpp"}, {moveOnlyTestCore}},
      {"Tests/Utilities/UsefulThingsFreeTest.cpp", {"Tests/Utilities/UsefulThingsFreeTest.hpp", "Source/fakeProject/Utilities/UsefulThings.hpp"}, {freeTestCore}},
      {"Tests/Utilities/UtilitiesFreeTest.cpp", {"Tests/Utilities/UtilitiesFreeTest.hpp", "Source/fakeProject/Utilities/Utilities.hpp"}, {freeTestCore}}
    };

    const auto buildDir{fake / "build" / "CMade" / "TestAll"};
    const auto objectDir{fs::path{"CMakeFiles"} / "TestAll.dir"};
    const bool ninja{system != build_system::visual_studio};
    auto object{[&](std::string_view source){ return objectDir / (std::string{source} + (ninja ? ".o" : ".obj")); }};

    const auto& sequoiaSource{get_project_paths().source().repo()};

    std::vector<compilation_record> records{};
    for(const auto& [source, inputs, sequoia_inputs] : units)
    {
      compilation_record record{.object{object(source)}, .inputs{fake / source}};
      for(const auto& input : inputs) record.inputs.push_back(fake / input);
      for(const auto& input : sequoia_inputs) record.inputs.push_back(sequoiaSource / input);
      records.push_back(std::move(record));
    }

    // What the build tree says of itself; the fake project's source dir is itself, and one sequoia directory stands in for the toolchain's
    fs::remove_all(buildDir / "CMakeFiles");
    fs::remove(buildDir / ".ninja_deps");
    fs::remove(buildDir / "build.ninja");
    fs::create_directories(buildDir / objectDir);
    fs::create_directories(buildDir / "CMakeFiles" / "4.1.2");
    write_to_file(buildDir / "CMakeCache.txt",
                  std::format("# Fake\nCMAKE_GENERATOR:INTERNAL={}\nCMAKE_HOME_DIRECTORY:INTERNAL={}\n", ninja ? "Ninja" : "Visual Studio 18 2026", fake.generic_string()),
                  std::ios_base::out);
    write_to_file(buildDir / "CMakeFiles" / "4.1.2" / "CMakeCXXCompiler.cmake",
                  std::format("set(CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES \"{}\")\n", (sequoiaSource / "sequoia" / "TextProcessing").generic_string()),
                  std::ios_base::out);

    if(ninja)
    {
      // The generator spells paths natively and escapes ninja's specials - on Windows `C$:\Users\...` - where the log, which ninja canonicalizes, is generic everywhere
      auto asWritten{[](fs::path p){ return replace_all(p.make_preferred().string(), ":", "$:"); }};

      // What the build currently has: a statement per object naming its source
      std::string statements{};
      for(const auto& record : records)
      {
        statements.append("build ").append(asWritten(record.object)).append(": CXX_COMPILER ").append(asWritten(record.inputs.front())).append(" || cmake_object_order_depends\n");
      }

      // An object the build once had and no longer does keeps its record in the log, and its source may be gone
      statements.append("build CMakeFiles/TestAll.dir/unrelated.o: CXX_COMPILER unrelated.cpp\n");
      auto logged{records};
      logged.push_back({.object{objectDir / "Tests/Retired/RetiredTest.cpp.o"}, .inputs{fake / "Tests/Retired/RetiredTest.cpp", fake / "Tests/Retired/Gone.hpp"}});

      // MSVC reports the headers it read but not the source, which the statement supplies
      if(system == build_system::ninja_with_msvc)
      {
        for(auto& record : logged) record.inputs.erase(record.inputs.begin());
      }

      write_to_file(buildDir / "build.ninja", statements, std::ios_base::out);
      write_ninja_deps(buildDir / ".ninja_deps", logged);
    }
    else
    {
      // The tracker spells paths in upper case and the reader recovers their case from the filesystem, so what it
      // wrote must exist; the logs themselves live beside the objects, under the configuration - here the
      // executable's own directory name
      for(auto& record : records)
      {
        record.object = buildDir / record.object;

        fs::create_directories(record.object.parent_path());
        write_to_file(record.object, "", std::ios_base::out);
      }

      write_tlogs(buildDir / objectDir / "TestAll" / "TestAll.tlog", records);
    }
  }

  void dependency_analyzer_free_test::run_tests()
  {
    test_staleness_threshold();

    m_ResetTime = std::chrono::file_clock::now() + resetOffset;

    const auto fake{auxiliary_materials() /= "FakeProject"};
    write_build_artefacts(fake, build_system::ninja);

    const main_paths main{fake / main_paths::default_main_cpp_from_root()};
    commandline_arguments args{{(fake / "build/CMade/TestAll/TestAll").generic_string()}};
    const project_paths projPaths{args.size(), args.get(), {.main_cpp{main.file()}, .common_includes{main.file()}}};

    check(equality, "No timestamp", (tests_to_run(projPaths)), opt_test_list{});

    const auto prunePaths{projPaths.prune()};
    fs::create_directories(prunePaths.dir());
    { std::ofstream s{prunePaths.stamp()}; }

    for(auto& entry : fs::recursive_directory_iterator(fake))
    {
      fs::last_write_time(entry.path(), m_ResetTime);
    }

    // The loop above reaches the stamp too, since the prune directory is inside the fake project;
    // set it afterwards so that it, rather than the reset time, marks the start of the run.
    fs::last_write_time(prunePaths.stamp(), m_ResetTime + pruneStampOffset);

    test_exceptions(projPaths);
    test_dependencies(projPaths);

    // The same build, as ninja records it when the compiler is MSVC, and as Visual Studio's tracker would have recorded it
    write_build_artefacts(fake, build_system::ninja_with_msvc);
    test_dependencies(projPaths);

    write_build_artefacts(fake, build_system::visual_studio);
    test_dependencies(projPaths);

    write_build_artefacts(fake, build_system::ninja);
    test_stamp_on_second_boundary(projPaths);
    test_pass_recorded_in_the_modification_second(projPaths);
    test_prune_record_round_trip(projPaths);
    test_prune_update(projPaths);
    test_instability_analysis_prune_upate(projPaths);
  }

  void dependency_analyzer_free_test::test_staleness_threshold()
  {
    using namespace std::chrono_literals;

    check(equality,
          "A stamp on a second boundary cannot be told from a truncated one, so it is widened",
          staleness_threshold(stamp_at(0ms)),
          stamp_at(-1000ms));

    check(equality,
          "A stamp carrying sub-second information is its own threshold",
          staleness_threshold(stamp_at(1500ms)),
          stamp_at(1500ms));

    check(equality,
          "Sub-second information before the clock's epoch, where a truncation towards zero would lose it",
          staleness_threshold(stamp_at(-1500ms)),
          stamp_at(-1500ms));

    check(equality,
          "A whole number of seconds before the clock's epoch is still on a boundary",
          staleness_threshold(stamp_at(-2000ms)),
          stamp_at(-3000ms));
  }

  void dependency_analyzer_free_test::test_exceptions(const project_paths& projPaths)
  {
    check_exception_thrown<std::runtime_error>(
      "Executable out of date",
      [this, projPaths]() {
        fs::last_write_time(projPaths.executable(), m_ResetTime + earlyExecutableOffset);
        return tests_to_run(projPaths);
      },
      [](const project_paths& paths, std::string message) {
        message = default_exception_message_postprocessor{}(paths, std::move(message));
        {
          const auto [first, last]{find_sandwiched_text(message, "FakeProject", "time")};
          if(first < last)
            message.replace(first, last - first, "/xxFILExx ");
        }

        std::string::size_type pos{};
        while(pos < message.size())
        {
          std::string_view remaining{message.data() + pos, message.size() - pos};
          const auto [first, last]{find_sandwiched_text(remaining, ":", "\n")};
          if(first >= last)
            break;

          pos += first;
          message.replace(pos, last - first, "****");
          pos += 4;
        }

        return message;
      }
    );
  }

  void dependency_analyzer_free_test::test_dependencies(const project_paths& projPaths)
  {
    fs::last_write_time(projPaths.executable(), m_ResetTime + lateExecutableOffset);

    const auto& testRepo{projPaths.tests().repo()};
    const auto& sourceRepo{projPaths.source().project()};
    const auto testUtilsPath{projPaths.project_root() / "TestUtilities"};
    const auto fooPath{projPaths.project_root() / "dependencies" / "foo" / "Source"};
    const auto& materials{projPaths.test_materials().repo()};

    check_tests_to_run("Nothing stale", projPaths, {}, {}, {});

    {
      // The report names files outside the fake project by absolute path, so the sequoia root is normalised away before comparison
      const auto report{read_to_string(projPaths.prune().external_dependencies(), std::ios_base::in)};
      if(!report)
        throw std::runtime_error{"Unable to read the external dependencies report"};

      const auto sequoiaSource{get_project_paths().source().repo().generic_string()};
      std::string normalised{report.value()};
      for(auto pos{normalised.find(sequoiaSource)}; pos != std::string::npos; pos = normalised.find(sequoiaSource, pos))
        normalised.replace(pos, sequoiaSource.size(), "<sequoia source>");

      write_to_file(working_materials() / "TestAll.external", normalised, std::ios_base::out);
      check(weak_equivalence, "External Dependencies", working_materials(), predictive_materials());
    }

    check_tests_to_run("Test cpp stale",
                       projPaths,
                       {.stale{{{testRepo / "HouseAllocationTest.cpp"}, modification_time::early}}, .to_run{{"HouseAllocationTest.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Test cpp naively stale, but has passed (when selected)",
                       projPaths,
                       {.stale{{{testRepo / "HouseAllocationTest.cpp"}, modification_time::early}}, .to_run{}},
                       {},
                       {{"HouseAllocationTest.cpp", m_ResetTime + to_duration(modification_time::late)}});

    check_tests_to_run("Test cpp stale; has previously passed (when selected), but this should be ignored",
                       projPaths,
                       {.stale{{{testRepo / "HouseAllocationTest.cpp"}, modification_time::early}}, .to_run{{"HouseAllocationTest.cpp"}}},
                       {},
                       {{"HouseAllocationTest.cpp", m_ResetTime + to_duration(modification_time::very_early)}});

    check_tests_to_run("Test hpp stale",
                       projPaths,
                       {.stale{{{testRepo / "HouseAllocationTest.hpp"}, modification_time::early}}, .to_run{{"HouseAllocationTest.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Test utils stale",
                       projPaths,
                       {.stale{{{testRepo / "Maths" / "ProbabilityTestingUtilities.hpp"}, modification_time::early}},
                         .to_run{{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Reused utils stale",
                       projPaths,
                       {.stale{{{testRepo / "Stuff" / "OldschoolTestingUtilities.hpp"}, modification_time::early}},
                         .to_run{{"Maybe/MaybeTest.cpp"}, {"Stuff/OldschoolTest.cpp"}, {"Stuff/OldschoolTestingDiagnostics.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Reused utils stale, but one of the tests has passed",
                       projPaths,
                       {.stale{{{testRepo / "Stuff" / "OldschoolTestingUtilities.hpp"}, modification_time::early}},
                         .to_run{{"Stuff/OldschoolTest.cpp"}, {"Stuff/OldschoolTestingDiagnostics.cpp"}}},
                       {},
                       {{"Maybe/MaybeTest.cpp", m_ResetTime + to_duration(modification_time::late)}});

    check_tests_to_run("Reused utils stale, but two of the tests have passed",
                       projPaths,
                       {.stale{{{testRepo / "Stuff" / "OldschoolTestingUtilities.hpp"}, modification_time::early}},
                         .to_run{{"Stuff/OldschoolTestingDiagnostics.cpp"}}},
                       {},
                       {{"Maybe/MaybeTest.cpp"    , m_ResetTime + to_duration(modification_time::late)},
                        {"Stuff/OldschoolTest.cpp", m_ResetTime + to_duration(modification_time::late)}});

    check_tests_to_run("Reused utils stale, but two of the tests have passed and a different one has failed",
                       projPaths,
                       {.stale{{{testRepo / "Stuff" / "OldschoolTestingUtilities.hpp"}, modification_time::early}},
                         .to_run{{"Stuff/OldschoolTestingDiagnostics.cpp"}, {"HouseAllocationTest.cpp"}}},
                       {{"HouseAllocationTest.cpp" , m_ResetTime + to_duration(modification_time::late)}},
                       {{"Maybe/MaybeTest.cpp"    , m_ResetTime + to_duration(modification_time::late)},
                        {"Stuff/OldschoolTest.cpp", m_ResetTime + to_duration(modification_time::late)}});

    check_tests_to_run("Reused utils stale, relative path",
                       projPaths,
                       {.stale{{{testRepo / "Stuff" / "FooTestingUtilities.hpp"}, modification_time::early}},
                         .to_run{{"Stuff/FooTest.cpp"}, {"Stuff/FooTestingDiagnostics.cpp"}, {"Utilities/Thing/UniqueThingTest.cpp"}, {"Utilities/Thing/UniqueThingTestingDiagnostics.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Source cpp stale",
                       projPaths,
                       {.stale{{{sourceRepo / "Maths" / "Probability.cpp"}, modification_time::early}},
                         .to_run{{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Source hpp stale",
                       projPaths,
                       {.stale{{{sourceRepo / "Maths" / "Probability.hpp"}, modification_time::early}},
                         .to_run{{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Source hpp stale, following a previously successful run",
                       projPaths,
                       {.stale{{{sourceRepo / "Maths" / "Probability.hpp"}, modification_time::early}},
                         .to_run{{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}}},
                       {},
                       {{"Maths/ProbabilityTest.cpp"              , m_ResetTime + to_duration(modification_time::very_early)},
                        {"Maths/ProbabilityTestingDiagnostics.cpp", m_ResetTime + to_duration(modification_time::very_early)}});

    check_tests_to_run("Source cpp stale, following a previously successful run",
                       projPaths,
                       {.stale{{{sourceRepo / "Maths" / "Probability.cpp"}, modification_time::early}},
                         .to_run{{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}}},
                       {},
                       {{"Maths/ProbabilityTest.cpp"              , m_ResetTime + to_duration(modification_time::very_early)},
                        {"Maths/ProbabilityTestingDiagnostics.cpp", m_ResetTime + to_duration(modification_time::very_early)}});

    check_tests_to_run("Source cpp indirectly stale via included header",
                       projPaths,
                       {.stale{{{sourceRepo / "Maths" / "Helper.hpp"}, modification_time::early}},
                         .to_run{{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Source hpp stale, reached by one test alone",
                       projPaths,
                       {.stale{{{sourceRepo / "Stuff" / "Baz.hpp"}, modification_time::early}},
                         .to_run{{"Stuff/BarFreeTest.cpp"}}},
                       {},
                       {});

    /* Cycle/First.hpp and Cycle/Second.hpp include one another. Each carries a test and includes a
       leaf of its own, so a staleness which propagates round the cycle in one direction only is
       caught whichever member the traversal enters by: one leaf or the other lies beyond the member
       which folds in its neighbour too soon.
    */
    const auto cycleTests{test_list{{"Cycle/FirstFreeTest.cpp"}, {"Cycle/SecondFreeTest.cpp"}}};

    check_tests_to_run("The leaf beyond First.hpp is stale",
                       projPaths,
                       {.stale{{{sourceRepo / "Cycle" / "FirstLeaf.hpp"}, modification_time::early}},
                         .to_run{cycleTests}},
                       {},
                       {});

    check_tests_to_run("The leaf beyond Second.hpp is stale",
                       projPaths,
                       {.stale{{{sourceRepo / "Cycle" / "SecondLeaf.hpp"}, modification_time::early}},
                         .to_run{cycleTests}},
                       {},
                       {});

    check_tests_to_run("One member of the cycle is stale",
                       projPaths,
                       {.stale{{{sourceRepo / "Cycle" / "First.hpp"}, modification_time::early}},
                         .to_run{cycleTests}},
                       {},
                       {});

    check_tests_to_run("The other member of the cycle is stale",
                       projPaths,
                       {.stale{{{sourceRepo / "Cycle" / "Second.hpp"}, modification_time::early}},
                         .to_run{cycleTests}},
                       {},
                       {});

    // The fold also carries the modification time round the cycle, which is what decides whether a
    // recorded pass post-dates the newest change.
    check_tests_to_run("The leaf beyond First.hpp is stale, following a previously successful run",
                       projPaths,
                       {.stale{{{sourceRepo / "Cycle" / "FirstLeaf.hpp"}, modification_time::early}},
                         .to_run{cycleTests}},
                       {},
                       {{"Cycle/SecondFreeTest.cpp", m_ResetTime + to_duration(modification_time::very_early)}});

    check_tests_to_run("The leaf beyond Second.hpp is stale, following a previously successful run",
                       projPaths,
                       {.stale{{{sourceRepo / "Cycle" / "SecondLeaf.hpp"}, modification_time::early}},
                         .to_run{cycleTests}},
                       {},
                       {{"Cycle/FirstFreeTest.cpp", m_ResetTime + to_duration(modification_time::very_early)}});

    check_tests_to_run("Source cpp indirectly stale via cpp definitions for included header",
                       projPaths,
                       {.stale{{{sourceRepo / "Maths" / "Helper.cpp"}, modification_time::early}},
                         .to_run{{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Source cpps indirectly stale via cpp from dependencies with the same name as a project cpp",
                       projPaths,
                       {.stale{{{fooPath / "foo" / "Utilities" / "Helper.cpp"}, modification_time::early}},
                         .to_run{{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}, {"Utilities/UsefulThingsFreeTest.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Stale header in additional project",
                       projPaths,
                       {.stale{{{testUtilsPath / "myLib" / "Utils.hpp"}, modification_time::early}},
                        .to_run{{"Maybe/MaybeTest.cpp"}, {"Stuff/OldschoolTest.cpp"}, {"Stuff/OldschoolTestingDiagnostics.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Stale cpp in additional project",
                       projPaths,
                       {.stale{{{testUtilsPath / "myLib" / "Utils.cpp"}, modification_time::early}},
                        .to_run{{"Maybe/MaybeTest.cpp"}, {"Stuff/OldschoolTest.cpp"}, {"Stuff/OldschoolTestingDiagnostics.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Materials stale",
                       projPaths,
                       {.stale{{{materials / "Stuff" / "FooTest" / "Prediction" / "RepresentativeCasesTemp" / "NoSeqpat" / "baz.txt"}, modification_time::early}},
                         .to_run{{"Stuff/FooTest.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Materials naively stale, but test previously passed (when selected)",
                       projPaths,
                       {.stale{{{materials / "Stuff" / "FooTest" / "Prediction" / "RepresentativeCasesTemp" / "NoSeqpat" / "baz.txt"}, modification_time::early}},
                         .to_run{}},
                       {},
                       {{"Stuff/FooTest.cpp", m_ResetTime + to_duration(modification_time::late)}});

    check_tests_to_run("Materials stale; test previously passed (when selected), but materials subsequently modified",
                       projPaths,
                       {.stale{{{materials / "Stuff" / "FooTest" / "Prediction" / "RepresentativeCasesTemp" / "NoSeqpat" / "baz.txt"}, modification_time::early}},
                         .to_run{{"Stuff/FooTest.cpp"}}},
                       {},
                       {{"Stuff/FooTest.cpp", m_ResetTime + to_duration(modification_time::very_early)}});

    check_tests_to_run("Materials stale; test previously passed (when selected); materials subsequently modified some early some late",
                       projPaths,
                       {.stale{{{materials / "Stuff" / "FooTest" / "Prediction" / "RepresentativeCasesTemp" / "NoSeqpat" / "baz.txt"}, modification_time::early},
                                {{materials / "Stuff" / "FooTest" / "Prediction" / "RepresentativeCasesTemp" / "NoSeqpat" / "baz2.txt"}, modification_time::very_late}},
                         .to_run{{"Stuff/FooTest.cpp"}}},
                       {},
                       {{"Stuff/FooTest.cpp", m_ResetTime + to_duration(modification_time::late)}});

    check_tests_to_run("Nothing stale, but a previous failure",
                       projPaths,
                       {.stale{}, .to_run{{"Maths/ProbabilityTest.cpp"}}},
                       {{"Maths/ProbabilityTest.cpp", m_ResetTime + to_duration(modification_time::early)}},
                       {});

    check_tests_to_run("Inconsistency: both passed and failed; failure wins",
                       projPaths,
                       {.stale{}, .to_run{{"Maths/ProbabilityTest.cpp"}}},
                       {{"Maths/ProbabilityTest.cpp" , m_ResetTime + to_duration(modification_time::late)}},
                       {{"Maths/ProbabilityTest.cpp", m_ResetTime + to_duration(modification_time::late)}});

    check_tests_to_run("Stale and a previous failure",
                       projPaths,
                       {.stale{{{testRepo / "Maths/ProbabilityTest.cpp"}, modification_time::early}}, .to_run{{"Maths/ProbabilityTest.cpp"}}},
                       {{"Maths/ProbabilityTest.cpp", m_ResetTime + to_duration(modification_time::late)}},
                       {});

    check_tests_to_run("Nothing stale, but two previous failures",
                       projPaths,
                       {.stale{}, .to_run{{"HouseAllocationTest.cpp"}, {"Maths/ProbabilityTest.cpp"}}},
                       {{"HouseAllocationTest.cpp"  , m_ResetTime + to_duration(modification_time::late)},
                        {"Maths/ProbabilityTest.cpp", m_ResetTime + to_duration(modification_time::late)}},
                       {});

    check_tests_to_run("Ensure that the staleness of a cpp isn't masked by a cpp which has freshly passed",
                       projPaths,
                       {
                         .stale{{{testRepo / "HouseAllocationTest.cpp"}  , modification_time::early},
                                {{testRepo / "Maths/ProbabilityTest.cpp"}, modification_time::early}},
                         .to_run{{"HouseAllocationTest.cpp"}}
                       },
                       {},
                       {{"HouseAllocationTest.cpp"  , m_ResetTime + to_duration(modification_time::very_early)},
                        {"Maths/ProbabilityTest.cpp", m_ResetTime + to_duration(modification_time::late)}});

  }

  void dependency_analyzer_free_test::test_stamp_on_second_boundary(const project_paths& projPaths)
  {
    /* A stamp which sits on a second boundary cannot be told from one a truncating filesystem
       produced, so a modification recorded at exactly that time must be treated as stale. This
       holds whatever the filesystem's resolution actually is, since the stamp is written on the
       boundary either way - which is what makes it the one check that observes
       `staleness_threshold` through the analyzer rather than calling it directly. Without it,
       removing the widening from `tests_to_run` would leave every check here passing.
    */
    using namespace std::chrono;
    const fs::file_time_type boundary{floor<seconds>(m_ResetTime + pruneStampOffset)};
    const auto stalePath{projPaths.tests().repo() / "HouseAllocationTest.cpp"};

    fs::last_write_time(projPaths.prune().stamp(), boundary);
    fs::last_write_time(stalePath, boundary);

    check(equality,
          "A modification recorded at a stamp which lies on a second boundary is stale",
          (tests_to_run(projPaths)),
          opt_test_list{test_list{{"HouseAllocationTest.cpp"}}});

    fs::last_write_time(stalePath, m_ResetTime);
    fs::last_write_time(projPaths.prune().stamp(), m_ResetTime + pruneStampOffset);

    check(equality, "Nothing Stale", (tests_to_run(projPaths)), opt_test_list{test_list{}});
  }

  void dependency_analyzer_free_test::test_pass_recorded_in_the_modification_second(const project_paths& projPaths)
  {
    /* The record of a test passing is necessarily written after the modification which made it run,
       but on a filesystem which truncates `last_write_time` to whole seconds - libstdc++ does, on
       macOS - the two land on the same value. The record must still count as post-dating the
       change, or the test is selected again by every later run and never settles.

       The situation is constructed rather than waited for, so that it is asserted on every platform
       and not only where the filesystem happens to be coarse. Note the two times are distinct: the
       *file's* stamp ties the modification, which is what a truncating filesystem produces, while
       the record inside it is later, which is what the run wrote.
    */
    using namespace std::chrono;

    const auto passesFile{projPaths.prune().selected_passes(std::nullopt)};
    const auto testFile{projPaths.tests().repo() / "HouseAllocationTest.cpp"};
    const auto modified{m_ResetTime + earlyEditOffset};

    fs::last_write_time(testFile, modified);
    write_tests(projPaths, passesFile, std::vector<prune_record>{{"HouseAllocationTest.cpp", modified + seconds{1}}});
    fs::last_write_time(passesFile, modified);

    check(equality,
          "A pass whose record ties the modification still post-dates it, so the test is not re-run",
          (tests_to_run(projPaths)),
          opt_test_list{test_list{}});

    fs::last_write_time(testFile, m_ResetTime);
    fs::remove(passesFile);
  }

  void dependency_analyzer_free_test::check_round_trip(const reporter& description,
                                                       const project_paths& projPaths,
                                                       const prune_records& records)
  {
    const auto file{projPaths.prune().failures(std::nullopt)};
    write_tests(projPaths, file, records);
    check(equality, description, read_tests(file), records);
    fs::remove(file);
  }

  void dependency_analyzer_free_test::test_prune_record_round_trip(const project_paths& projPaths)
  {
    const auto stamp{m_ResetTime};

    check_round_trip("A single record",
                     projPaths,
                     {{"HouseAllocationTest.cpp", stamp}});

    check_round_trip("Several records",
                     projPaths,
                     {{"HouseAllocationTest.cpp", stamp}, {"Maths/ProbabilityTest.cpp", stamp}});

    check_round_trip("A path containing a space",
                     projPaths,
                     {{"Stuff/My Test.cpp", stamp}});

    check_round_trip("A path containing a space does not take the rest of the file with it",
                     projPaths,
                     {{"HouseAllocationTest.cpp", stamp}, {"Stuff/My Test.cpp", stamp}, {"Maths/ProbabilityTest.cpp", stamp}});

    check_round_trip("A path beginning with a quotation mark",
                     projPaths,
                     {{"\"HouseAllocationTest.cpp", stamp}, {"Maths/ProbabilityTest.cpp", stamp}});

    check_round_trip("A path with a quotation mark elsewhere in it",
                     projPaths,
                     {{"House\"AllocationTest.cpp", stamp}});

    check_round_trip("A path containing consecutive spaces, and one which ends in a space",
                     projPaths,
                     {{"Stuff/My  Test.cpp", stamp}, {"Stuff/Trailing.cpp ", stamp}});

    check_round_trip("A time stamp before the file clock's epoch, which under libstdc++ is every stamp there is",
                     projPaths,
                     {{"HouseAllocationTest.cpp", prune_record::stamp_type{} - std::chrono::seconds{1}}});

    check_round_trip("The file clock's epoch itself",
                     projPaths,
                     {{"HouseAllocationTest.cpp", prune_record::stamp_type{}}});

    const auto file{projPaths.prune().failures(std::nullopt)};

    {
      const transient_file noTrailingNewline{file, "path: HouseAllocationTest.cpp\ntimestamp: 0"};

      check(equality,
            "A file ending after a complete record, with no trailing newline",
            read_tests(file),
            prune_records{{"HouseAllocationTest.cpp", prune_record::stamp_type{}}});
    }

    {
      // Prune state lives in the build tree, so a file in a superseded format survives an upgrade
      const transient_file supersededFormat{file, "HouseAllocationTest.cpp 12345\nMaths/ProbabilityTest.cpp 12345\n"};

      check_exception_thrown<std::runtime_error>("A prune file in a superseded format", [&file]() { return read_tests(file); });
    }

    {
      const transient_file malformedStamp{file, "path: HouseAllocationTest.cpp\ntimestamp: 0\npath: Maths/ProbabilityTest.cpp\ntimestamp: soon\n"};

      check_exception_thrown<std::runtime_error>("A malformed time stamp", [&file]() { return read_tests(file); });
    }

    {
      const transient_file pathWithoutStamp{file, "path: HouseAllocationTest.cpp\n"};

      check_exception_thrown<std::runtime_error>("A path with no time stamp after it", [&file]() { return read_tests(file); });
    }
  }

  void dependency_analyzer_free_test::test_prune_update(const project_paths& projPaths)
  {
    const auto updateTime{m_ResetTime};
    const auto lateUpdateTime{m_ResetTime + std::chrono::seconds{1}};
    const auto prune{projPaths.prune()};
    const auto failureFile{prune.failures(std::nullopt)};
    const auto passesFile{prune.selected_passes(std::nullopt)};

    using prune_graph = transition_checker<test_outcomes>::transition_graph;
    using edge_t = transition_checker<test_outcomes>::edge;

    auto update_unfiltered{
      [&](const test_outcomes& d, test_list failures) {
        write_or_remove(projPaths, failureFile, passesFile, d);

        update_prune_files(projPaths, std::move(failures), updateTime, std::nullopt);
        return test_outcomes{read(failureFile), {read(passesFile)}};
      }
    };

    auto update_filtered{
      [&](const test_outcomes& d, test_list executed, test_list failures, std::filesystem::file_time_type targetTime) {
        write_or_remove(projPaths, failureFile, passesFile, d);

        update_prune_files(projPaths, std::move(executed), std::move(failures), targetTime, std::nullopt);
        return test_outcomes{read(failureFile), {read(passesFile)}};
      }
    };

    const prune_graph g{
      { // Begin null_fails_null_passes
        { edge_t{empty_fails_null_passes,
                 "Nothing executed, unfiltered",
                 [update_unfiltered](const test_outcomes& d) { return update_unfiltered(d, {}); }
          },
          edge_t{empty_fails_empty_passes,
                 "Nothing executed, filtered",
                 [update_filtered, updateTime](const test_outcomes& d) { return update_filtered(d, {}, {}, updateTime); }
          },
          edge_t{house_fails_null_passes,
                 "A single failure, unfiltered",
                 [update_unfiltered](const test_outcomes& d) { return update_unfiltered(d, {{"HouseAllocationTest.cpp"}}); }
          },
          edge_t{house_fails_empty_passes,
                 "A single failure, filtered",
                 [update_filtered, updateTime](const test_outcomes& d) { return update_filtered(d, {{"HouseAllocationTest.cpp"}}, {{"HouseAllocationTest.cpp"}}, updateTime); }
          },
          edge_t{empty_fails_house_passes,
                 "A single pass, filtered",
                 [update_filtered, updateTime](const test_outcomes& d) { return update_filtered(d, {{"HouseAllocationTest.cpp"}}, {}, updateTime); }
          },
          edge_t{house_prob_fails_null_passes,
                 "Two failures, unfiltered",
                 [update_unfiltered](const test_outcomes& d) { return update_unfiltered(d, {{"HouseAllocationTest.cpp"}, {"Maths/ProbabilityTest.cpp"}}); }
          }
        }, // End   null_fails_null_passes
        {  // Begin empty_fails_null_passes
        }, // End   empty_fails_null_passes
        {  // Begin empty_fails_empty_passes
        }, // End   empty_fails_empty_passes
        {  // Begin house_fails_null_passes
          edge_t{house_prob_fails_empty_passes,
                 "An additional failure, filtered",
                 [update_filtered, updateTime](const test_outcomes& d) { return update_filtered(d, {{"Maths/ProbabilityTest.cpp"}}, {{"Maths/ProbabilityTest.cpp"}}, updateTime); }
          },
          edge_t{house_prob_maybe_fails_empty_passes,
                 "Two additional failures, filtered",
                 [update_filtered, updateTime](const test_outcomes& d) { return update_filtered(d, {{"Maths/ProbabilityTest.cpp"}, {"Maybe/MaybeTest.cpp"}}, {{"Maths/ProbabilityTest.cpp"}, {"Maybe/MaybeTest.cpp"}}, updateTime); }
          },
          edge_t{house_prob_fails_maybe_passes,
                 "One additional failure, one pass, filtered",
                 [update_filtered, updateTime](const test_outcomes& d) { return update_filtered(d, {{"Maths/ProbabilityTest.cpp"}, {"Maybe/MaybeTest.cpp"}}, {{"Maths/ProbabilityTest.cpp"}}, updateTime); }
          },
          edge_t{house_fails_maybe_prob_passes,
                 "Two additional passes, filtered",
                 [update_filtered, updateTime](const test_outcomes& d) { return update_filtered(d, {{"Maths/ProbabilityTest.cpp"}, {"Maybe/MaybeTest.cpp"}}, {}, updateTime); }
          },
          edge_t{house_fails_late_empty_passes,
                 "The same test, failing later",
                 [update_filtered, lateUpdateTime](const test_outcomes& d) {
                   return update_filtered(d, {{"HouseAllocationTest.cpp"}}, {{"HouseAllocationTest.cpp"}}, lateUpdateTime);
                 }
          },
          edge_t{house_prob_fails_late_empty_passes,
                 "The same test and another one, failing later",
                 [update_filtered, lateUpdateTime](const test_outcomes& d) {
                   return update_filtered(
                            d,
                            {{"HouseAllocationTest.cpp"}, {"Maths/ProbabilityTest.cpp"}},
                            {{"HouseAllocationTest.cpp"}, {"Maths/ProbabilityTest.cpp"}},
                            lateUpdateTime
                   );
                 }
          },
        }, // End   house_fails_null_passes
        {  // Begin house_fails_empty_passes
        }, // End   house_fails_empty_passes
        {  // Begin empty_fails_house_passes
        }, // End   empty_fails_house_passes
        {  // Begin house_prob_fails_null_passes
          edge_t{house_fails_null_passes,
                 "One failure fewer, unfiltered",
                 [update_unfiltered](const test_outcomes& d) { return update_unfiltered(d, {{"HouseAllocationTest.cpp"}}); }
          }
        }, // End   house_prob_fails_null_passes
        {  // Begin house_prob_fails_empty_passes
          edge_t{house_fails_prob_passes,
                 "One failure fewer, filtered",
                 [update_filtered, updateTime](const test_outcomes& d) { return update_filtered(d, {{"Maths/ProbabilityTest.cpp"}}, {}, updateTime); }
          },
          edge_t{house_prob_maybe_fails_empty_passes,
                 "One more failure, filtered",
                 [update_filtered, updateTime](const test_outcomes& d) { return update_filtered(d, {{"Maybe/MaybeTest.cpp"}}, {{"Maybe/MaybeTest.cpp"}}, updateTime); }
          },
          edge_t{house_prob_fails_late_empty_passes,
                 "Same failures at a later time, filtered",
                 [update_filtered, lateUpdateTime](const test_outcomes& d) {
                   return update_filtered(
                            d,
                            {{"Maths/ProbabilityTest.cpp"}, {"HouseAllocationTest.cpp"}},
                            {{"HouseAllocationTest.cpp"}, {"Maths/ProbabilityTest.cpp"}},
                            lateUpdateTime
                          );
                 }
                 
          }
        }, // End   house_prob_fails_empty_passes
        {  // Begin house_fails_prob_passes
          edge_t{empty_fails_null_passes ,
                 "No failures, unfiltered",
                 [update_unfiltered](const test_outcomes& d) { return update_unfiltered(d, {}); }
          },
          edge_t{house_prob_fails_empty_passes,
                 "Add a failure, filtered",
                 [update_filtered, updateTime](const test_outcomes& d) { return update_filtered(d, {{"Maths/ProbabilityTest.cpp"}}, {{"Maths/ProbabilityTest.cpp"}}, updateTime); }
          }
        }, // End   house_fails_prob_passes
        {  // Begin house_prob_maybe_fails_empty_passes
          edge_t{empty_fails_null_passes,
                "Three failures all pass, unfiltered",
                [update_unfiltered](const test_outcomes& d) { return update_unfiltered(d, {}); }
          }
        }, // End   house_prob_maybe_fails_empty_passes
        {  // Begin house_prob_fails_maybe_passes
          edge_t{house_fails_maybe_prob_passes,
                 "One of two failures becomes a pass, filtered",
                 [update_filtered, updateTime](const test_outcomes& d) { return update_filtered(d, {{"Maths/ProbabilityTest.cpp"}}, {}, updateTime); }
          }
        }, // End   house_prob_fails_maybe_passes
        {  // Begin house_fails_maybe_prob_passes
          edge_t{empty_fails_maybe_house_prob_passes,
                 "Only failure becomes a pass, filtered",
                 [update_filtered, updateTime](const test_outcomes& d) { return update_filtered(d, {{"HouseAllocationTest.cpp"}}, {}, updateTime); }
          }
        }, // End   house_fails_maybe_prob_passes
        {  // Begin empty_fails_maybe_house_prob_passes
          edge_t{house_fails_maybe_prob_passes,
                 "One pass becomes a failure, filtered",
                 [update_filtered, updateTime](const test_outcomes& d) { return update_filtered(d, {{"HouseAllocationTest.cpp"}}, {{"HouseAllocationTest.cpp"}}, updateTime); }
          },
          edge_t{house_prob_fails_maybe_passes,
                 "Two passes becomes failures, filtered",
                 [update_filtered, updateTime](const test_outcomes& d) { return update_filtered(d, {{"HouseAllocationTest.cpp"}, {"Maths/ProbabilityTest.cpp"}}, {{"HouseAllocationTest.cpp"}, {"Maths/ProbabilityTest.cpp"}}, updateTime); }
          }
        }, // End   empty_fails_maybe_house_prob_passes
        {  // Begin house_fails_late_empty_passes
        }, // End   house_fails_late_empty_passes
        {  // Begin house_prob_fails_late_empty_passes
        }, // End   house_prob_fails_late_empty_passes
      },
      {
        test_outcomes{std::nullopt, std::nullopt},
        test_outcomes{prune_records{}, std::nullopt},
        test_outcomes{prune_records{}, prune_records{}},
        test_outcomes{{{{"HouseAllocationTest.cpp", updateTime}}}, std::nullopt},
        test_outcomes{{{{"HouseAllocationTest.cpp", updateTime}}}, prune_records{}},
        test_outcomes{prune_records{}, {{{"HouseAllocationTest.cpp", updateTime}}}},
        test_outcomes{{{{"HouseAllocationTest.cpp", updateTime}, {"Maths/ProbabilityTest.cpp", updateTime}}}, std::nullopt},
        test_outcomes{{{{"HouseAllocationTest.cpp", updateTime}, {"Maths/ProbabilityTest.cpp", updateTime}}}, prune_records{}},
        test_outcomes{{{{"HouseAllocationTest.cpp", updateTime}}}, {{{"Maths/ProbabilityTest.cpp", updateTime}}}},
        test_outcomes{{{{"HouseAllocationTest.cpp", updateTime}, {"Maybe/MaybeTest.cpp", updateTime}, {"Maths/ProbabilityTest.cpp", updateTime}}}, prune_records{}},
        test_outcomes{{{{"HouseAllocationTest.cpp", updateTime}, {"Maths/ProbabilityTest.cpp", updateTime}}}, {{{"Maybe/MaybeTest.cpp", updateTime}}}},
        test_outcomes{{{{"HouseAllocationTest.cpp", updateTime}}}, {{{"Maybe/MaybeTest.cpp", updateTime}, {"Maths/ProbabilityTest.cpp", updateTime}}}},
        test_outcomes{prune_records{}, {{{"Maybe/MaybeTest.cpp", updateTime}, {"HouseAllocationTest.cpp", updateTime}, {"Maths/ProbabilityTest.cpp", updateTime}}}},
        test_outcomes{{{{"HouseAllocationTest.cpp", lateUpdateTime}}}, prune_records{}},
        test_outcomes{{{{"HouseAllocationTest.cpp", lateUpdateTime}, {"Maths/ProbabilityTest.cpp", lateUpdateTime}}}, prune_records{}},
      }
    };


    auto checkerFn{
        [this](std::string_view description, const test_outcomes& obtained, const test_outcomes& prediction) {
          check_data(description, obtained, prediction);
        }
    };

    transition_checker<test_outcomes>::check(report(""), g, checkerFn);
  }

  void dependency_analyzer_free_test::test_instability_analysis_prune_upate(const project_paths& projPaths)
  {
    const auto updateTime{m_ResetTime};
    const auto lateUpdateTime{m_ResetTime + std::chrono::seconds{1}};
    const auto prune{projPaths.prune()};
    const auto failureFile{prune.failures(std::nullopt)};
    const auto passesFile{prune.selected_passes(std::nullopt)};

    fs::remove_all(prune.dir());
    fs::create_directory(prune.dir());

    using prune_graph = transition_checker<test_outcomes>::transition_graph;
    using edge_t = transition_checker<test_outcomes>::edge;

    auto update_unfiltered{
      [&](const test_outcomes& d, multi_test_list failures) -> test_outcomes {
        setup_instability_analysis_prune_folder(projPaths);

        write_or_remove(projPaths, failureFile, passesFile, d);

        for(auto i : std::views::iota(0uz, failures.size()))
        {
          update_prune_files(projPaths, std::move(failures[i]), updateTime, i);
        }

        aggregate_instability_analysis_prune_files(projPaths, prune_mode::active, updateTime, failures.size());

        return {read(failureFile), read(passesFile)};
      }
    };

    auto update_filtered{
      [&](const test_outcomes& d, test_list executed, multi_test_list failures, std::filesystem::file_time_type targetTime) -> test_outcomes {

        setup_instability_analysis_prune_folder(projPaths);

        write_or_remove(projPaths, failureFile, passesFile, d);

        for(auto i : std::views::iota(0uz, failures.size()))
        {
          update_prune_files(projPaths, executed, std::move(failures[i]), targetTime, i);
        }

        aggregate_instability_analysis_prune_files(projPaths, prune_mode::passive, targetTime, failures.size());

        return {read(failureFile), read(passesFile)};
      }
    };

    const prune_graph g{
      {
        { // Begin null_fails_null_passes
          edge_t{empty_fails_null_passes,
                 "Nothing executed, unfiltered",
                 [update_unfiltered](const test_outcomes& d) { return update_unfiltered(d, {}); }
          },
          edge_t{empty_fails_empty_passes,
                 "Nothing executed, filtered",
                 [update_filtered, updateTime](const test_outcomes& d) { return update_filtered(d, {}, {{}}, updateTime); }
          },
          edge_t{house_fails_null_passes,
                 "A single failure in only the first of two instances, unfiltered",
                 [update_unfiltered](const test_outcomes& d) { return update_unfiltered(d, {{{"HouseAllocationTest.cpp"}}, {}}); }
          },
          edge_t{house_fails_null_passes,
                 "A single failure in only the second of two instances, unfiltered",
                 [update_unfiltered](const test_outcomes& d) { return update_unfiltered(d, {{}, {{"HouseAllocationTest.cpp"}}}); }
          },
          edge_t{house_fails_null_passes,
                 "A single failure in both instances, unfiltered",
                 [update_unfiltered](const test_outcomes& d) { return update_unfiltered(d, {{{"HouseAllocationTest.cpp"}}, {{"HouseAllocationTest.cpp"}}}); }
          },
          edge_t{house_fails_empty_passes,
                 "A single failure in only the first of two instances, filtered",
                 [update_filtered, updateTime](const test_outcomes& d) { return update_filtered(d, {{"HouseAllocationTest.cpp"}}, {{{"HouseAllocationTest.cpp"}}, {}}, updateTime); }
          },
          edge_t{house_fails_empty_passes,
                 "A single failure in only the second of two instances, filtered",
                 [update_filtered, updateTime](const test_outcomes& d) { return update_filtered(d, {{"HouseAllocationTest.cpp"}}, {{}, {{"HouseAllocationTest.cpp"}}}, updateTime); }
          },
          edge_t{house_fails_empty_passes,
                 "A single failure in both instances, filtered",
                 [update_filtered, updateTime](const test_outcomes& d) { return update_filtered(d, {{"HouseAllocationTest.cpp"}}, {{{"HouseAllocationTest.cpp"}}, {{"HouseAllocationTest.cpp"}}}, updateTime); }
          },
          edge_t{empty_fails_house_passes,
                 "Passes in both instances, filtered",
                 [update_filtered, updateTime](const test_outcomes& d) { return update_filtered(d, {{"HouseAllocationTest.cpp"}}, {{}, {}}, updateTime); }
          },
          edge_t{house_prob_fails_null_passes,
                 "Two failures, unfiltered, on different runs",
                 [update_unfiltered](const test_outcomes& d) { return update_unfiltered(d, {{{"HouseAllocationTest.cpp"}}, {{"Maths/ProbabilityTest.cpp"}}}); }
          },
          edge_t{house_prob_fails_null_passes,
                 "Two failures, unfiltered, on different runs",
                 [update_unfiltered](const test_outcomes& d) { return update_unfiltered(d, {{{"Maths/ProbabilityTest.cpp"}}, {{"HouseAllocationTest.cpp"}}}); }
          },
        }, // End   null_fails_null_passes
        {  // Begin empty_fails_null_passes
        }, // End   empty_fails_null_passes
        {  // Begin empty_fails_empty_passes
        }, // End   empty_fails_empty_passes
        {  // Begin house_fails_null_passes
          edge_t{house_prob_fails_empty_passes,
                 "An additional failure on the first run, filtered",
                 [update_filtered, updateTime](const test_outcomes& d) { return update_filtered(d, {{"Maths/ProbabilityTest.cpp"}}, {{{"Maths/ProbabilityTest.cpp"}}, {}}, updateTime); }
          },
          edge_t{house_prob_fails_empty_passes,
                 "An additional failure on the second run, filtered",
                 [update_filtered, updateTime](const test_outcomes& d) { return update_filtered(d, {{"Maths/ProbabilityTest.cpp"}}, {{}, {{"Maths/ProbabilityTest.cpp"}}}, updateTime); }
          },
          edge_t{house_prob_maybe_fails_empty_passes,
                 "Two additional failures on different runs, filtered",
                 [update_filtered, updateTime](const test_outcomes& d) {
                   return update_filtered(
                            d,
                            {{"Maths/ProbabilityTest.cpp"}, {"Maybe/MaybeTest.cpp"}},
                            {{{"Maths/ProbabilityTest.cpp"}}, {{"Maybe/MaybeTest.cpp"}}},
                            updateTime
                   );
                 }
          },
          edge_t{house_prob_maybe_fails_empty_passes,
                 "Two additional failures on different runs, filtered",
                 [update_filtered, updateTime](const test_outcomes& d) {
                   return update_filtered(
                            d,
                            {{"Maths/ProbabilityTest.cpp"}, {"Maybe/MaybeTest.cpp"}},
                            {{{"Maybe/MaybeTest.cpp"}}, {{"Maths/ProbabilityTest.cpp"}}},
                            updateTime
                   );
                 }
          },
          edge_t{house_prob_maybe_fails_empty_passes,
                 "Two additional failures, one reliable, filtered",
                 [update_filtered, updateTime](const test_outcomes& d) {
                   return update_filtered(
                            d,
                            {{"Maths/ProbabilityTest.cpp"}, {"Maybe/MaybeTest.cpp"}},
                            {{{"Maybe/MaybeTest.cpp"}, {"Maths/ProbabilityTest.cpp"}}, {{"Maths/ProbabilityTest.cpp"}}},
                            updateTime
                   );
                 }
          },
          edge_t{house_fails_late_empty_passes,
                 "The same test, failing later, on the first run",
                 [update_filtered, lateUpdateTime](const test_outcomes& d) {
                   return update_filtered(d, {{"HouseAllocationTest.cpp"}}, {{{"HouseAllocationTest.cpp"}}, {}}, lateUpdateTime);
                 }
          },
          edge_t{house_fails_late_empty_passes,
                 "The same test, failing later, on the second run",
                 [update_filtered, lateUpdateTime](const test_outcomes& d) {
                   return update_filtered(d, {{"HouseAllocationTest.cpp"}}, {{}, {{"HouseAllocationTest.cpp"}}}, lateUpdateTime);
                 }
          },
          edge_t{house_prob_fails_late_empty_passes,
                 "The same test and another one failing later, on different runs",
                 [update_filtered, lateUpdateTime](const test_outcomes& d) {
                   return update_filtered(
                            d,
                            {{"HouseAllocationTest.cpp"}, {"Maths/ProbabilityTest.cpp"}},
                            {{{"HouseAllocationTest.cpp"}}, {{"Maths/ProbabilityTest.cpp"}}},
                            lateUpdateTime
                          );
                 }
          },
          edge_t{house_prob_fails_late_empty_passes,
                 "The same test and another one failing later, on different runs",
                 [update_filtered, lateUpdateTime](const test_outcomes& d) {
                   return update_filtered(
                            d,
                            {{"HouseAllocationTest.cpp"}, {"Maths/ProbabilityTest.cpp"}},
                            {{{"Maths/ProbabilityTest.cpp"}}, {{"HouseAllocationTest.cpp"}}},
                            lateUpdateTime
                          );
                 }
          },
        }, // End   house_fails_null_passes
        {  // Begin house_fails_empty_passes
        }, // End   house_fails_empty_passes
        {  // Begin empty_fails_house_passes
          edge_t{house_prob_fails_null_passes,
                 "Two failures, from differing instances, unfiltered",
                 [update_unfiltered](const test_outcomes& d) { return update_unfiltered(d, {{{"HouseAllocationTest.cpp"}}, {{"Maths/ProbabilityTest.cpp"}}}); }
          },
          edge_t{house_prob_fails_empty_passes,
                 "Two failures, from three instances, filtered",
                 [update_filtered, updateTime](const test_outcomes& d) {
                    return update_filtered(
                             d,
                             {{"HouseAllocationTest.cpp"}, {"Maths/ProbabilityTest.cpp"}},
                             {{{"Maths/ProbabilityTest.cpp"}}, {}, {"HouseAllocationTest.cpp"}},
                             updateTime
                           );
                 }
          }
        }, // End   empty_fails_house_passes
        {  // Begin house_prob_fails_null_passes
          edge_t{house_fails_null_passes,
                 "One failure fewer on the first run, unfiltered",
                 [update_unfiltered](const test_outcomes& d) { return update_unfiltered(d, {{{"HouseAllocationTest.cpp"}}, {}}); }
          },
          edge_t{house_fails_null_passes,
                 "One failure fewer on the second run, unfiltered",
                 [update_unfiltered](const test_outcomes& d) { return update_unfiltered(d, {{}, {{"HouseAllocationTest.cpp"}}}); }
          }
        }, // End   house_prob_fails_null_passes
        {  // Begin house_prob_fails_empty_passes
          edge_t{house_fails_prob_passes,
                 "One failure fewer, filtered",
                 [update_filtered, updateTime](const test_outcomes& d) { return update_filtered(d, {{"Maths/ProbabilityTest.cpp"}}, {{}, {}}, updateTime); }
          },
          edge_t{house_prob_maybe_fails_empty_passes,
                 "One more failure on second run, filtered",
                 [update_filtered, updateTime](const test_outcomes& d) { return update_filtered(d, {{"Maybe/MaybeTest.cpp"}}, {{}, {{"Maybe/MaybeTest.cpp"}}}, updateTime); }
          },
        }, // End   house_prob_fails_empty_passes
        {  // Begin house_fails_prob_passes
        }, // End   house_fails_prob_passes
        {  // Begin house_prob_maybe_fails_empty_passes
        }, // End   house_prob_maybe_fails_empty_passes
        {  // Begin house_prob_fails_maybe_passes
        }, // End   house_prob_fails_maybe_passes
        {  // Begin house_fails_maybe_prob_passes
        }, // End   house_fails_maybe_prob_passes
        {  // Begin empty_fails_maybe_house_prob_passes
        }, // End   empty_fails_maybe_house_prob_passes
        {  // Begin house_fails_late_empty_passes
        }, // End   house_fails_late_empty_passes
        {  // Begin house_prob_fails_late_empty_passes
        }, // End   house_prob_fails_late_empty_passes
      },
      {
        test_outcomes{std::nullopt, std::nullopt},
        test_outcomes{prune_records{}, std::nullopt},
        test_outcomes{prune_records{}, prune_records{}},
        test_outcomes{{{{"HouseAllocationTest.cpp", updateTime}}}, std::nullopt},
        test_outcomes{{{{"HouseAllocationTest.cpp", updateTime}}}, prune_records{}},
        test_outcomes{prune_records{}, {{{"HouseAllocationTest.cpp", updateTime}}}},
        test_outcomes{{{{"HouseAllocationTest.cpp", updateTime}, {"Maths/ProbabilityTest.cpp", updateTime}}}, std::nullopt},
        test_outcomes{{{{"HouseAllocationTest.cpp", updateTime}, {"Maths/ProbabilityTest.cpp", updateTime}}}, prune_records{}},
        test_outcomes{{{{"HouseAllocationTest.cpp", updateTime}}}, {{{"Maths/ProbabilityTest.cpp", updateTime}}}},
        test_outcomes{{{{"HouseAllocationTest.cpp", updateTime}, {"Maybe/MaybeTest.cpp", updateTime}, {"Maths/ProbabilityTest.cpp", updateTime}}}, prune_records{}},
        test_outcomes{{{{"HouseAllocationTest.cpp", updateTime}, {"Maths/ProbabilityTest.cpp", updateTime}}}, {{{"Maybe/MaybeTest.cpp", updateTime}}}},
        test_outcomes{{{{"HouseAllocationTest.cpp", updateTime}}}, {{{"Maybe/MaybeTest.cpp", updateTime}, {"Maths/ProbabilityTest.cpp", updateTime}}}},
        test_outcomes{prune_records{}, {{{"Maybe/MaybeTest.cpp", updateTime}, {"HouseAllocationTest.cpp", updateTime}, {"Maths/ProbabilityTest.cpp", updateTime}}}},
        test_outcomes{{{{"HouseAllocationTest.cpp", lateUpdateTime}}}, prune_records{}},
        test_outcomes{{{{"HouseAllocationTest.cpp", lateUpdateTime}, {"Maths/ProbabilityTest.cpp", lateUpdateTime}}}, prune_records{}},
      }
    };

    auto checkerFn{
        [this](std::string_view description, const test_outcomes& obtained, const test_outcomes& prediction) {
          check_data(description, obtained, prediction);
        }
    };

    transition_checker<test_outcomes>::check(report(""), g, checkerFn);
  }

  void dependency_analyzer_free_test::check_data(std::string_view description, const test_outcomes& obtained, const test_outcomes& prediction)
  {
    check(equality, std::string{description}.append(": failures"), obtained.failures, prediction.failures);
    check(equality, std::string{description}.append(": passes"), obtained.passes, prediction.passes);
  }

}
