////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

module sequoia.streaming;

import std;

namespace sequoia
{
  [[nodiscard]]
  std::string report_failed_read(const std::filesystem::path& file)
  {
    return std::format("Unable to open file {} for reading\n", file.generic_string());
  }

  [[nodiscard]]
  std::string report_failed_write(const std::filesystem::path& file)
  {
    return std::format("Unable to write to file {}\n", file.generic_string());
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

  namespace
  {
    /** \brief How far a write got: its open failed, its write or close
               failed, or all three succeeded.
     */
    enum class write_outcome { open_failed, write_failed, written };

    /** \brief Writes `text` to `file`, opened in `mode`. */
    [[nodiscard]]
    write_outcome
      try_write_to_file(const std::filesystem::path& file, std::string_view text, std::ios_base::openmode mode)
    {
      std::ofstream ofile{file, mode};
      if(!ofile.is_open())
        return write_outcome::open_failed;

      ofile.write(text.data(), static_cast<std::streamsize>(text.size()));

      // Only a caller of close learns whether closing failed; the destructor
      // reports nothing. A networked filesystem may report a full disk, an
      // exceeded quota or an I/O error only then.
      ofile.close();
      return ofile.fail() ? write_outcome::write_failed : write_outcome::written;
    }

    /** \brief Renames `from` over `to`.

        \returns `to` if the rename failed, and `std::nullopt` otherwise.
     */
    [[nodiscard]]
    std::optional<std::filesystem::path> try_rename(const std::filesystem::path& from, const std::filesystem::path& to)
    {
      std::error_code error{};
      std::filesystem::rename(from, to, error);
      return error ? std::optional{to} : std::nullopt;
    }

    /** \brief A path at which no directory entry exists, for a temporary file
               beside `file`.
     */
    [[nodiscard]]
    std::filesystem::path unused_partial_path(const std::filesystem::path& file)
    {
      auto partialPath{
        [&file](std::size_t n) {
          return std::filesystem::path{file} += ((n == 0) ? std::string{".partial"} : std::format(".{}.partial", n));
        }
      };

      auto namesAnEntry{
        [](const std::filesystem::path& path) {
          std::error_code selectsTheNonThrowingOverload{};
          return std::filesystem::exists(std::filesystem::symlink_status(path, selectsTheNonThrowingOverload));
        }
      };

      auto unused{
          std::views::iota(0uz)
        | std::views::transform(partialPath)
        | std::views::filter(std::not_fn(namesAnEntry))
      };

      return unused.front();
    }
  }

  void write_to_file(const std::filesystem::path& file, std::string_view text, std::ios_base::openmode mode)
  {
    if(try_write_to_file(file, text, mode) != write_outcome::written)
      throw std::runtime_error{report_failed_write(file)};
  }

  void replace_contents(const std::filesystem::path& file, std::string_view text, write_mode mode)
  {
    if(const auto failurePath{replace_contents_quietly(file, text, mode)})
      throw std::runtime_error{report_failed_write(*failurePath)};
  }

  std::optional<std::filesystem::path>
    replace_contents_quietly(const std::filesystem::path& file, std::string_view text, write_mode mode)
  {
    const auto partial{unused_partial_path(file)};
    const auto binary{mode == write_mode::binary ? std::ios_base::binary : std::ios_base::openmode{}};

    // Another process may create partial between the search and this open.
    // With noreplace the open then fails, rather than overwriting that file.
    const auto outcome{try_write_to_file(partial, text, std::ios_base::out | std::ios_base::noreplace | binary)};
    const auto failurePath{(outcome == write_outcome::written) ? try_rename(partial, file) : std::optional{partial}};

    // A successful open created partial, which this function may then remove
    if(failurePath && (outcome != write_outcome::open_failed))
    {
      std::error_code selectsTheNonThrowingOverload{};
      std::filesystem::remove(partial, selectsTheNonThrowingOverload);
    }

    return failurePath;
  }
}
