////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/FailureInfo.hpp"

#include <fstream>

namespace sequoia::testing
{
  std::ostream& operator<<(std::ostream& s, const failure_info& info)
  {
    s << "$Check: " << info.check_index << '\n';
    s << info.message << "\n$\n";
      
    return s;
  }

  std::istream& operator>>(std::istream& s, failure_info& info)
  {
    while(s && std::isspace(static_cast<unsigned char>(s.peek()))) s.get();

    if(s)
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

  std::ostream& operator<<(std::ostream& s, const instability_analysis_info& info)
  {
    s << info.name << "\n";
    for(const auto& output : info.output)
    {
      s << output << '\n';
    }
    
    return s;
  }

   std::istream& operator>>(std::istream& s, instability_analysis_info& info)
   {
     s >> info.name;

     while(s)
     {
       failure_info finfo{};
       s >> finfo;
       info.output.push_back(finfo);
     }
     
     return s;
   }
}
