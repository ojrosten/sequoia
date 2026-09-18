////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/Streaming/Streaming.hpp"

#include <fstream>
#include <system_error>

namespace sequoia
{
  namespace
  {
    [[nodiscard]]
    std::string report_file_issue(const std::filesystem::path& file, std::string_view description)
    {
      auto mess{std::string{"Unable to open file "}.append(file.generic_string())};
      if(!description.empty()) mess.append(" ").append(description);
      mess.append("\n");

      return mess;
    }
  }

  [[nodiscard]]
  std::string report_failed_read(const std::filesystem::path& file)
  {
    return report_file_issue(file, " for reading");
  }

  [[nodiscard]]
  std::string report_failed_write(const std::filesystem::path& file)
  {
    return report_file_issue(file, " for writing");
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

  void write_to_file(const std::filesystem::path& file, std::string_view text, std::ios_base::openmode mode)
  {
    if(std::ofstream ofile{file, mode})
    {
      ofile.write(text.data(), text.size());
    }
    else
    {
      throw std::runtime_error{report_failed_write(file)};
    }
  }
}
