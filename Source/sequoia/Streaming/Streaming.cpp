////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/Streaming/Streaming.hpp"

#include <fstream>

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
  std::string read_to_string(const std::filesystem::path& file)
  {
    if(std::ifstream ifile{file})
    {
      std::stringstream buffer{};
      buffer << ifile.rdbuf();
      return buffer.str();
    }

    throw std::runtime_error{report_failed_read(file)};
  }

  void write_to_file(const std::filesystem::path& file, std::string_view text)
  {
    if(std::ofstream ofile{file})
    {
      ofile.write(text.data(), text.size());
    }
    else
    {
      throw std::runtime_error{report_failed_write(file)};
    }
  }
}
