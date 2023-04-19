////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for FailureInfo.hpp
*/

#include "sequoia/TestFramework/FailureInfo.hpp"
#include "sequoia/TestFramework/FileSystemUtilities.hpp"
#include "sequoia/TestFramework/Output.hpp"
#include "sequoia/TextProcessing/Substitutions.hpp"
#include "sequoia/Streaming/Streaming.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>

namespace sequoia::testing
{
  namespace fs = std::filesystem;
  
  namespace
  {
    [[nodiscard]]
    std::string analyse_output(const fs::path& filename, const std::vector<failure_output>& failuresFromFiles)
    {
      if(failuresFromFiles.size() <= 1) return "";

      using namespace std::string_literals;

      auto to_percent{
        [&failuresFromFiles](auto num){
          std::stringstream s{};
          s << std::setprecision(3) << 100 * static_cast<double>(num) / failuresFromFiles.size();
          return s.str();
        }
      };

      auto first{failuresFromFiles.begin()},
           last{failuresFromFiles.end()},
           current{first};

      const auto initial{first};

      std::string freqs{"["};
      std::string messages{};
      while(++first != last)
      {
        if(*first != *current)
        {
          freqs += to_percent(std::ranges::distance(current, first)) += "%,";
          auto[i,j]{std::ranges::mismatch(*current, *first)};
          if(j == first->end())
          {
            throw std::logic_error{"Unable to identify instability"};
          }
          else if(i == current->end())
          {
            if(current->begin() == current->end())
            {
              messages.append("--No failures--\n\nvs.\n\n").append(j->message);
            }
            else
            {
              const auto commonMessage{
                [current](){
                  std::string mess{};
                  for(auto c{current->begin()}; c != current->end(); ++c)
                  {
                    mess.append(c->message).append("\n");
                  }

                  return mess;
                }()
              };

              messages.append(messages.empty() ? commonMessage : "\n");

              messages.append("vs.\n\n")
                      .append(commonMessage)
                      .append(j->message);
            }
          }
          else
          {
            if(current == initial)
              messages.append(i->message);

            messages.append("\nvs.\n\n").append(j->message);
          }

          current = first;
        }
      }

      if(current != initial)
      {
        freqs += to_percent(std::ranges::distance(current, last)) += "%]\n\n"s;

        return std::string{"\nInstability detected in file \""}
          .append(filename.string())
          .append("\"\nOutcome frequencies:\n" + freqs)
          .append(messages)
          .append("\n")
          .append(instability_footer());
      }

      return "";
    }

    [[nodiscard]]
    fs::path source_from_instability_analysis(const fs::path& dir)
    {
      auto parent{dir.parent_path()};
      if(!parent.empty())
      {
        std::string str{(--parent.end())->string()};
        if(auto pos{str.find_last_of('_')}; pos != std::string::npos)
        {
          str[pos] = '.';
        }

        return str;
      }

      return "";
    }
  }
  
  std::ostream& operator<<(std::ostream& s, const failure_info& info)
  {
    s << "$Check: " << info.check_index << '\n';
    s << info.message << "\n$\n";
      
    return s;
  }

  std::istream& operator>>(std::istream& s, failure_info& info)
  {
    while(s && std::isspace(static_cast<unsigned char>(s.peek()))) s.get();

    if(s && (s.peek() != std::istream::traits_type::eof()))
    {
      failure_info newInfo{};
      if(std::string str{}; (s >> str) && (str ==  "$Check:"))
      {        
        s >> newInfo.check_index;
        if(s.fail())
          throw std::runtime_error{"Error while parsing failure_info: unable to determine index"};
          
        s.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      }
      else
      {
        throw std::runtime_error{"Error while parsing failure_info: unable to find $Check:"};
      }

      auto messageBuilder{
        [&s,&newInfo]() -> bool {
          std::string line{};
          while(std::getline(s, line))
          {
            if(line == "$")
            {
              if(auto& mess{newInfo.message}; !mess.empty() && (mess.back() == '\n'))
              {
                mess.pop_back();
              }

              return true;
            }

            newInfo.message.append(line).append("\n");
          }

          return false;
        }
      };

      if(!messageBuilder())
      {
        throw std::runtime_error{"Error while parsing failure_info: unable to find message"};
      }

      info = std::move(newInfo);
    }
    
    return s;
  }

  std::ostream& operator<<(std::ostream& s, const failure_output& output)
  {
    for(const auto& info : output)
    {
      s << info << '\n';
    }
    
    return s;
  }

  std::istream& operator>>(std::istream& s, failure_output& output)
  {
    while(s)
    {
      failure_info info{};
      s >> info;
      if(!s.fail())
        output.push_back(info);
    }

    return s;
  }

  [[nodiscard]]
  std::string instability_analysis(const fs::path& root, const std::size_t trials)
  {
    if(trials <= 1) return "";

    std::string message{};

    const auto files{
      [&root](){
        std::vector<fs::path> outputFiles{};
        for(const auto& entry : fs::recursive_directory_iterator(root))
        {
          if(is_regular_file(entry))
          {
            const auto& path{entry.path()};
            if(path.extension() == ".txt")
            {
              outputFiles.push_back(path);
            }
          }
        }

        std::ranges::sort(outputFiles);

        return outputFiles;
      }()
    };

    if(files.size() % trials)
      throw std::runtime_error{"Instability analysis: incorrect number of output files"};

    for(auto i{files.begin()}; i != files.end(); i+=trials)
    {
      std::vector<failure_output> failuresFromFiles{};
      std::ranges::transform(i, std::next(i, trials), std::back_inserter(failuresFromFiles), [](const fs::path& file){
        failure_output output{};
        if(std::ifstream ifile{file})
        {
          ifile >> output;
        }
        else
        {
          throw std::runtime_error{report_failed_read(file)};
        }
        return output;
      });

      std::ranges::sort(failuresFromFiles);
      message += analyse_output(source_from_instability_analysis(i->parent_path()), failuresFromFiles);
    }

    return !message.empty() ? message : "\nNo instabilities detected\n";
  }
}
