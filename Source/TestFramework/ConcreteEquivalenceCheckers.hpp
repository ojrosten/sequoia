////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Useful specializations for the class template equivalence_checker.
*/

#include "FreeCheckers.hpp"
#include "Concepts.hpp"

#include <tuple>
#include <filesystem>

namespace sequoia::testing
{
  template<class Char, class Traits>
  struct detailed_equality_checker<std::basic_string_view<Char, Traits>>
  {
    using string_view_type = std::basic_string_view<Char, Traits>;
  private:
    using size_type = typename string_view_type::size_type;

    template<class Advisor>
    static auto make_advisor(std::string_view info, string_view_type obtained, string_view_type prediction, size_type pos, const tutor<Advisor>& advisor)
    {
      constexpr size_type count{60}, npos{string_view_type::npos};
      const auto sz{std::min(obtained.size(), prediction.size())};

      auto newlineLoc{ [pos](string_view_type sv){ return sv.rfind('\n', pos); } };

      auto loc{pos < prediction.size() ? newlineLoc(prediction) : npos};
      if((loc == npos) && (pos < obtained.size())) loc = newlineLoc(obtained);

      constexpr size_type defaultOffset{30};
      const size_type offset{loc < npos ? std::min(defaultOffset, pos - loc) : defaultOffset};

      const auto lpos{pos < offset ? 0 :
                         pos < sz  ? pos - offset : sz - std::min(sz, offset)};

      auto make{
        [lpos](string_view_type sv){
          std::string mess{lpos > 0 ? "..." : ""};

          const auto pos{lpos < sv.size() && sv[lpos] == '\n' ? lpos + 1 : lpos};
          if(pos == lpos + 1) mess.append("\\n");

          mess.append(sv.substr(pos, count));

          if(lpos + count < sv.size())
            mess.append("...");

          return mess;
        }
      };

      const bool truncation{(lpos > 0) || (lpos + count < obtained.size()) || (lpos + count < prediction.size())};
      const auto message{append_lines(info, truncation ? "Surrounding substring(s):" : "Full strings:",
                                      prediction_message(make(obtained), make(prediction)))};
        
      if constexpr(invocable<tutor<Advisor>, Char, Char>)
      {

        return tutor{
          [message, advisor] (Char a, Char b) {

            auto m{message};
            return append_advice(m, {advisor, a, b});            
          },
          "\n"
        };
      }
      else
      {
        return tutor{
          [message](Char, Char) { return message; },
          "\n"
        };
      }
    }
  public:
    template<test_mode Mode, class Advisor>
    static void check(test_logger<Mode>& logger, string_view_type obtained, string_view_type prediction, const tutor<Advisor>& advisor)
    {
      auto iters{std::mismatch(obtained.begin(), obtained.end(), prediction.begin(), prediction.end())};

      if((iters.first != obtained.end()) && (iters.second != prediction.end()))
      {
        const auto dist{std::distance(obtained.begin(), iters.first)};          
        auto adv{make_advisor("", obtained, prediction, dist, advisor)};

        const auto mess{
          std::string{"First difference detected at character "}
            .append(std::to_string(dist)).append(":")
        };
          
        check_equality(mess, logger, *(iters.first), *(iters.second), adv);
      }
      else if((iters.first != obtained.end()) || (iters.second != prediction.end()))
      {
        auto check{
          [&logger, obtained, prediction, &advisor](auto begin, auto iter, std::string_view state, std::string_view adjective){
            const auto dist{std::distance(begin, iter)};
            const auto info{std::string{"First "}.append(state).append(" character: ").append(display_character(*iter))};
            auto adv{make_advisor(info, obtained, prediction, dist, advisor)};

            const auto mess{append_lines("Lengths differ", std::string{"Obtained string is too "}.append(adjective))};

            check_equality(mess, logger, obtained.size(), prediction.size(), adv);
          }
        };
        
        if(iters.second != prediction.end())
        {
          check(prediction.begin(), iters.second, "missing", "short");
        }
        else if(iters.first != obtained.end())
        {
          check(obtained.begin(), iters.first, "excess", "long");
        }
      }
    }
  };

  
  /*! \brief Checks equivalence of std::basic_string to char[] and string_view */
  template<class Char, class Traits, alloc Allocator>
  struct equivalence_checker<std::basic_string<Char, Traits, Allocator>>
  {
    using string_type = std::basic_string<Char, Traits, Allocator>;
    
