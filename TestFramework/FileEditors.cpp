////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for FileEditors.hpp
 */

#include "FileEditors.hpp"

namespace sequoia::testing
{
  void add_include(const std::filesystem::path& filePath, std::string_view include)
  {
    if(std::ofstream ofile{filePath, std::ios_base::app})      
    {
      ofile << "#include \"" << include << "\"\n";
    }    
    else
    {
      throw std::runtime_error(std::string{"Unable to open "}.append(filePath.string()).append(" for writing"));
    }
  }
}
