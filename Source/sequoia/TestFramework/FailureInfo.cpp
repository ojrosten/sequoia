////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/FailureInfo.hpp"
#include "sequoia/TestFramework/Output.hpp"
#include "sequoia/Streaming/Streaming.hpp"

#include <fstream>
#include <iomanip>

namespace sequoia::testing
{
  namespace fs = std::filesystem;
  
  namespace
  {
    [[nodiscard]]
    std::string analyse_output(const std::vector<failure_output>& failuresFromFiles)
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
           begin{first};

      std::string freqs{"["};
      std::string messages{};
      while(++first != last)
      {
        if(*first != *begin)
        {
          freqs += to_percent(std::distance(begin, first)) += "%,";
          auto[i,j]{std::mismatch(begin->begin(), begin->end(), first->begin(), first->end())};
          if((i == begin->end()) && (j == first->end()))
          {
            throw std::logic_error{"Unable to identify instability"};
          }
          else if(i == begin->end())
          {
            messages.append("No failures\n\nvs.\n\n").append(j->message).append("\n\n");
          }
          else if(j == first->end())
          {
            messages.append(i->message).append("\n\nvs.\n\nNo failures");
          }
          else
          {
            messages.append(i->message).append("\n\nvs.\n\n").append(j->message);
          }

          begin = first;
        }
      }

      freqs += to_percent(std::distance(begin, last)) += "%]\n\n"s;

      return ("\nInstability detected. Outcome frequencies:\n" + freqs) += messages += footer();
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
  std::string instability_analysis(const std::filesystem::path& root, const std::size_t trials)
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

        std::sort(outputFiles.begin(), outputFiles.end());

        return outputFiles;
      }()
    };

    if(files.size() % trials)
      throw std::runtime_error{"Instability analysis: incorrect number of output files"};

    for(auto i{files.begin()}; i != files.end(); i+=trials)
    {
      std::vector<failure_output> failuresFromFiles{};
      std::transform(i, std::next(i, trials), std::back_inserter(failuresFromFiles), [](const fs::path& file){
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

      std::sort(failuresFromFiles.begin(), failuresFromFiles.end());
      message += analyse_output(failuresFromFiles);
    }

    return message;
  }
}
