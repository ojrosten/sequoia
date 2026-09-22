////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

export module sequoia.test_framework:FileSystemUtilities;

import std;

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

  /** \brief `p` made relative to `dir`.

      A relative `p` - a path typed at the command line, or a `source_location` from a compiler
      given a relative source - was spelt from a directory this function cannot know. The
      function assumes `p` names something beneath `dir`, and takes the reading on which `p`
      does. So, unlike `std::filesystem::relative`, the result never begins with `..`. A `p`
      whose leading components name no directory - the output of `-ffile-prefix-map`, say - has
      no such reading, and comes back unchanged.

      \returns One of:
      -# The empty path, if `p` is empty;
      -# `std::filesystem::relative(p, dir)`, if both are absolute;
      -# Otherwise, lexically, `p` less any leading `..` components and less the longest proper
         prefix of what remains which is also a suffix of `dir`. A trailing separator on either
         argument is ignored.

      \throws std::runtime_error, for a non-empty `p`, if:
      -# `dir` is empty, or exists and is not a directory;
      -# `p` comprises nothing but `..` components.

      \throws std::filesystem::filesystem_error if the filesystem cannot be consulted.
   */
  [[nodiscard]]
  std::filesystem::path rebase_from(const std::filesystem::path& p, const std::filesystem::path& dir);
}
