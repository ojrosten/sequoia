////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Utilities for reading/writing to files.
 */

#include "sequoia/Core/Meta/Concepts.hpp"

#include <charconv>
#include <filesystem>
#include <format>
#include <ios>
#include <istream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace sequoia
{
  [[nodiscard]]
  std::string report_failed_read(const std::filesystem::path& file);

  [[nodiscard]]
  std::string report_failed_write(const std::filesystem::path& file);


  /** \brief The contents of a regular file, read in `mode`.

      \returns `std::nullopt` if the file cannot be opened, or its size cannot be read.
   */
  [[nodiscard]]
  std::optional<std::string> read_to_string(const std::filesystem::path& file, std::ios_base::openmode mode);

  /** \brief Closes `stream`, and returns `false` if the stream has failed:
             to open, in any operation on it, or in closing.

      Only a caller of `close` learns whether closing failed; the destructor
      reports nothing. A networked filesystem may report a full disk, an
      exceeded quota or an I/O error only then. The destructor still closes a
      stream which an exception leaves open.
   */
  [[nodiscard]]
  bool try_close(std::ofstream& stream);

  /** \brief Closes `stream`, which writes to `file`.

      \throws std::runtime_error naming `file`, if `stream` has failed: to
              open, in any operation on it, or in closing
   */
  void throw_unless_closed(std::ofstream& stream, const std::filesystem::path& file);

  /** \brief Writes `text` to `file`, opened in `mode`, and returns whether
             the open, the write and the close all succeeded.
   */
  [[nodiscard]]
  bool try_write_to_file(const std::filesystem::path& file, std::string_view text, std::ios_base::openmode mode);

  /** \brief Writes `text` to `file`, opened in `mode`.

      \throws std::runtime_error naming `file`, if the open, the write or the
              close fails
   */
  void write_to_file(const std::filesystem::path& file, std::string_view text, std::ios_base::openmode mode);

  /** \brief Peeks at `s`, consuming nothing, and sets `failbit` on `s` if no character can be peeked.

      \returns `s`, which converts to `false` if no character could be peeked.
   */
  std::istream& peek_for_more(std::istream& s);

  /** \brief Reads the next line of `s`, and returns `parse` applied to the rest of the line after `key`.

      \throws std::runtime_error if `s` has no next line, or the line does not begin with `key`. Whatever `parse`
      throws propagates.
   */
  template<std::invocable<std::string> Parser>
  [[nodiscard]]
  auto extract_field(std::istream& s, std::string_view key, Parser parse)
  {
    std::string line{};
    if(!std::getline(s, line))
      throw std::runtime_error{std::format("Expected a line beginning '{}' but there are no more lines", key)};

    if(!line.starts_with(key))
      throw std::runtime_error{std::format("Expected a line beginning '{}' but found '{}'", key, line)};

    return parse(line.substr(key.size()));
  }

  /** \brief Reads the next `length` characters of `s`, then a line break, and returns the characters.

      \throws std::runtime_error if
      -# `s` has failed;
      -# `length` is more characters than a stream iterator can count;
      -# Fewer than `length` characters remain, or they are not followed by a line break.
   */
  [[nodiscard]]
  std::string extract_text(std::istream& s, std::size_t length);

  /** \brief Parses the whole of `text` as an integer of type `T`: an optional `-`, then decimal digits.

      \throws std::runtime_error if `text` is not, in its entirety, such an integer representable as `T`. The
      message says that `text` is not `description`.
   */
  template<integer T>
  [[nodiscard]]
  T parse_integer(std::string_view text, std::string_view description)
  {
    const auto last{text.data() + text.size()};
    if(T value{}; std::from_chars(text.data(), last, value) == std::from_chars_result{last, std::errc{}})
      return value;

    throw std::runtime_error{std::format("'{}' is not {}", text, description)};
  }

  template<std::invocable<std::string&> Fn>
  void read_modify_write(const std::filesystem::path& file, Fn fn)
  {
    if(auto text{read_to_string(file, std::ios_base::in)})
    {
      fn(*text);
      write_to_file(file, *text, std::ios_base::out);
    }
    else
    {
      throw std::runtime_error{report_failed_read(file)};
    }
  }
}
