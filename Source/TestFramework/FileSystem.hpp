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

#include "Concepts.hpp"

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
  inline std::filesystem::path output_path(std::string_view subDirectory)
  {
    return sibling_path("output").append(subDirectory);
  }

  [[nodiscard]]
  inline std::filesystem::path aux_path(std::string_view subDirectory)
  {
    return sibling_path("aux_files").append(subDirectory);
  }

  [[nodiscard]]
  inline std::filesystem::path test_materials_path()
  {
    return sibling_path("TestMaterials");
  }

  [[nodiscard]]
  inline std::filesystem::path temporary_data_path()
  {
    return output_path("TestsTemporaryData");
  }

  template<class Pred>
    requires invocable<Pred, std::filesystem::path>
  void throw_if(const std::filesystem::path& p, std::string_view message, Pred pred)
  {
    if(pred(p))
    {      
      throw std::runtime_error{std::string{p}.append(" ").append(message)};
    }
  }

  [[nodiscard]]
  std::string report_file_issue(const std::filesystem::path& file, std::string_view description);
 
  [[nodiscard]]
  std::string report_failed_read(const std::filesystem::path& file);
 
  [[nodiscard]]
  std::string report_failed_write(const std::filesystem::path& file);

  void throw_unless_exists(const std::filesystem::path& p, std::string_view message="");  

  void throw_unless_directory(const std::filesystem::path& p, std::string_view message="");
  
  void throw_unless_regular_file(const std::filesystem::path& p, std::string_view message="");


  /*! \brief Used for performing a recursive search
   */

  class search_tree
  {
  public:
    search_tree(std::filesystem::path root)
      : m_Root{std::move(root)}
    {
      throw_unless_directory(m_Root, "");
    }

    [[nodiscard]]
    const std::filesystem::path& root() const noexcept
    {
      return m_Root;
    }

    [[nodiscard]]
    std::filesystem::path find(std::string_view filename) const
    {
      using dir_iter = std::filesystem::recursive_directory_iterator;

      for(const auto& i : dir_iter{m_Root})
      {
        if(auto p{i.path()}; p.filename().string() == filename)
          return p;
      }

      return {};
    }    
  private:
    std::filesystem::path m_Root{};
  };
}
