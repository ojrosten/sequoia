////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief File paths and related utilities.
 */

#include "sequoia/TestFramework/CoreInfrastructure.hpp"

#include <filesystem>

namespace sequoia::testing
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

  /** \brief A path naming something beneath `dir`, spelt from `dir` or from an ancestor of
      it, made relative to `dir`.

      A relative path may begin with the components it shares with the end of `dir`; the
      longest such prefix is taken to be shared, so a path that could have been spelt from
      more than one ancestor is read from the outermost. A component matching elsewhere in
      `dir` is not shared. Leading `..` components are discarded, and a path comprising
      nothing else throws, as does an empty `dir` or one which exists and is not a directory.
      An absolute path is made relative to an absolute `dir` by `std::filesystem::relative`,
      which consults the filesystem; everything else is lexical.
   */
  [[nodiscard]]
  std::filesystem::path rebase_from(const std::filesystem::path& p, const std::filesystem::path& dir);
}
