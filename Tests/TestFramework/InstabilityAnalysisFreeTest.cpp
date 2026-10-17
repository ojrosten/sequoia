////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "InstabilityAnalysisFreeTest.hpp"
#include "sequoia/TestFramework/InstabilityAnalysis.hpp"
#include "sequoia/TestFramework/InstabilityProjection.hpp"
#include "sequoia/Streaming/Streaming.hpp"

namespace sequoia::testing
{
  namespace
  {
    constexpr auto measurement_zeroing_projection{
      [](std::string_view message) { return instability_projection(message); }
    };

    constexpr auto identity_projection{
      [](std::string_view message) { return std::string{message}; }
    };
  }

  [[nodiscard]]
  std::filesystem::path instability_analysis_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void instability_analysis_free_test::check_exceptions()
  {
    auto analyseMalformed{
      [this]() {
        return instability_analysis(working_materials() / "Malformed", 2, measurement_zeroing_projection);
      }
    };
    check_exception_thrown<std::runtime_error>("A malformed file of failures, named by the instability analysis",
                                               analyseMalformed);
  }

  void instability_analysis_free_test::check_instability_analysis()
  {
    write_to_file(working_materials() / "StableAnalysis.txt",
                  instability_analysis(working_materials() / "Stable", 2, measurement_zeroing_projection),
                  std::ios_base::binary);

    check(
      equivalence,
      "Outputs which agree, beside a file which is not a .txt file",
      working_materials() / "StableAnalysis.txt",
      predictive_materials() / "StableAnalysis.txt"
    );

    write_to_file(working_materials() / "MeasuredValuesAnalysis.txt",
                  instability_analysis(working_materials() / "MeasuredValues", 2, measurement_zeroing_projection),
                  std::ios_base::binary);

    check(
      equivalence,
      "Outputs which differ only in measurements",
      working_materials() / "MeasuredValuesAnalysis.txt",
      predictive_materials() / "MeasuredValuesAnalysis.txt"
    );

    write_to_file(working_materials() / "MeasuredValuesUnprojectedAnalysis.txt",
                  instability_analysis(working_materials() / "MeasuredValues", 2, identity_projection),
                  std::ios_base::binary);

    check(
      equivalence,
      "Outputs which differ only in measurements, compared under the identity projection",
      working_materials() / "MeasuredValuesUnprojectedAnalysis.txt",
      predictive_materials() / "MeasuredValuesUnprojectedAnalysis.txt"
    );

    write_to_file(working_materials() / "VerdictsAnalysis.txt",
                  instability_analysis(working_materials() / "Verdicts", 2, measurement_zeroing_projection),
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
                                       measurement_zeroing_projection),
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
