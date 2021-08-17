////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/Core/Meta/Concepts.hpp"

#include <filesystem>
#include <optional>

namespace sequoia
{
  [[nodiscard]]
  std::string report_failed_read(const std::filesystem::path& file);

  [[nodiscard]]
  std::string report_failed_write(const std::filesystem::path& file);


  [[nodiscard]]
  std::optional<std::string> read_to_string(const std::filesystem::path& file);

  void write_to_file(const std::filesystem::path& file, std::string_view text);

  template<invocable<std::string&> Fn>
  void read_modify_write(const std::filesystem::path& file, Fn fn)
  {
    if(auto text{read_to_string(file)})
    {
      fn(*text);
      write_to_file(file, *text);
    }
    else
    {
      throw std::runtime_error{report_failed_read(file)};
    }
  }
}
