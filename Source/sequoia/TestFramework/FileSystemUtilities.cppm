////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

module;

#include <array>
#include <concepts>
#include <filesystem>
#include <format>
#include <functional>
#include <iterator>
#include <sstream>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

export module sequoia.test_framework:FileSystemUtilities;

import :CoreInfrastructure;
export import sequoia.core.meta;

/** \file
    \brief File paths and related utilities.
 */



export namespace sequoia::testing
{
  template<>
  struct serializer<std::filesystem::path>
  {
    [[nodiscard]]
    static std::string make(const std::filesystem::path& p);
  };

  template<>
  struct serializer<std::filesystem::file_type>
  {
    [[nodiscard]]
    static std::string make(const std::filesystem::file_type& val);
  };

  void throw_unless_exists(const std::filesystem::path& p, std::string_view message="");

  void throw_unless_directory(const std::filesystem::path& p, std::string_view message="");

  void throw_unless_regular_file(const std::filesystem::path& p, std::string_view message="");

  [[nodiscard]]
  std::filesystem::path find_in_tree(const std::filesystem::path& root, const std::filesystem::path& toFind);

  [[nodiscard]]
  std::filesystem::path rebase_from(const std::filesystem::path& filename, const std::filesystem::path& dir);
}