    template<test_mode Mode, std::size_t N, class Advisor>
    static void check(test_logger<Mode>& logger, const string_type& obtained, char const (&prediction)[N], tutor<Advisor> advisor)
    {
       using checker = detailed_equality_checker<std::basic_string_view<Char, Traits>>;
      
       checker::check(logger, std::string_view{obtained}, std::string_view{prediction}, std::move(advisor));
    }

    template<test_mode Mode, class Advisor>
    static void check(test_logger<Mode>& logger, const string_type& obtained, std::basic_string_view<Char, Traits> prediction, tutor<Advisor> advisor)
    {
       using checker = detailed_equality_checker<std::basic_string_view<Char, Traits>>;
      
      checker::check(logger, std::string_view{obtained}, std::string_view{prediction}, std::move(advisor));
    }
  };

  /*! \brief Checks equivalence of std::pair<S,T> and std::pair<U,V> where,
      after removing references and cv-qualifiers, S,U and T,V are each the same
   */
  template<class S, class T>
  struct equivalence_checker<std::pair<S, T>>
  {
    template<test_mode Mode, class U, class V, class Advisor>
    static void check(test_logger<Mode>& logger, const std::pair<S, T>& value, const std::pair<U, V>& prediction, tutor<Advisor> advisor)
    {        
      static_assert(   std::is_same_v<std::remove_cvref_t<S>, std::remove_cvref_t<U>>
                    && std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<V>>);

      check_equality("First element of pair is incorrect", logger, value.first, prediction.first, advisor);
      check_equality("Second element of pair is incorrect", logger, value.second, prediction.second, advisor);
    }
  };

  /*! \brief Checks equivalence of std::tuple<T...> and std::tuple<U...> where T... and U...
      are the same size and, after removing references and cv-qualifiers, the respective elements
      are of the same type
   */
  template<class... T>
  struct equivalence_checker<std::tuple<T...>>
  {
  private:
    template<test_mode Mode, std::size_t I = 0, class... U, class Advisor>
    static void check_tuple_elements(test_logger<Mode>& logger, const std::tuple<T...>& value, const std::tuple<U...>& prediction, tutor<Advisor> advisor)
    {
      if constexpr(I < sizeof...(T))
      {
        const std::string message{"Element " + std::to_string(I) + " of tuple incorrect"};
        check_equality(message, logger, std::get<I>(value), std::get<I>(prediction), advisor);
        check_tuple_elements<Mode, I+1>(logger, value, prediction, advisor);
      }
    }
      
  public:
    template<test_mode Mode, class... U, class Advisor>
    static void check(test_logger<Mode>& logger, const std::tuple<T...>& value, const std::tuple<U...>& prediction, tutor<Advisor> advisor)
    {
      static_assert(sizeof...(T) == sizeof...(U));
      static_assert((std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>> && ...));      

      check_tuple_elements(logger, value, prediction, advisor);
    }
  };

  template<>
  struct serializer<std::filesystem::file_type>
  {
    [[nodiscard]]
    static std::string make(const std::filesystem::file_type& val)
    {
      using ft = std::filesystem::file_type;
      switch(val)
      {
      case ft::none:
        return "none";
      case ft::not_found:
        return "not found";
      case ft::regular:
        return "regular";
      case ft::directory:
        return "directory";
      case ft::symlink:
        return "symlink";
      case ft::block:
        return "block";
      case ft::character:
        return "character";
      case ft::fifo:
        return "fifo";
      case ft::socket:
        return "socket";
      case ft::unknown:
        return "unknown";
      default:
        return "unrecognized";
      }
    }
  };

  /*! \brief Checks equivalence of filesystem paths.

      Files are considered equivalent if they have the same name and the same contents;
      similarly directories.

   */

