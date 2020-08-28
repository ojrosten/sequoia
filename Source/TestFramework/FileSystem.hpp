////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief File paths and related utilities.
 */


#include <filesystem>

namespace sequoia::testing
{
  const static auto working_path_v{std::filesystem::current_path()};

  [[nodiscard]]
  inline std::filesystem::path working_path()
  {
    return working_path_v;
  }

  [[nodiscard]]
  inline std::filesystem::path parent_path()
  {
    return working_path().parent_path();
  }  

  [[nodiscard]]
  inline std::filesystem::path sibling_path(std::string_view directory)
  {
    return parent_path().append(directory);
  }
  
  [[nodiscard]]
  inline std::filesystem::path get_output_path(std::string_view subDirectory)
  {
    return sibling_path("output").append(subDirectory);
  }  

  [[nodiscard]]
  inline std::filesystem::path get_aux_path(std::string_view subDirectory)
  {
    return sibling_path("aux_files").append(subDirectory);
  }

  /*! \brief Used for specifying a directory path, together with a flag to indicate whether a
      search should be recursive
   */

  class search_path
  {
  public:    
    enum class recursive { yes, no };

    search_path(std::filesystem::path directory, recursive r)
      : m_Directory{std::move(directory)}
      , m_Recursive{r}
    {
      if(!std::filesystem::exists(m_Directory))
        throw_error("not found");
      
      if(!std::filesystem::is_directory(m_Directory))
        throw_error("does not correspond to a directory");
    }

    [[nodiscard]]
    std::filesystem::path find(std::string_view filename) const
    {
      namespace fs = std::filesystem;
      
      return m_Recursive == recursive::yes
        ? find<fs::recursive_directory_iterator>(filename)
        : find<fs::directory_iterator>(filename);
    }
  private:
    std::filesystem::path m_Directory{};
    recursive m_Recursive{};

    void throw_error(std::string_view message) const
    {
      throw std::runtime_error{std::string{"search_path: "}.append(m_Directory).append(" ").append(message)};
    }

    template<class DirIter>
    [[nodiscard]]
    std::filesystem::path find(std::string_view filename) const
    {
      namespace fs = std::filesystem;
      
      for(const auto& i : DirIter{m_Directory})
      {
        if(auto p{i.path()}; p.filename().string() == filename)
          return p;
      }

      return {};
    }
  };

}
