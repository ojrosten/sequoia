////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for FileSystem.hpp
 */

#include "FileSystem.hpp"

#include "Format.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path project_root(int argc, char** argv, const std::filesystem::path& fallback)
  {    
    namespace fs = std::filesystem;

    static fs::path p{};
    
    if(p.empty())
    {
      if(argc)
      {
        std::string_view zeroth{argv[0]};
        p = fs::canonical(fs::path(zeroth)).parent_path();
        if(!p.empty())
        {
          auto back{[](const fs::path& p){ return *(--p.end()); }};

          while((std::distance(p.begin(), p.end()) > 1))
          {
            const auto last{back(p)};
            p = p.parent_path();
            if(last == "build") break;
          }

          return p;
        }
      }

      p = fallback;
    }

    return p;
  }

  [[nodiscard]]
  std::string report_file_issue(const std::filesystem::path& file, std::string_view description)
  {
    auto mess{std::string{"Unable to open file "}.append(file)};
    if(!description.empty()) mess.append(" ").append(description);
    mess.append("\n");
 
    return mess;
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

  void throw_unless_exists(const std::filesystem::path& p, std::string_view message)
  {
    namespace fs = std::filesystem;

    throw_if(p, append_lines(" does not exist", message),
             [](const fs::path& p){ return !fs::exists(p); });
  }
  
  void throw_unless_directory(const std::filesystem::path& p, std::string_view message)
  {
    namespace fs = std::filesystem;

    throw_unless_exists(p, message);
    throw_if(p, append_lines(" is not a directory", message), [](const fs::path& p){ return !fs::is_directory(p); });
  }

  void throw_unless_regular_file(const std::filesystem::path& p, std::string_view message)
  {
    namespace fs = std::filesystem;

    throw_unless_exists(p, message);
    throw_if(p, append_lines(" is not a regular file", message), [](const fs::path& p){ return !fs::is_regular_file(p); });
  }
}
