////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "InstabilityAnalysisFreeTest.hpp"
#include "sequoia/TestFramework/InstabilityAnalysis.hpp"
#include "sequoia/TestFramework/TestRunner.hpp"
#include "sequoia/Streaming/Streaming.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path instability_analysis_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void instability_analysis_free_test::check_exceptions()
  {
    auto instabilityProjection{[](std::string_view message) { return instability_projection(message); }};

    auto analyseMalformed{
      [this, instabilityProjection]() {
        return instability_analysis(working_materials() / "Malformed", 2, instabilityProjection);
      }
    };
    check_exception_thrown<std::runtime_error>("A malformed file of failures, named by the instability analysis",
                                               analyseMalformed);
  }

  void instability_analysis_free_test::check_instability_analysis()
  {
    auto instabilityProjection{[](std::string_view message) { return instability_projection(message); }};

    write_to_file(working_materials() / "StableAnalysis.txt",
                  instability_analysis(working_materials() / "Stable", 2, instabilityProjection),
                  std::ios_base::binary);

    check(
      equivalence,
      "Outputs which agree, beside a file which is not a .txt file",
      working_materials() / "StableAnalysis.txt",
      predictive_materials() / "StableAnalysis.txt"
    );

    write_to_file(working_materials() / "MeasuredValuesAnalysis.txt",
                  instability_analysis(working_materials() / "MeasuredValues", 2, instabilityProjection),
                  std::ios_base::binary);

    check(
      equivalence,
      "Outputs which differ only in measurements",
      working_materials() / "MeasuredValuesAnalysis.txt",
      predictive_materials() / "MeasuredValuesAnalysis.txt"
    );

    auto unchanged{[](std::string_view message) { return std::string{message}; }};

    write_to_file(working_materials() / "MeasuredValuesUnprojectedAnalysis.txt",
                  instability_analysis(working_materials() / "MeasuredValues", 2, unchanged),
                  std::ios_base::binary);

    check(
      equivalence,
      "Outputs which differ only in measurements, compared with no projection",
      working_materials() / "MeasuredValuesUnprojectedAnalysis.txt",
      predictive_materials() / "MeasuredValuesUnprojectedAnalysis.txt"
    );

    write_to_file(working_materials() / "VerdictsAnalysis.txt",
                  instability_analysis(working_materials() / "Verdicts", 2, instabilityProjection),
                  std::ios_base::binary);

    check(
      equivalence,
      "Outputs which differ in a verdict, as well as in measurements",
      working_materials() / "VerdictsAnalysis.txt",
      predictive_materials() / "VerdictsAnalysis.txt"
    );

    write_to_file(working_materials() / "VerdictsAfterMeasuredValuesAnalysis.txt",
                  instability_analysis(working_materials() / "VerdictsAfterMeasuredValues",
                                       3,
                                       instabilityProjection),
                  std::ios_base::binary);

    check(
      equivalence,
      "Outputs which differ in a verdict, after a failure which differs only in measurements",
      working_materials() / "VerdictsAfterMeasuredValuesAnalysis.txt",
      predictive_materials() / "VerdictsAfterMeasuredValuesAnalysis.txt"
    );
  }

  void instability_analysis_free_test::run_tests()
  {
    check_exceptions();
    check_instability_analysis();
  }
}
