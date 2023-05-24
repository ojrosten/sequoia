////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "PathFreeDiagnostics.hpp"
#include "sequoia/TestFramework/ConcreteTypeCheckers.hpp"

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    struct dummy_file_comparer
    {
      template<test_mode Mode>
      void operator()(test_logger<Mode>&, const std::filesystem::path&, const std::filesystem::path&) const
      {}
    };

    using bespoke_file_checker_t = general_file_checker<string_based_file_comparer, dummy_file_comparer>;

    const bespoke_file_checker_t bespoke_file_checker{{".*", ".ignore"}};

    const general_equivalence_check_t<bespoke_file_checker_t>      bespoke_path_equivalence{bespoke_file_checker};
    const general_weak_equivalence_check_t<bespoke_file_checker_t> bespoke_path_weak_equivalence{bespoke_file_checker};
  }

  log_summary& postprocess(log_summary& summary, const std::filesystem::path& projectRoot)
  {
    std::string updatedOutput{summary.diagnostics_output()};

    replace_all(updatedOutput, projectRoot.generic_string() + "/", "");

    summary.diagnostics_output(updatedOutput);

    return summary;
  }

  [[nodiscard]]
  std::filesystem::path path_false_positive_free_diagnostics::source_file() const
  {
    return std::source_location::current().file_name();
  }

  [[nodiscard]]
  log_summary path_false_positive_free_diagnostics::summarize(duration delta) const
  {
    auto summary{free_false_positive_test::summarize(delta)};
    return postprocess(summary, project_root());
  }

  void path_false_positive_free_diagnostics::run_tests()
  {
    test_paths();
  }

  
  void path_false_positive_free_diagnostics::test_paths()
  {
    check(equivalence,
          report_line("Inequivalence of two different paths, neither of which exists"),
          working_materials().append("Stuff/Blah"),
          working_materials().append("Stuff/Blurg"));

    check(equivalence,
          report_line("Inequivalence of two different paths, one of which exists"),
          working_materials().append("Stuff/Blah"),
          working_materials().append("Stuff/A"));

    check(equivalence,
          report_line("Inequivalence of directory/file"),
          working_materials().append("Stuff/A"),
          working_materials().append("Stuff/A/foo.txt"));

    check(equivalence,
          report_line("Inequivalence of differently named files"),
          working_materials().append("Stuff/B/foo.txt"),
          working_materials().append("Stuff/B/bar.txt"));

    check(equivalence,
          report_line("Inequivalence of file contents"),
          working_materials().append("Stuff/A/foo.txt"),
          working_materials().append("Stuff/B/foo.txt"));

    check(equivalence,
          report_line("Inequivalence of differently named directories with the same contents"),
          working_materials().append("Stuff/A"),
          working_materials().append("Stuff/C"));

    check(equivalence,
          report_line("Inequivalence of directories with the same files but different contents"),
          working_materials().append("Stuff/A"),
          working_materials().append("MoreStuff/A"));

    check(equivalence,
          report_line("Inequivalence of directories with some common files"),
          working_materials().append("Stuff/B"),
          working_materials().append("MoreStuff/B"));

    check(equivalence,
          report_line("Inequivalence of directories with some common files"),
          working_materials().append("MoreStuff/B"),
          working_materials().append("Stuff/B"));

    check(equivalence,
          report_line("File inequivalence when default file checking is used"),
          working_materials().append("CustomComparison/A/DifferingContent.ignore"),
          working_materials().append("CustomComparison/B/DifferingContent.ignore"));

    check(equivalence,
          report_line("Range inequivalence when default file checking us used"),
          std::vector<fs::path>{working_materials().append("CustomComparison/A/DifferingContent.ignore")},
          std::vector<fs::path>{working_materials().append("CustomComparison/B/DifferingContent.ignore")});

    check(weak_equivalence,
          report_line("Weak inequivalence of directories with some common files"),
          working_materials().append("MoreStuff/B"),
          working_materials().append("Stuff/B"));

    check(weak_equivalence,
          report_line("Directory weak inequivalence when default file checking is used"),
          working_materials().append("CustomComparison/A"),
          working_materials().append("CustomComparison/B"));

    check(weak_equivalence,
          report_line("Weak inequivalence of range when default file checking is used"),
          std::vector<fs::path>{{working_materials().append("CustomComparison/A")}},
          std::vector<fs::path>{{working_materials().append("CustomComparison/B")}});
  }
  
  [[nodiscard]]
  std::filesystem::path path_false_negative_free_diagnostics::source_file() const
  {
    return std::source_location::current().file_name();
  }

  [[nodiscard]]
  log_summary path_false_negative_free_diagnostics::summarize(duration delta) const
  {
    auto summary{free_false_negative_test::summarize(delta)};
    return postprocess(summary, project_root());
  }

  void path_false_negative_free_diagnostics::run_tests()
  {
    test_paths();
  }

  
  void path_false_negative_free_diagnostics::test_paths()
  {
    check(equivalence,
          report_line("Equivalence of a file to itself"),
          working_materials().append("Stuff/A/foo.txt"),
          working_materials().append("Stuff/A/foo.txt"));

    check(equivalence,
          report_line("Equivalence of a directory to itself"),
          working_materials().append("Stuff/A"),
          working_materials().append("Stuff/A"));

    check(equivalence,
          report_line("Equivalence of a directory, with sub-directories to itself"),
          working_materials().append("Stuff"),
          working_materials().append("Stuff"));

    check(equivalence,
          report_line("Equivalence of identical directories in different locations"),
          working_materials().append("Stuff/C"),
          working_materials().append("SameStuff/C"));

    check(bespoke_path_equivalence,
          report_line("File equivalence when .ignore is ignored"),
          working_materials().append("CustomComparison/A/DifferingContent.ignore"),
          working_materials().append("CustomComparison/B/DifferingContent.ignore"));

    check(bespoke_path_equivalence,
          report_line("Range equivalence when .ignore is ignored"),
          std::vector<fs::path>{working_materials().append("CustomComparison/A/DifferingContent.ignore")},
          std::vector<fs::path>{working_materials().append("CustomComparison/B/DifferingContent.ignore")});

    check(weak_equivalence,
         report_line("Weak equivalence of directories in with the same contents but different names"),
         working_materials().append("Stuff"),
         working_materials().append("SameStuff"));

    check(bespoke_path_weak_equivalence,
          report_line("Weak equivalence when .ignore is ignored"),
          working_materials().append("CustomComparison/A"),
          working_materials().append("CustomComparison/B"));

    check(bespoke_path_weak_equivalence,
          report_line("Weak equivalence of range when .ignore is ignored"),
          std::vector<fs::path>{{working_materials().append("CustomComparison/A")}},
          std::vector<fs::path>{{working_materials().append("CustomComparison/B")}});
  }
}
