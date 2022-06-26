////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for PerformanceTestCore.hpp
*/

#include "sequoia/TestFramework/PerformanceTestCore.hpp"

namespace sequoia::testing
{
  namespace
  {
    [[nodiscard]]
    bool acceptable_mismatch(std::string_view testOutput, std::string_view referenceOutput)
    {
      auto iters{std::mismatch(testOutput.begin(), testOutput.end(), referenceOutput.begin(), referenceOutput.end())};
      if((iters.first != testOutput.end()) && (iters.second != referenceOutput.end()))
      {
        constexpr auto npos{std::string::npos};
        const auto pos{std::distance(testOutput.begin(), iters.first)};
        if(testOutput.rfind("Task duration:", pos) != npos)
        {
          const auto endLine{testOutput.find('\n', pos)};
          const auto refEndLine{referenceOutput.find('\n', pos)};

          if((endLine != npos) && (refEndLine != npos))
          {
            std::string_view lineView{testOutput.substr(pos, endLine - pos)};
            std::string_view refLineView{referenceOutput.substr(pos, refEndLine - pos)};

            auto acceptable{
              [=](std::string_view open, std::string_view close){
                const auto openPos{lineView.find(open)};
                const auto closePos{lineView.find(close)};
                const auto refOpenPos{refLineView.find(open)};
                const auto refClosePos{refLineView.find(close)};
                if((closePos != npos) && (openPos < closePos) && (refClosePos != npos) &&(refOpenPos < refClosePos))
                {
                  const auto[lineIter, refLineIter]{
                          std::mismatch(lineView.begin() + openPos, lineView.begin() + closePos,
                                        refLineView.begin() + refOpenPos, refLineView.begin() + refClosePos)};

                  return (lineIter == (lineView.begin() + closePos)) && (refLineIter == (refLineView.begin() + refClosePos));
                }

                return true;
              }
            };

            if(!acceptable("-", "*") || !acceptable("(", ")"))
              return false;

            return acceptable_mismatch(testOutput.substr(endLine), referenceOutput.substr(refEndLine));
          }
        }
      }

      return (iters.first == testOutput.end()) && (iters.second == referenceOutput.end());
    }
  }

  [[nodiscard]]
  std::string_view postprocess(std::string_view testOutput, std::string_view referenceOutput)
  {
    return acceptable_mismatch(testOutput, referenceOutput) ? referenceOutput : testOutput;
  }

  template<test_mode Mode>
  [[nodiscard]]
  log_summary basic_performance_test<Mode>::summarize(duration delta) const
  {
    auto summary{base_t::summarize(delta)};

    if constexpr(Mode != test_mode::standard)
    {
      const auto referenceOutput{
        [filename{this->diagnostics_output_filename()}]() -> std::string {
          if(std::filesystem::exists(filename))
          {
            if(auto contents{read_to_string(filename)})
              return contents.value();

            throw std::runtime_error{report_failed_read(filename)};
          }

          return "";
        }()
      };

      std::string outputToUse{postprocess(summary.diagnostics_output(), referenceOutput)};
      summary.diagnostics_output(std::move(outputToUse));
    }

    return summary;
  }

  template class basic_performance_test<test_mode::standard>;
  template class basic_performance_test<test_mode::false_positive>;
  template class basic_performance_test<test_mode::false_negative>;
}
