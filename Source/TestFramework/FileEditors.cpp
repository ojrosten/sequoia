////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for FileEditors.hpp
 */

#include "FileEditors.hpp"
#include "FileSystem.hpp"
#include "Output.hpp"

#include <fstream>

namespace sequoia::testing
{
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

  void add_include(const std::filesystem::path& file, std::string_view include)
  {
    std::string text{read_to_string(file)};

    const std::string::size_type pos{
      [&text](){
        const auto lastIncludePos{text.rfind("#include")};
        if(lastIncludePos != std::string::npos)
        {
          const auto pos{text.find('\n', lastIncludePos)};
          if(pos < text.size()) return pos + 1;
        }

        return text.size();
      }()
    };

    text.insert(pos, std::string{"#include \""}.append(include).append("\"\n"));
    
    if(std::ofstream ofile{file})      
    {
      ofile << text;
    }    
    else
    {
      throw std::runtime_error{report_failed_write(file)};
    }
  }

  void add_to_family(const std::filesystem::path& file, std::string_view familyName, const std::vector<std::string>& tests)
  {
    namespace fs = std::filesystem;

    if(!fs::exists(file))
      throw fs::filesystem_error{"Unable to find file", file, std::error_code{}};

    if(tests.empty())
      throw std::logic_error{std::string{"No tests specified to be added to the test family \""}.append(familyName).append("\"")};

    std::string text{read_to_string(file)};
    const auto pattern{std::string{"\""}.append(familyName).append("\",")};

    bool correctStructure{};
    constexpr auto npos{std::string::npos};
    if(auto pos{text.find(pattern)}; pos != npos)
    {
      if(const auto linePos{text.rfind('\n', pos)}; linePos != npos)
      {
        std::string_view preamble{"add_test_family("};
        if((linePos > preamble.size()) && (text.find(preamble, linePos - preamble.size()) != npos))
        {
          if(const auto nextLinePos{text.find('\n', pos)}; nextLinePos != npos)
          {          
            const indentation indent{text.substr(linePos + 1, pos - linePos - 1)};
            const auto endpos{text.find(");", pos)};
            std::string_view subtext{text.substr(pos, endpos - pos)};
            correctStructure = true;
            for(const auto& t : tests)
            {
              if(subtext.find(t) == npos)
              {
                text.insert(nextLinePos+1, std::string{indent}.append(t).append(",\n"));
              }
            }
          }
        }
      }
    }
    else if(pos = text.find("runner.execute"); pos != npos)
    {
      if(const auto linePos{text.rfind('\n', pos)}; linePos != npos)
      {
        correctStructure = true;

        auto builder{
          [&text, &tests, pos, linePos, familyName](){

            const indentation indent_0{text.substr(linePos + 1, pos - linePos - 1)};
            const indentation indent_1{std::string{indent_0}.append("  ")};

            auto str{std::string{"\n"}.append(indent_0).append("runner.add_test_family(")};

            append_indented(str, std::string{"\""}.append(familyName).append("\","), indent_1);

            for(auto i{tests.cbegin()}; i != tests.cend() - 1; ++i)
            {
              append_indented(str, std::string{*i}.append(","), indent_1);
            }

            append_indented(str, tests.back(), indent_1);
            append_indented(str, ");\n", indent_0);

            return str;
          }
        };
        
        text.insert(linePos, builder());
      }
    }

    if(!correctStructure)
    {
      throw std::runtime_error{"Unable to find appropriate place to add test family"};
    }

    const auto tempPath{fs::path{file}.concat("x")};
    if(std::ofstream ofile{tempPath})
    {
      ofile.write(text.data(), text.size());
    }
    else
    {
      throw std::runtime_error{report_failed_write(tempPath)};
    }

    fs::copy_file(tempPath, file, fs::copy_options::overwrite_existing);
    fs::remove(tempPath);
  }
}
