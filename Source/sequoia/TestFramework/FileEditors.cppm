////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

export module sequoia.test_framework:FileEditors;

import std;

export import sequoia.core.meta;

/** \file
    \brief Contains utilities for automatically editing certain files as part of the test creation process.
 */

export namespace sequoia::testing
{
  void add_include(const std::filesystem::path& file, std::string_view includePath);

  /** \brief A CMake command's name, and the arguments which precede its entries */
  struct cmake_command
  {
    std::string_view name{}, leading_arguments{};
  };

  /** \brief A file to add to a CMake list file, the directory its entry is relative to, and how the list file
      spells that directory
   */
  struct cmake_entry
  {
    std::filesystem::path file_to_add{}, directory{};
    std::string_view directory_spelling{};
  };

  /** \brief Adds `entry` to the entries of the first invocation of `command` in `cmakeLists`.

      The invocation is the first place `cmakeLists` spells the command's name, an opening parenthesis and the
      leading arguments. Its entries are the lines which follow, up to the first `)` that ends a line. The new
      entry is the path of `entry.file_to_add` relative to `entry.directory`, joined to `entry.directory_spelling`.
      The entries are then sorted. Each is indented by one more than the length of the command's name, so that it
      aligns after the parenthesis of a command which starts its line.

      \throws std::runtime_error if `cmakeLists` cannot be read, contains no such invocation with its closing `)`, or
      cannot be opened to write
   */
  void add_to_cmake(const std::filesystem::path& cmakeLists,
                    const cmake_command& command,
                    const cmake_entry& entry);

  /** \brief Registers each of `tests` in `file`, skipping any already registered there.

      \throws std::logic_error if `tests` is empty.
      \throws std::runtime_error if `file` cannot be read, does not contain `runner.execute`, or cannot be
              opened to write a registration.
   */
  void add_test_registrations(const std::filesystem::path& file, const std::vector<std::string>& tests);

  struct reduced_file_contents
  {
    std::optional<std::string> working, prediction;
  };

  [[nodiscard]]
  reduced_file_contents get_reduced_file_content(const std::filesystem::path& file, const std::filesystem::path& prediction);

}
