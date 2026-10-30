////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

module sequoia.parsing;

import std;

namespace sequoia::parsing::commandline
{
  namespace
  {
    [[nodiscard]]
    std::string make(std::string_view type, std::string_view message, const indentation& indent)
    {
      return std::format("{}{}: {}", std::string_view{indent}, type, message);
    }

    [[nodiscard]]
    std::string make(std::string_view type, std::initializer_list<std::string_view> messages, const indentation& indent)
    {
      if(std::ranges::empty(messages)) return {};

      // Continuation lines start beneath the first message, past "<type>: "
      const auto continuationIndent{indent + indentation{std::string(type.size() + 2, ' ')}};

      auto appendContinuation{
        [&continuationIndent](std::string mess, std::string_view message) -> std::string {
          append_indented(mess, message, continuationIndent);
          return mess;
        }
      };

      return std::ranges::fold_left(messages | std::views::drop(1),
                                    make(type, *messages.begin(), indent),
                                    appendContinuation)
               .append(2, '\n');
    }
  }

  [[nodiscard]]
  std::string error(std::string_view message, indentation indent)
  {
    return make("Error", message, indent);
  }

  [[nodiscard]]
  std::string error(std::initializer_list<std::string_view> messages, indentation indent)
  {
    return make("Error", messages, indent);
  }

  [[nodiscard]]
  std::string warning(std::string_view message, indentation indent)
  {
    return make("Warning", message, indent);
  }

  [[nodiscard]]
  std::string warning(std::initializer_list<std::string_view> messages, indentation indent)
  {
    return make("Warning", messages, indent);
  }
}
