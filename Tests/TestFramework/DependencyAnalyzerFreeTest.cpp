////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "DependencyAnalyzerFreeTest.hpp"
#include "Parsing/CommandLineArgumentsTestingUtilities.hpp"

#include "sequoia/TestFramework/StateTransitionUtilities.hpp"
#include "sequoia/TextProcessing/Patterns.hpp"
#include "sequoia/TestFramework/ChronoCheckers.hpp"
#include "sequoia/TestFramework/SumTypeCheckers.hpp"

#include <fstream>
#include <sstream>

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
                                                         std::string_view cutoff,
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

    opt_test_list actual{tests_to_run(projPaths, cutoff)};

    opt_test_list prediction{fileStates.to_run};
    std::ranges::sort(*prediction);

    check(equality, description, actual, prediction);

    for(const auto& f : fileStates.stale)
    {
      fs::last_write_time(f.file, m_ResetTime);
    }

    fs::remove(failureFile);
    fs::remove(passesFile);

    check(equality, append_lines(description.message(), "Nothing Stale"), tests_to_run(projPaths, cutoff), opt_test_list{test_list{}});
  }

  void dependency_analyzer_free_test::run_tests()
  {
    test_staleness_threshold();
    test_source_scanning();

    m_ResetTime = std::chrono::file_clock::now() + resetOffset;

    const auto fake{auxiliary_materials() /= "FakeProject"};
    const main_paths main{fake / main_paths::default_main_cpp_from_root()};
    commandline_arguments args{{(fake / "build/CMade/TestAll/TestAll").generic_string()}};
    const project_paths projPaths{args.size(), args.get(), {.additional_dependency_analysis_paths{{"TestUtilities"}, {"dependencies/foo/Source"}}, .main_cpp{main.file()}, .common_includes{main.file()}}};

    check(equality, "No timestamp", tests_to_run(projPaths, ""), opt_test_list{});

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
    test_stamp_on_second_boundary(projPaths);
    test_pass_recorded_in_the_modification_second(projPaths);
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

  void dependency_analyzer_free_test::check_scan(const reporter& description,
                                                 std::string_view source,
                                                 std::string_view cutoff,
                                                 const std::vector<fs::path>& prediction)
  {
    std::istringstream stream{std::string{source}};
    check(equality, description, scan_dependencies(stream, cutoff).includes, prediction);
  }

  void dependency_analyzer_free_test::test_source_scanning()
  {
    using paths = std::vector<fs::path>;

    check_scan("Nothing at all", "", "", paths{});
    check_scan("Source with no directives", "int main() { return 0; }\n", "", paths{});

    check_scan("Quoted header name",  "#include \"foo.hpp\"\n", "", paths{"foo.hpp"});
    check_scan("Angled header name",  "#include <foo.hpp>\n",   "", paths{"foo.hpp"});
    check_scan("Header name carrying a directory", "#include \"Stuff/Foo.hpp\"\n", "", paths{"Stuff/Foo.hpp"});
    check_scan("Several includes, in the order written",
               "#include \"foo.hpp\"\n#include <bar.hpp>\n#include \"baz.hpp\"\n",
               "",
               paths{"foo.hpp", "bar.hpp", "baz.hpp"});

    check_scan("A preceding directive does not swallow the include on the next line",
               "#ifdef SOMETHING\n#endif\n#include \"foo.hpp\"\n",
               "",
               paths{"foo.hpp"});

    check_scan("...nor does one carrying a trailing comment",
               "#ifdef SOMETHING\n#endif // SOMETHING\n#include \"foo.hpp\"\n",
               "",
               paths{"foo.hpp"});

    check_scan("A tab separates the directive name from the header name",
               "#include\t\"foo.hpp\"\n",
               "",
               paths{"foo.hpp"});

    check_scan("Nothing at all separates the directive name from the header name",
               "#include\"foo.hpp\"\n",
               "",
               paths{"foo.hpp"});

    check_scan("...and the same for an angled one",
               "#include<foo.hpp>\n",
               "",
               paths{"foo.hpp"});

    check_scan("A space separates the hash from the directive name",
               "# include \"foo.hpp\"\n",
               "",
               paths{"foo.hpp"});

    check_scan("A tab separates the hash from the directive name",
               "#\tinclude <foo.hpp>\n",
               "",
               paths{"foo.hpp"});

    check_scan("Both separations at once, each spelled differently",
               "#  include\t<foo.hpp>\n",
               "",
               paths{"foo.hpp"});

    check_scan("A vertical tab separates the tokens",
               "#\vinclude\v\"foo.hpp\"\n",
               "",
               paths{"foo.hpp"});

    check_scan("A form feed separates the tokens",
               "#\finclude\f\"foo.hpp\"\n",
               "",
               paths{"foo.hpp"});

    check_scan("Indented directive", "  \t#include \"foo.hpp\"\n", "", paths{"foo.hpp"});

    check_scan("Carriage returns do not reach the header name",
               "#ifdef SOMETHING\r\n#endif\r\n#include \"foo.hpp\"\r\n#include <bar.hpp>\r\n",
               "",
               paths{"foo.hpp", "bar.hpp"});

    check_scan("A directive name of which `include` is merely a prefix",
               "#included \"foo.hpp\"\n",
               "",
               paths{});

    check_scan("An undelimited header name is not one",
               "#include foo.hpp\n#include \"bar.hpp\"\n",
               "",
               paths{"bar.hpp"});

    check_scan("A header name which the file ends in the middle of",
               "#include \"foo.hpp",
               "",
               paths{});

    check_scan("A string literal spelled exactly `\"#include\"` opens no header name",
               "std::string_view tag{\"#include\"};\n#include \"foo.hpp\"\n",
               "",
               paths{"foo.hpp"});

    check_scan("...nor does one with a trailing space",
               "const char* s{\"#include \"};\n#include \"foo.hpp\"\n",
               "",
               paths{"foo.hpp"});

    check_scan("...nor one with a trailing angle bracket",
               "const char* s{\"#include <\"};\n#include \"foo.hpp\"\n",
               "",
               paths{"foo.hpp"});

    check_scan("A directive with nothing following it",
               "#include\n#include \"foo.hpp\"\n",
               "",
               paths{"foo.hpp"});

    check_scan("A bare hash", "#\n#include \"foo.hpp\"\n", "", paths{"foo.hpp"});

    check_scan("Standard library headers are lexed; filtering them is the caller's business",
               "#include <vector>\n#include \"foo.hpp\"\n",
               "",
               paths{"vector", "foo.hpp"});

    check_scan("An include commented out line-wise",
               "// #include \"foo.hpp\"\n#include \"bar.hpp\"\n",
               "",
               paths{"bar.hpp"});

    check_scan("An include commented out block-wise",
               "/* #include \"foo.hpp\" */\n#include \"bar.hpp\"\n",
               "",
               paths{"bar.hpp"});

    check_scan("A trailing comment ends where the next include begins",
               "#include \"foo.hpp\" // why\n#include \"bar.hpp\"\n",
               "",
               paths{"foo.hpp", "bar.hpp"});

    check_scan("A solidus which opens no comment",
               "int x{a/b};\n#include \"foo.hpp\"\n",
               "",
               paths{"foo.hpp"});

    check_scan("An unterminated block comment consumes the rest of the file",
               "/* #include \"foo.hpp\"\n#include \"bar.hpp\"\n",
               "",
               paths{});

    check_scan("Scanning stops at the first line containing the cutoff",
               "#include \"foo.hpp\"\nnamespace stuff {}\n#include \"bar.hpp\"\n",
               "namespace",
               paths{"foo.hpp"});

    check_scan("An empty cutoff scans to the end",
               "#include \"foo.hpp\"\nnamespace stuff {}\n#include \"bar.hpp\"\n",
               "",
               paths{"foo.hpp", "bar.hpp"});

    check_scan("A cutoff which never appears",
               "#include \"foo.hpp\"\n#include \"bar.hpp\"\n",
               "namespace",
               paths{"foo.hpp", "bar.hpp"});

    check_scan("Conditional compilation is lexed, not evaluated: the guarded include is reported",
               "#if 0\n#include \"foo.hpp\"\n#endif\n#include \"bar.hpp\"\n",
               "",
               paths{"foo.hpp", "bar.hpp"});

    check_scan("A directive inside a string literal is reported, for the same reason",
               "const char* s{\"#include <foo.hpp>\"};\n#include \"bar.hpp\"\n",
               "",
               paths{"foo.hpp", "bar.hpp"});
  }

  void dependency_analyzer_free_test::test_exceptions(const project_paths& projPaths)
  {
    check_exception_thrown<std::runtime_error>(
      "Executable out of date",
      [this, projPaths]() {
        fs::last_write_time(projPaths.executable(), m_ResetTime + earlyExecutableOffset);
        return tests_to_run(projPaths, "");
      },
      [](const project_paths& paths, std::string message) {
        message = default_exception_message_postprocessor{}(paths, std::move(message));
        {
          const auto [first, last]{find_sandwiched_text(message, "fakeProject", "time")};
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

    check_tests_to_run("Nothing stale", projPaths, "", {}, {}, {});

    fs::copy(projPaths.prune().external_dependencies(), working_materials());
    check(weak_equivalence, "External Dependencies", working_materials(), predictive_materials());

    check_tests_to_run("Test cpp stale (no cutoff)",
                       projPaths,
                       "",
                       {.stale{{{testRepo / "HouseAllocationTest.cpp"}, modification_time::early}}, .to_run{{"HouseAllocationTest.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Test cpp naively stale, but has passed (when selected)",
                       projPaths,
                       "namespace",
                       {.stale{{{testRepo / "HouseAllocationTest.cpp"}, modification_time::early}}, .to_run{}},
                       {},
                       {{"HouseAllocationTest.cpp", m_ResetTime + to_duration(modification_time::late)}});

    check_tests_to_run("Test cpp stale; has previously passed (when selected), but this should be ignored",
                       projPaths,
                       "namespace",
                       {.stale{{{testRepo / "HouseAllocationTest.cpp"}, modification_time::early}}, .to_run{{"HouseAllocationTest.cpp"}}},
                       {},
                       {{"HouseAllocationTest.cpp", m_ResetTime + to_duration(modification_time::very_early)}});

    check_tests_to_run("Test hpp stale (no cutoff)",
                       projPaths,
                       "",
                       {.stale{{{testRepo / "HouseAllocationTest.hpp"}, modification_time::early}}, .to_run{{"HouseAllocationTest.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Test hpp stale",
                       projPaths,
                       "namespace",
                       {.stale{{{testRepo / "HouseAllocationTest.hpp"}, modification_time::early}}, .to_run{{"HouseAllocationTest.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Test utils stale",
                       projPaths,
                       "namespace",
                       {.stale{{{testRepo / "Maths" / "ProbabilityTestingUtilities.hpp"}, modification_time::early}},
                         .to_run{{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Reused utils stale",
                       projPaths,
                       "namespace",
                       {.stale{{{testRepo / "Stuff" / "OldschoolTestingUtilities.hpp"}, modification_time::early}},
                         .to_run{{"Maybe/MaybeTest.cpp"}, {"Stuff/OldschoolTest.cpp"}, {"Stuff/OldschoolTestingDiagnostics.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Reused utils stale, but one of the tests has passed",
                       projPaths,
                       "namespace",
                       {.stale{{{testRepo / "Stuff" / "OldschoolTestingUtilities.hpp"}, modification_time::early}},
                         .to_run{{"Stuff/OldschoolTest.cpp"}, {"Stuff/OldschoolTestingDiagnostics.cpp"}}},
                       {},
                       {{"Maybe/MaybeTest.cpp", m_ResetTime + to_duration(modification_time::late)}});

    check_tests_to_run("Reused utils stale, but two of the tests have passed",
                       projPaths,
                       "namespace",
                       {.stale{{{testRepo / "Stuff" / "OldschoolTestingUtilities.hpp"}, modification_time::early}},
                         .to_run{{"Stuff/OldschoolTestingDiagnostics.cpp"}}},
                       {},
                       {{"Maybe/MaybeTest.cpp"    , m_ResetTime + to_duration(modification_time::late)},
                        {"Stuff/OldschoolTest.cpp", m_ResetTime + to_duration(modification_time::late)}});

    check_tests_to_run("Reused utils stale, but two of the tests have passed and a different one has failed",
                       projPaths,
                       "namespace",
                       {.stale{{{testRepo / "Stuff" / "OldschoolTestingUtilities.hpp"}, modification_time::early}},
                         .to_run{{"Stuff/OldschoolTestingDiagnostics.cpp"}, {"HouseAllocationTest.cpp"}}},
                       {{"HouseAllocationTest.cpp" , m_ResetTime + to_duration(modification_time::late)}},
                       {{"Maybe/MaybeTest.cpp"    , m_ResetTime + to_duration(modification_time::late)},
                        {"Stuff/OldschoolTest.cpp", m_ResetTime + to_duration(modification_time::late)}});

    check_tests_to_run("Reused utils stale, relative path",
                       projPaths,
                       "namespace",
                       {.stale{{{testRepo / "Stuff" / "FooTestingUtilities.hpp"}, modification_time::early}},
                         .to_run{{"Stuff/FooTest.cpp"}, {"Stuff/FooTestingDiagnostics.cpp"}, {"Utilities/Thing/UniqueThingTest.cpp"}, {"Utilities/Thing/UniqueThingTestingDiagnostics.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Source cpp stale",
                       projPaths,
                       "namespace",
                       {.stale{{{sourceRepo / "Maths" / "Probability.cpp"}, modification_time::early}},
                         .to_run{{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Source hpp stale",
                       projPaths,
                       "namespace",
                       {.stale{{{sourceRepo / "Maths" / "Probability.hpp"}, modification_time::early}},
                         .to_run{{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Source hpp stale, following a previously successful run",
                       projPaths,
                       "namespace",
                       {.stale{{{sourceRepo / "Maths" / "Probability.hpp"}, modification_time::early}},
                         .to_run{{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}}},
                       {},
                       {{"Maths/ProbabilityTest.cpp"              , m_ResetTime + to_duration(modification_time::very_early)},
                        {"Maths/ProbabilityTestingDiagnostics.cpp", m_ResetTime + to_duration(modification_time::very_early)}});

    check_tests_to_run("Source cpp stale, following a previously successful run",
                       projPaths,
                       "namespace",
                       {.stale{{{sourceRepo / "Maths" / "Probability.cpp"}, modification_time::early}},
                         .to_run{{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}}},
                       {},
                       {{"Maths/ProbabilityTest.cpp"              , m_ResetTime + to_duration(modification_time::very_early)},
                        {"Maths/ProbabilityTestingDiagnostics.cpp", m_ResetTime + to_duration(modification_time::very_early)}});

    check_tests_to_run("Source cpp indirectly stale via included header",
                       projPaths,
                       "namespace",
                       {.stale{{{sourceRepo / "Maths" / "Helper.hpp"}, modification_time::early}},
                         .to_run{{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}}},
                       {},
                       {});

    // The lexer's own coverage is in test_source_scanning; these two put the same spellings
    // through the whole pipeline, so that an edge the scanner reports is an edge the graph has.
    check_tests_to_run("Source hpp stale, reached by an include whose hash is followed by whitespace",
                       projPaths,
                       "namespace",
                       {.stale{{{sourceRepo / "Stuff" / "Baz.hpp"}, modification_time::early}},
                         .to_run{{"Stuff/BarFreeTest.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Source hpp stale, reached by an include with no space before the header name",
                       projPaths,
                       "namespace",
                       {.stale{{{sourceRepo / "Stuff" / "Qux.hpp"}, modification_time::early}},
                         .to_run{{"Stuff/BarFreeTest.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Source cpp indirectly stale via cpp definitions for included header",
                       projPaths,
                       "namespace",
                       {.stale{{{sourceRepo / "Maths" / "Helper.cpp"}, modification_time::early}},
                         .to_run{{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Source cpps indirectly stale via cpp from dependencies with the same name as a project cpp",
                       projPaths,
                       "namespace",
                       {.stale{{{fooPath / "foo" / "Utilities" / "Helper.cpp"}, modification_time::early}},
                         .to_run{{"Maths/ProbabilityTest.cpp"}, {"Maths/ProbabilityTestingDiagnostics.cpp"}, {"Utilities/UsefulThingsFreeTest.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Stale header in additional project",
                       projPaths,
                       "namespace",
                       {.stale{{{testUtilsPath / "myLib" / "Utils.hpp"}, modification_time::early}},
                        .to_run{{"Maybe/MaybeTest.cpp"}, {"Stuff/OldschoolTest.cpp"}, {"Stuff/OldschoolTestingDiagnostics.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Stale cpp in additional project",
                       projPaths,
                       "namespace",
                       {.stale{{{testUtilsPath / "myLib" / "Utils.cpp"}, modification_time::early}},
                        .to_run{{"Maybe/MaybeTest.cpp"}, {"Stuff/OldschoolTest.cpp"}, {"Stuff/OldschoolTestingDiagnostics.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Materials stale",
                       projPaths,
                       "namespace",
                       {.stale{{{materials / "Stuff" / "FooTest" / "Prediction" / "RepresentativeCasesTemp" / "NoSeqpat" / "baz.txt"}, modification_time::early}},
                         .to_run{{"Stuff/FooTest.cpp"}}},
                       {},
                       {});

    check_tests_to_run("Materials naively stale, but test previously passed (when selected)",
                       projPaths,
                       "namespace",
                       {.stale{{{materials / "Stuff" / "FooTest" / "Prediction" / "RepresentativeCasesTemp" / "NoSeqpat" / "baz.txt"}, modification_time::early}},
                         .to_run{}},
                       {},
                       {{"Stuff/FooTest.cpp", m_ResetTime + to_duration(modification_time::late)}});

    check_tests_to_run("Materials stale; test previously passed (when selected), but materials subsequently modified",
                       projPaths,
                       "namespace",
                       {.stale{{{materials / "Stuff" / "FooTest" / "Prediction" / "RepresentativeCasesTemp" / "NoSeqpat" / "baz.txt"}, modification_time::early}},
                         .to_run{{"Stuff/FooTest.cpp"}}},
                       {},
                       {{"Stuff/FooTest.cpp", m_ResetTime + to_duration(modification_time::very_early)}});

    check_tests_to_run("Materials stale; test previously passed (when selected); materials subsequently modified some early some late",
                       projPaths,
                       "namespace",
                       {.stale{{{materials / "Stuff" / "FooTest" / "Prediction" / "RepresentativeCasesTemp" / "NoSeqpat" / "baz.txt"}, modification_time::early},
                                {{materials / "Stuff" / "FooTest" / "Prediction" / "RepresentativeCasesTemp" / "NoSeqpat" / "baz2.txt"}, modification_time::very_late}},
                         .to_run{{"Stuff/FooTest.cpp"}}},
                       {},
                       {{"Stuff/FooTest.cpp", m_ResetTime + to_duration(modification_time::late)}});

    check_tests_to_run("Nothing stale, but a previous failure",
                       projPaths,
                       "namespace",
                       {.stale{}, .to_run{{"Maths/ProbabilityTest.cpp"}}},
                       {{"Maths/ProbabilityTest.cpp", m_ResetTime + to_duration(modification_time::early)}},
                       {});

    check_tests_to_run("Inconsistency: both passed and failed; failure wins",
                       projPaths,
                       "namespace",
                       {.stale{}, .to_run{{"Maths/ProbabilityTest.cpp"}}},
                       {{"Maths/ProbabilityTest.cpp" , m_ResetTime + to_duration(modification_time::late)}},
                       {{"Maths/ProbabilityTest.cpp", m_ResetTime + to_duration(modification_time::late)}});

    check_tests_to_run("Stale and a previous failure",
                       projPaths,
                       "namespace",
                       {.stale{{{testRepo / "Maths/ProbabilityTest.cpp"}, modification_time::early}}, .to_run{{"Maths/ProbabilityTest.cpp"}}},
                       {{"Maths/ProbabilityTest.cpp", m_ResetTime + to_duration(modification_time::late)}},
                       {});

    check_tests_to_run("Nothing stale, but two previous failures",
                       projPaths,
                       "namespace",
                       {.stale{}, .to_run{{"HouseAllocationTest.cpp"}, {"Maths/ProbabilityTest.cpp"}}},
                       {{"HouseAllocationTest.cpp"  , m_ResetTime + to_duration(modification_time::late)},
                        {"Maths/ProbabilityTest.cpp", m_ResetTime + to_duration(modification_time::late)}},
                       {});

    check_tests_to_run("Ensure that the staleness of a cpp isn't masked by a cpp which has freshly passed",
                       projPaths,
                       "namespace",
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
          tests_to_run(projPaths, ""),
          opt_test_list{test_list{{"HouseAllocationTest.cpp"}}});

    fs::last_write_time(stalePath, m_ResetTime);
    fs::last_write_time(projPaths.prune().stamp(), m_ResetTime + pruneStampOffset);

    check(equality, "Nothing Stale", tests_to_run(projPaths, ""), opt_test_list{test_list{}});
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
          tests_to_run(projPaths, ""),
          opt_test_list{test_list{}});

    fs::last_write_time(testFile, m_ResetTime);
    fs::remove(passesFile);
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
