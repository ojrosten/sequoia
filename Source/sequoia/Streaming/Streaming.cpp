////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/Streaming/Streaming.hpp"

#include <fstream>
#include <iterator>
#include <ranges>
#include <limits>
#include <system_error>
#include <utility>

namespace sequoia
{
  [[nodiscard]]
  std::string report_failed_read(const std::filesystem::path& file)
  {
    return std::string{"Unable to open file "}.append(file.generic_string()).append(" for reading\n");
  }

  [[nodiscard]]
  std::string report_failed_write(const std::filesystem::path& file)
  {
    return std::string{"Unable to write to file "}.append(file.generic_string()).append("\n");
  }

  [[nodiscard]]
  std::optional<std::string> read_to_string(const std::filesystem::path& file, std::ios_base::openmode mode)
  {
    std::error_code error{};
    const auto size{std::filesystem::file_size(file, error)};
    if(error)
      return std::nullopt;

    if(std::ifstream ifile{file, mode})
    {
      /* Sized from the file, then cut to what arrived: in text mode a platform may deliver fewer characters
         than the file holds, as Windows does for CRLF. The size is the filesystem's rather than `tellg`'s,
         which in text mode on MSVC does not give the size of an LF file.
       */
      std::string text(size, '\0');
      ifile.read(text.data(), static_cast<std::streamsize>(size));
      text.resize(static_cast<std::size_t>(ifile.gcount()));

      return text;
    }

    return std::nullopt;
  }

  std::istream& peek_for_more(std::istream& s)
  {
    if(s.peek() == std::char_traits<char>::eof())
      s.setstate(std::ios_base::failbit);

    return s;
  }

  [[nodiscard]]
  std::string extract_text(std::istream& s, std::size_t length)
  {
    if(!s)
      throw std::runtime_error{"Text cannot be read from a stream which has failed"};

    using iter_t  = std::istreambuf_iterator<char>;
    using count_t = std::iter_difference_t<iter_t>;
    if(std::cmp_greater(length, std::numeric_limits<count_t>::max()))
      throw std::runtime_error{std::format("A length of {} is more characters than a stream iterator can count", length)};

    const auto text{
        std::ranges::subrange{iter_t{s}, iter_t{}}
      | std::views::take(static_cast<count_t>(length))
      | std::ranges::to<std::string>()
    };

    if(text.size() < length)
      throw std::runtime_error{std::format("Expected {} characters but the stream ended after {}", length, text.size())};

    if(s.get() != '\n')
      throw std::runtime_error{std::format("Expected a line break after {} characters", length)};

    return text;
  }

  [[nodiscard]]
  bool try_close(std::ofstream& stream)
  {
    stream.close();
    return !stream.fail();
  }

  void throw_unless_closed(std::ofstream& stream, const std::filesystem::path& file)
  {
    if(!try_close(stream))
      throw std::runtime_error{report_failed_write(file)};
  }

  [[nodiscard]]
  bool try_write_to_file(const std::filesystem::path& file, std::string_view text, std::ios_base::openmode mode)
  {
    // A stream which fails to open is failed already, so the write does nothing and `try_close` reports the failure
    std::ofstream ofile{file, mode};
    ofile.write(text.data(), static_cast<std::streamsize>(text.size()));
    return try_close(ofile);
  }

  void write_to_file(const std::filesystem::path& file, std::string_view text, std::ios_base::openmode mode)
  {
    if(!try_write_to_file(file, text, mode))
      throw std::runtime_error{report_failed_write(file)};
  }
}