  template<>
  struct equivalence_checker<std::filesystem::path>
  {
    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const std::filesystem::path& path, const std::filesystem::path& prediction)
    {      
      namespace fs = std::filesystem;

      const auto pathType{fs::status(path).type()};
      const auto predictionType{fs::status(prediction).type()};

      if(check_equality(preamble("Path type", path, prediction), logger, pathType, predictionType))
      {
        if(!path.empty())
        {
          const auto pathFinalToken{*(--path.end())};
          const auto predictionFinalToken{*(--prediction.end())};
          if(check_equality("Final path token", logger, pathFinalToken, predictionFinalToken))
          {
            switch(pathType)
            {
            case fs::file_type::regular:
              check_file(logger, path, prediction);
              break;
            case fs::file_type::directory:
              check_directory(logger, path, prediction);
              break;
            default:
              throw std::logic_error{std::string{"Detailed equivalance check for paths of type "}
                .append(serializer<fs::file_type>::make(pathType)).append(" not currently implemented")};
            }
          }
        }
      }          
    }

  private:
    constexpr static std::array<std::string_view, 2>
      st_ExcludedFiles{
        ".DS_Store",
        ".keep"
    };
    
    template<test_mode Mode>
    static void check_directory(test_logger<Mode>& logger, const std::filesystem::path& dir, const std::filesystem::path& prediction)
    {
      namespace fs = std::filesystem;

      auto generator{
        [](const fs::path& dir){
          std::vector<fs::path> paths{};
          for(const auto& p : fs::directory_iterator(dir))
          {
            if(std::find(st_ExcludedFiles.begin(), st_ExcludedFiles.end(), p.path().filename()) == st_ExcludedFiles.end())
               paths.push_back(p);
          }

          std::sort(paths.begin(), paths.end());

          return paths;
        }
      };

      const std::vector<fs::path> paths{generator(dir)}, predictedPaths{generator(prediction)};

      check_equality(std::string{"Number of directory entries for "}.append(dir), logger, paths.size(), predictedPaths.size());

      const auto iters{std::mismatch(paths.begin(), paths.end(), predictedPaths.begin(), predictedPaths.end(),
          [&dir,&prediction](const fs::path& lhs, const fs::path& rhs){
            return fs::relative(lhs, dir) == fs::relative(rhs, prediction);
          })};
      if((iters.first != paths.end()) && (iters.second != predictedPaths.end()))
      {
        check_equality("First directory entry mismatch", logger, *iters.first, *iters.second);
      }
      else if(iters.first != paths.end())
      {
        check_equality("First directory entry mismatch", logger, *iters.first, fs::path{});
      }
      else if(iters.second != predictedPaths.end())
      {
        check_equality("First directory entry mismatch", logger, fs::path{}, *iters.second);
      }
      else
      {
        for(std::size_t i{}; i < paths.size(); ++i)
        {
          check(logger, paths[i], predictedPaths[i]);
        }
      } 
    }

    template<test_mode Mode>
    static void check_file(test_logger<Mode>& logger,const std::filesystem::path& file, const std::filesystem::path& prediction)
    {
      std::ifstream file1{file}, file2{prediction};

      if(   testing::check(report_failed_read(file), logger, static_cast<bool>(file1))
         && testing::check(report_failed_read(prediction), logger, static_cast<bool>(file2)))
      {
        std::stringstream buffer1{}, buffer2{};
        buffer1 << file1.rdbuf();
        buffer2 << file2.rdbuf();

        check_equality(preamble("Contents of", file, prediction), logger, buffer1.str(), buffer2.str());
      }
    }

    [[nodiscard]]
    static std::string report_file_issue(const std::filesystem::path& file, std::string_view description)
    {
      auto mess{std::string{"Unable to open file "}.append(file)};
      if(!description.empty()) mess.append(" ").append(description);
      mess.append("\n");

      return mess;
    }

    [[nodiscard]]
    static std::string report_failed_read(const std::filesystem::path& file)
    {
      return report_file_issue(file, " for reading");
    }

    [[nodiscard]]
    static std::string preamble(std::string_view prefix, const std::filesystem::path& path, const std::filesystem::path& prediction)
    {
      return append_lines(prefix, path.string(), "vs", prediction.string()).append("\n");
    }
  };
}
