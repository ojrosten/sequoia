////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief A collection of functions for formatting test output. 
 */

#include <string>

namespace sequoia::testing
{
  [[nodiscard]]
  std::string merge(std::string_view s1, std::string_view s2, std::string_view sep=" ");

  [[nodiscard]]
  std::string indent(std::string_view s, std::string_view space="\t");

  //[[nodiscard]]
  //std::string indent(std::string_view s1, std::string_view s2, std::string_view space="\t");

  std::string& indent_after(std::string& s1, std::string_view s2, std::string_view space="\t");

  [[nodiscard]]
  std::string make_message(std::string_view tag, std::string_view currentMessage, std::string_view exceptionMessage, const bool exceptionsDetected);

  [[nodiscard]]
  std::string operator_message(std::string_view description, std::string_view op, std::string_view retVal);

  [[nodiscard]]
  std::string prediction_message(std::string_view obtained, std::string_view predicted, std::string_view advice="");

  [[nodiscard]]
  std::string report_line(std::string_view file, const int line, const std::string_view message);

  #define LINE(message) report_line(__FILE__, __LINE__, message)   
}
