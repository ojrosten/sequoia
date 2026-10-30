////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

module;

#include "sequoia/PlatformSpecific/Macros.hpp"

module sequoia.test_framework;

import std;

import sequoia.streaming;

namespace sequoia::testing
{
  [[nodiscard]]
  std::string to_string(const failure_info& info)
  {
    return std::format("check: {}\nlength: {}\n{}\n", info.check_index, info.message.size(), info.message);
  }

  std::ostream& operator<<(std::ostream& s, const failure_info& info)
  {
    return s << to_string(info);
  }

  std::istream& operator>>(std::istream& s, failure_info& info)
  {
    if(!peek_for_more(s))
      return s;

    auto toCheckIndex{[](const std::string& text) { return parse_integer<std::size_t>(text, "a check index"); }};
    auto toLength    {[](const std::string& text) { return parse_integer<std::size_t>(text, "a length"); }};

    info = failure_info{
      .check_index{extract_field(s, "check: ", toCheckIndex)},
      .message{extract_text(s, extract_field(s, "length: ", toLength))}
    };

    return s;
  }

  [[nodiscard]]
  std::string to_string(const failure_output& output)
  {
    auto infoText{[](const failure_info& info) { return to_string(info); }};

    return
        output
      | std::views::transform(infoText)
      | std::views::join
      | std::ranges::to<std::string>();
  }

  std::ostream& operator<<(std::ostream& s, const failure_output& output)
  {
    return s << to_string(output);
  }

  std::istream& operator>>(std::istream& s, failure_output& output)
  {
    using iter_t = std::istream_iterator<failure_info>;
    output = std::ranges::subrange{iter_t{s}, iter_t{}} | std::ranges::to<failure_output>();
    return s;
  }
}
