////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/TestRunnerUtilities.hpp"

#include <chrono>

// TO DO: remove once libc++ supports <format>
//#ifdef _MSC_VER
//  #include <format>
//#endif

namespace sequoia::testing
{
  void report(std::ostream& stream, std::string_view prefix, std::string_view message)
  {
    if(!message.empty())
    {
      stream << prefix << '\n';
      stream << message << "\n\n";
    }
  }

  void set_top_copyright(std::string& text, std::string_view copyright)
  {
    constexpr auto npos{std::string::npos};
    if(auto copyrightPos{text.find("Copyright")}; copyrightPos != npos)
    {
      auto left{text.rfind("//", copyrightPos)};
      auto right{text.find("//", copyrightPos)};
      if((left == npos) || (right == npos))
      {
        throw std::runtime_error("Unable to find boundaries of copyright message");
      }

      // TO DO: prefer the MSC code once supported by other compilers
      const auto year{
        []() -> std::string {
//#ifdef _MSC_VER
//          using namespace std::chrono;
//          return std::format("{}", year_month_day{floor<days>(system_clock::now())}.year());
//#else
          const auto now{std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())};
          return std::to_string(1900 + std::localtime(&now)->tm_year);
//#endif
        }()
      };

      const auto newCopyright{std::string{"Copyright "}.append(copyright).append(" ").append(year).append(".")};

      const auto reservedSpace{right - left - 2};
      const auto requiredSpace{newCopyright.size()};
      const auto remainingSpace{reservedSpace > requiredSpace ? reservedSpace - requiredSpace : std::string::size_type{}};
      const auto rightSpace(remainingSpace / 2);
      const auto leftSpace(remainingSpace - rightSpace);

      const auto replacement{std::string(leftSpace, ' ').append(newCopyright).append(std::string(rightSpace, ' '))};

      text.replace(left + 2, reservedSpace, replacement);
    }
    else
    {
      throw std::runtime_error{"Unable to locate Copyright information"};
    }
  }
}