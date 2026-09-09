////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief The name of a type, recovered at compile time.
 */

#include <source_location>
#include <string_view>

namespace sequoia::meta
{
  namespace impl
  {
    using trial_type = void;
    constexpr std::string_view trial_type_name{"void"};

    namespace wrapped_type
    {
      template <typename T>
      [[nodiscard]]
      constexpr std::string_view name() noexcept
      {
        return std::source_location::current().function_name();
      }

      [[nodiscard]]
      constexpr std::size_t prefix_length() noexcept
      {
        return name<trial_type>().find(trial_type_name);
      }

      [[nodiscard]]
      constexpr std::size_t suffix_length() noexcept
      {
        return name<trial_type>().length() - prefix_length() - trial_type_name.length();
      }
    }
  }

  template<class T>
  [[nodiscard]]
  consteval std::string_view type_name()
  {
    using namespace impl::wrapped_type;
    constexpr auto wrappedName{name<T>()};
    constexpr auto prefixLength{prefix_length()};
    constexpr auto nameLength{wrappedName.length() - prefixLength - suffix_length()};
    return wrappedName.substr(prefixLength, nameLength);
  }

  /** \brief Removes the `class`, `struct` or `enum` which MSVC prefixes to a type's name.

      clang and gcc give the bare name, so `type_name` alone is spelt three ways for two compilers
      and one; this makes the leading keyword agree. The trial type used to calibrate `type_name`
      is `void`, which takes no such keyword and therefore cannot expose the difference.

      Two things it does not do. Fundamental types remain divergent - gcc spells `unsigned long` as
      `long unsigned int` - so this does not make an arbitrary type name canonical; for that, at
      runtime and over demangled names, see `testing::tidy_name`. And only the *leading* keyword is
      removed, so a specialization keeps the ones among its arguments.

      Where it bites is narrower than it looks: a name carrying a namespace loses the keyword anyway
      when the qualification is stripped. What it protects is a type declared at global scope, which
      a client's tests may well be, a namespace being optional in a generated project.
   */

  [[nodiscard]]
  constexpr std::string_view tidy_type_name(std::string_view name) noexcept
  {
    for(auto keyword : {std::string_view{"class "}, std::string_view{"struct "}, std::string_view{"enum "}})
      if(name.starts_with(keyword)) return name.substr(keyword.size());

    return name;
  }
}
