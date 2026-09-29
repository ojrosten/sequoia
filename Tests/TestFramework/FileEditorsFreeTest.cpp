////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "FileEditorsFreeTest.hpp"
#include "sequoia/TestFramework/FileEditors.hpp"
#include "Utilities/TestUtilities.hpp"

#include "sequoia/Streaming/Streaming.hpp"
#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path file_editors_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void file_editors_free_test::run_tests()
  {
    test_add_include_without_an_existing_block();
    test_add_include_to_an_existing_block();
    test_add_test_registrations();
    test_registrations_added_and_removed();
    test_includes_added_and_removed();
    test_cmake_entries_added_and_removed();
    test_whether_anything_was_removed();
    test_comparison_of_file_contents();
    test_empty_lines_of_a_seqpat();
    test_trailing_spaces_of_a_seqpat_pattern();
    test_refused_lines_of_a_seqpat();
  }

  /// A file with no `#include` anywhere has no block to extend. The include must
  /// still be placed ahead of the first `import`, for the reason given in
  /// add_include: a textual include after an import re-parses what the module
  /// already carried. Appending at the end of the file satisfies neither, and
  /// leaves whatever the header declares undeclared at every point of use.
  void file_editors_free_test::test_add_include_without_an_existing_block()
  {
    const auto file{working_materials() /= "NoBlock/Main.cpp"};
    add_include(file, "Stuff/FooTest.hpp");

    check(equivalence, "Include added to a file with no include block", file, predictive_materials() /= "NoBlock/Main.cpp");
  }

  /// The established behaviour, pinned so the case above cannot be fixed by
  /// breaking it: an existing block is replaced by the sorted union, angled
  /// before quoted.
  void file_editors_free_test::test_add_include_to_an_existing_block()
  {
    const auto file{working_materials() /= "ExistingBlock/Main.cpp"};
    add_include(file, "Stuff/FooTest.hpp");

    check(equivalence, "Include added to an existing include block", file, predictive_materials() /= "ExistingBlock/Main.cpp");
  }

  /** How an existing registration is recognised, and the position, indentation and order of new
      registrations, are not in `add_test_registrations`' contract. The predictions pin the
      implementation's choices, and change with them.
   */
  void file_editors_free_test::test_add_test_registrations()
  {
    auto checkRegistration{
      [this](std::string_view description, const std::filesystem::path& main, const std::vector<std::string>& tests) {
        const auto file{working_materials() /= "Registration" / main};
        add_test_registrations(file, tests);

        check(equivalence, description, file, predictive_materials() /= "Registration" / main);
      }
    };

    checkRegistration("After the last registration, which a blank line separates from the execution",
                      "BlankLineBeforeExecution/Main.cpp",
                      {"gamma_test"});

    checkRegistration("After the last registration, which the execution immediately follows",
                      "NoBlankLine/Main.cpp",
                      {"gamma_test"});

    checkRegistration("After the runner's declaration, with no registration to follow",
                      "NoRegistrations/Main.cpp",
                      {"gamma_test"});

    checkRegistration("Indented with tabs, as the execution",
                      "TabIndentation/Main.cpp",
                      {"gamma_test"});

    checkRegistration("Indented as the execution, not as the registration it follows",
                      "DifferentIndentation/Main.cpp",
                      {"gamma_test"});

    checkRegistration("After an #endif, so that the registration is unconditional",
                      "ConditionalRegistration/Main.cpp",
                      {"gamma_test"});

    checkRegistration("Before a registration which shares the execution's line",
                      "RegistrationOnExecutionLine/Main.cpp",
                      {"gamma_test"});

    checkRegistration("Despite a commented-out registration and one of another runner",
                      "LookalikeRegistrations/Main.cpp",
                      {"gamma_test"});

    checkRegistration("Several tests at once, skipping one already registered",
                      "SeveralTests/Main.cpp",
                      {"gamma_test", "alpha_test", "beta_test"});

    check_exception_thrown<std::runtime_error>(
      "A main with no call to runner.execute",
      [this]() { add_test_registrations(working_materials() /= "Registration/NoExecution/Main.cpp", {"gamma_test"}); }
    );

    check_exception_thrown<std::logic_error>(
      "No tests to register",
      [this]() { add_test_registrations(working_materials() /= "Registration/NoBlankLine/Main.cpp", {}); }
    );
  }

  /** Each state of a main is a node; adding registrations and removing them are the edges between states, so each
      removal is checked to undo the addition it mirrors. The lookalikes - a registration commented out, and one
      with another runner - are not registrations, and survive every removal.
   */
  void file_editors_free_test::test_registrations_added_and_removed()
  {
    using transition_checker_t = transition_checker<std::string, check_ordering::no>;
    using main_graph           = transition_checker_t::transition_graph;
    using edge_t               = transition_checker_t::edge;

    const std::string
      lookalikes{"int main()\n"
                 "{\n"
                 "\trunner.register_test<alpha_test>();\n"
                 "\t// runner.register_test<gamma_test>();\n"
                 "\tmy_runner.register_test<gamma_test>();\n"
                 "\n"
                 "\trunner.execute();\n"
                 "}\n"},
      gamma     {"int main()\n"
                 "{\n"
                 "\trunner.register_test<alpha_test>();\n"
                 "\t// runner.register_test<gamma_test>();\n"
                 "\tmy_runner.register_test<gamma_test>();\n"
                 "\trunner.register_test<gamma_test>();\n"
                 "\n"
                 "\trunner.execute();\n"
                 "}\n"},
      gammaDelta{"int main()\n"
                 "{\n"
                 "\trunner.register_test<alpha_test>();\n"
                 "\t// runner.register_test<gamma_test>();\n"
                 "\tmy_runner.register_test<gamma_test>();\n"
                 "\trunner.register_test<gamma_test>();\n"
                 "\trunner.register_test<delta_test>();\n"
                 "\n"
                 "\trunner.execute();\n"
                 "}\n"};

    auto add{
      [this](std::vector<std::string> tests) {
        return [this, tests](const std::string& text) {
          return edited(text, [&tests](const std::filesystem::path& file) { add_test_registrations(file, tests); });
        };
      }
    };

    auto remove{
      [this](std::vector<std::string> tests) {
        return [this, tests](const std::string& text) {
          return edited(text, [&tests](const std::filesystem::path& file) { remove_test_registrations(file, tests); });
        };
      }
    };

    main_graph g{
      { { edge_t{1, "Register gamma", add({"gamma_test"})},
          edge_t{2, "Register gamma and delta at once", add({"gamma_test", "delta_test"})},
          edge_t{0, "Remove gamma, which only the lookalikes name", remove({"gamma_test"})} }, // 0: lookalikes
        { edge_t{0, "Remove gamma", remove({"gamma_test"})},
          edge_t{2, "Register delta", add({"delta_test"})},
          edge_t{1, "Register gamma again", add({"gamma_test"})} },                            // 1: gamma
        { edge_t{1, "Remove delta", remove({"delta_test"})},
          edge_t{0, "Remove gamma and delta at once", remove({"gamma_test", "delta_test"})} } // 2: gamma and delta
      },
      {lookalikes, gamma, gammaDelta}
    };

    auto checkerFn{
      [this](std::string_view description, const std::string& obtained, const std::string& prediction) {
        check(equality, description, obtained, prediction);
      }
    };

    transition_checker_t::check(report("Registrations added and removed"), g, checkerFn);
  }

  /** Only the line `add_include` writes is removed: an include of a header of the same name in another directory
      stays, as does one written with a comment.
   */
  void file_editors_free_test::test_includes_added_and_removed()
  {
    using transition_checker_t = transition_checker<std::string, check_ordering::no>;
    using header_graph         = transition_checker_t::transition_graph;
    using edge_t               = transition_checker_t::edge;

    const std::string
      withoutBeta{"#include <vector>\n"
                  "#include \"Alpha.hpp\"\n"
                  "#include \"Beta.hpp\" // included by hand\n"
                  "#include \"Other/Beta.hpp\"\n"
                  "\n"
                  "int x{};\n"},
      withBeta   {"#include <vector>\n"
                  "#include \"Alpha.hpp\"\n"
                  "#include \"Beta.hpp\"\n"
                  "#include \"Beta.hpp\" // included by hand\n"
                  "#include \"Other/Beta.hpp\"\n"
                  "\n"
                  "int x{};\n"};

    auto add{
      [this](const std::string& text) {
        return edited(text, [](const std::filesystem::path& file) { add_include(file, "Beta.hpp"); });
      }
    };

    auto remove{
      [this](const std::string& text) {
        return edited(text, [](const std::filesystem::path& file) { remove_include(file, "Beta.hpp"); });
      }
    };

    header_graph g{
      { { edge_t{1, "Include Beta.hpp", add},
          edge_t{0, "Remove Beta.hpp, which only the lookalikes name", remove} }, // 0: without Beta.hpp
        { edge_t{0, "Remove Beta.hpp", remove} }                                             // 1: with Beta.hpp
      },
      {withoutBeta, withBeta}
    };

    auto checkerFn{
      [this](std::string_view description, const std::string& obtained, const std::string& prediction) {
        check(equality, description, obtained, prediction);
      }
    };

    transition_checker_t::check(report("Includes added and removed"), g, checkerFn);
  }

  /** The list is emptied back to the one line it started as, and an entry for a file of the same name in another
      directory is not the entry removed.
   */
  void file_editors_free_test::test_cmake_entries_added_and_removed()
  {
    using transition_checker_t = transition_checker<std::string, check_ordering::no>;
    using cmake_graph          = transition_checker_t::transition_graph;
    using edge_t               = transition_checker_t::edge;

    const std::string
      empty    {"add_executable(TestAll TestMain.cpp)\n"
                "\n"
                "target_sources(TestAll PRIVATE)\n"
                "\n"
                "target_link_libraries(TestAll PRIVATE sequoia)\n"},
      beta     {"add_executable(TestAll TestMain.cpp)\n"
                "\n"
                "target_sources(TestAll PRIVATE\n"
                "               ${TestDir}/Stuff/BetaTest.cpp)\n"
                "\n"
                "target_link_libraries(TestAll PRIVATE sequoia)\n"},
      alphaBeta{"add_executable(TestAll TestMain.cpp)\n"
                "\n"
                "target_sources(TestAll PRIVATE\n"
                "               ${TestDir}/Stuff/AlphaTest.cpp\n"
                "               ${TestDir}/Stuff/BetaTest.cpp)\n"
                "\n"
                "target_link_libraries(TestAll PRIVATE sequoia)\n"},
      alpha    {"add_executable(TestAll TestMain.cpp)\n"
                "\n"
                "target_sources(TestAll PRIVATE\n"
                "               ${TestDir}/Stuff/AlphaTest.cpp)\n"
                "\n"
                "target_link_libraries(TestAll PRIVATE sequoia)\n"},
      elsewhere{"add_executable(TestAll TestMain.cpp)\n"
                "\n"
                "target_sources(TestAll PRIVATE\n"
                "               ${TestDir}/Other/BetaTest.cpp)\n"
                "\n"
                "target_link_libraries(TestAll PRIVATE sequoia)\n"};

    const auto testsDir{working_materials() / "Tests"};

    auto add{
      [this, &testsDir](std::string_view source) {
        return [this, &testsDir, source](const std::string& text) {
          return edited(text, [&testsDir, source](const std::filesystem::path& file) {
              add_to_cmake(file, testsDir, testsDir / source, "target_sources(", ")\n", "${TestDir}/");
            }
          );
        };
      }
    };

    auto remove{
      [this, &testsDir](std::string_view source) {
        return [this, &testsDir, source](const std::string& text) {
          return edited(text, [&testsDir, source](const std::filesystem::path& file) {
              remove_from_cmake(file, testsDir, testsDir / source, "target_sources(", ")\n", "${TestDir}/");
            }
          );
        };
      }
    };

    cmake_graph g{
      { { edge_t{1, "Add BetaTest.cpp to an empty list", add("Stuff/BetaTest.cpp")},
          edge_t{3, "Add AlphaTest.cpp to an empty list", add("Stuff/AlphaTest.cpp")},
          edge_t{0, "Remove BetaTest.cpp from an empty list", remove("Stuff/BetaTest.cpp")} }, // 0: empty
        { edge_t{2, "Add AlphaTest.cpp before BetaTest.cpp", add("Stuff/AlphaTest.cpp")},
          edge_t{0, "Remove the only entry", remove("Stuff/BetaTest.cpp")} },                   // 1: BetaTest.cpp
        { edge_t{1, "Remove the first entry", remove("Stuff/AlphaTest.cpp")},
          edge_t{3, "Remove the last entry", remove("Stuff/BetaTest.cpp")} },                   // 2: both
        { edge_t{2, "Add BetaTest.cpp after AlphaTest.cpp", add("Stuff/BetaTest.cpp")},
          edge_t{0, "Remove the only entry", remove("Stuff/AlphaTest.cpp")} },                  // 3: AlphaTest.cpp
        { edge_t{4, "Remove Stuff/BetaTest.cpp, where only Other/BetaTest.cpp is listed", remove("Stuff/BetaTest.cpp")}
        } // 4: elsewhere
      },
      {empty, beta, alphaBeta, alpha, elsewhere}
    };

    auto checkerFn{
      [this](std::string_view description, const std::string& obtained, const std::string& prediction) {
        check(equality, description, obtained, prediction);
      }
    };

    transition_checker_t::check(report("CMake entries added and removed"), g, checkerFn);
  }

  void file_editors_free_test::test_whether_anything_was_removed()
  {
    const auto file{working_materials() /= "Removal.txt"};

    auto removes{
      [&file](std::string_view text, auto removal) {
        write_to_file(file, text, std::ios_base::out);
        return removal(file);
      }
    };

    auto registrationOfGamma{
      [](const std::filesystem::path& f) { return remove_test_registrations(f, {"gamma_test"}); }
    };

    auto includeOfBeta{[](const std::filesystem::path& f) { return remove_include(f, "Beta.hpp"); }};
    auto entryForBeta{
      [testsDir{working_materials() / "Tests"}](const std::filesystem::path& f) {
        return remove_from_cmake(f, testsDir, testsDir / "BetaTest.cpp", "target_sources(", ")\n", "${TestDir}/");
      }
    };

    constexpr std::string_view
      gammaRegistration{"\trunner.register_test<gamma_test>();\n"},
      betaRegistration {"\trunner.register_test<beta_test>();\n"},
      betaInclude      {"#include \"Beta.hpp\"\n"},
      alphaInclude     {"#include \"Alpha.hpp\"\n"},
      betaEntry        {"target_sources(T PRIVATE\n  ${TestDir}/BetaTest.cpp)\n"},
      emptyList        {"target_sources(T PRIVATE)\n"},
      noList           {"add_executable(T main.cpp)\n"};

    check("A registration removed",           removes(gammaRegistration, registrationOfGamma));
    check("No registration to remove",       !removes(betaRegistration,  registrationOfGamma));
    check("An include removed",               removes(betaInclude,       includeOfBeta));
    check("No include to remove",            !removes(alphaInclude,      includeOfBeta));
    check("An entry removed",                 removes(betaEntry,         entryForBeta));
    check("No entry to remove",              !removes(emptyList,         entryForBeta));
    check("No list to remove an entry from", !removes(noList,            entryForBeta));
  }

  /** The 0x1A checks are aimed at MSVC's text mode, which stops reading at that byte; POSIX text
      mode is binary mode, so there they cannot fail.
   */
  void file_editors_free_test::test_comparison_of_file_contents()
  {
    using namespace std::string_view_literals;

    check("Text differing only in its line endings",   compares_equivalent("alpha\r\nbeta\r\n", "alpha\nbeta\n"));
    check("Text differing in more than that",         !compares_equivalent("alpha\r\nbeta\r\n", "alpha\nbeta\ngamma\n"));
    check("Text which is identical",                   compares_equivalent("alpha\nbeta\n", "alpha\nbeta\n"));

    check("A file with a null byte is compared byte for byte, line endings included",
          !compares_equivalent("\0alpha\r\n"sv, "\0alpha\n"sv));
    check("Two identical files with a null byte compare equivalent",
          compares_equivalent("\0alpha\r\n"sv, "\0alpha\r\n"sv));

    check("Two files differing only after a 0x1A byte",
          !compares_equivalent("head\x1A" "tail", "head\x1A" "different"));
    check("The same, for a file which is not text",
          !compares_equivalent("\0head\x1A" "tail"sv, "\0head\x1A" "different"sv));
  }

  void file_editors_free_test::test_empty_lines_of_a_seqpat()
  {
    const transient_file patterns{working_materials() /= "ContentsUnderComparison.seqpat", "alpha[0-9]\n\nbeta[0-9]\n"};

    check("A pattern after an empty line is applied", compares_equivalent("alpha1 beta1\n", "alpha2 beta2\n"));
  }

  void file_editors_free_test::test_trailing_spaces_of_a_seqpat_pattern()
  {
    const transient_file patterns{working_materials() /= "ContentsUnderComparison.seqpat", "alpha[0-9] \n"};

    check("A pattern's trailing space is part of the pattern", compares_equivalent("alpha1 text\n", "text\n"));
  }

  void file_editors_free_test::test_refused_lines_of_a_seqpat()
  {
    auto checkRefusal{
      [this](const reporter& description, std::string_view seqpatContents) {
        const transient_file patterns{working_materials() /= "ContentsUnderComparison.seqpat", seqpatContents};

        check_exception_thrown<std::runtime_error>(
          description,
          [this]() { return compares_equivalent("text\n", "text\n"); }
        );
      }
    };

    checkRefusal("A line of spaces is refused, naming its line", "alpha[0-9]\n\n  \nbeta[0-9]\n");
    checkRefusal("A line holding only a tab is refused, naming its line", "alpha[0-9]\n\t\n");
    checkRefusal("An invalid pattern is refused, naming its line, empty lines counted", "alpha[0-9]\n\n(\n");
  }

  template<std::invocable<const std::filesystem::path&> Edit>
  [[nodiscard]]
  std::string file_editors_free_test::edited(const std::string& text, Edit edit) const
  {
    const transient_file file{working_materials() /= "Edited.txt", text};
    edit(file.path());
    return read_to_string(file.path(), std::ios_base::in).value();
  }

  [[nodiscard]]
  bool file_editors_free_test::compares_equivalent(std::string_view working, std::string_view prediction) const
  {
    const auto dir{working_materials()};
    const transient_file workingFile{dir / "ContentsUnderComparison.working", working},
                         predictionFile{dir / "ContentsUnderComparison.prediction", prediction};

    const auto contents{get_reduced_file_content(workingFile.path(), predictionFile.path())};

    return contents.working.value() == contents.prediction.value();
  }
}
