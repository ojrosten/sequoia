////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Useful specializations for the class templates detailed_equality_checker and equivalence_checker.

    The specializations in this header are for various types defined in std. Internally,
    check_equality/check_equivalnce is used meaning that there will be automatic, recursive dispatch to
    other specializations of detailed_equality_checker, if appropriate. For example,
    consider two instances of std::pair<my_type1, mytype2>, x and y. The utilities in this
    header means the call

    check_equality("descripion", logger, x, y);

    will automatically call

    check_equality("automatically enhanced desciption", logger, x.first, y,first)

    and similarly for the second element. In turn, this nested check_equality will use
    a specialization of the detailed_equality_checker of my_type1, should it exist. As
    usual, if the specialization for T does not exist, but T may be interpreted as
    a container holding a type U, then everything will simply work, provided either that
    there exists a specialization of the detailed_equality_checker for U or U is serializable.
 */

#include "sequoia/TestFramework/FreeCheckers.hpp"
#include "sequoia/TestFramework/FileEditors.hpp"
#include "sequoia/TestFramework/FileSystem.hpp"
#include "sequoia/Runtime/Factory.hpp"
#include "sequoia/Streaming/Streaming.hpp"

#include <array>
#include <tuple>
#include <optional>
#include <variant>

namespace sequoia::testing
{
  /*! \brief Checks equality of std::basic_string */
  template<class Char, class Traits>
  struct detailed_equality_checker<std::basic_string_view<Char, Traits>>
  {
    using string_view_type = std::basic_string_view<Char, Traits>;
  private:
    using size_type = typename string_view_type::size_type;

    template<class Advisor>
    static auto make_advisor(std::string_view info, string_view_type obtained, string_view_type prediction, size_type pos, const tutor<Advisor>& advisor)
    {
      constexpr size_type defaultOffset{30}, defaultCount{60}, npos{string_view_type::npos};
      const auto sz{std::min(obtained.size(), prediction.size())};

      auto newlineBackwards{ [pos](string_view_type sv){ return pos < sv.size() ? sv.rfind('\n', pos) : npos; } };

      const auto loc{std::min(newlineBackwards(prediction), newlineBackwards(obtained))};

      const size_type offset{loc < npos ? std::min(defaultOffset, pos - loc) : defaultOffset};

      const auto lpos{pos < offset ? 0 :
                         pos < sz  ? pos - offset : sz - std::min(sz, offset)};

      struct message{ std::string mess; bool trunc{}; };

      auto make{
        [lpos](string_view_type sv) -> message {
          std::string mess{lpos > 0 ? "..." : ""};

          const auto pos{lpos < sv.size() && sv[lpos] == '\n' ? lpos + 1 : lpos};
          if(pos == lpos + 1) mess.append("\\n");

          const auto rpos{sv.find('\n', pos+1)};
          const auto count{rpos == npos ? defaultCount : rpos - pos};

          mess.append(sv.substr(pos, count));

          const bool trunc{lpos + count < sv.size()};
          if(trunc) mess.append("...");

          return {mess, trunc};
        }
      };

      const auto[obMess,obTrunc]{make(obtained)};
      const auto[prMess,prTrunc]{make(prediction)};

      const bool trunc{lpos > 0 || obTrunc || prTrunc};
      const auto message{append_lines(info,  trunc ? "Surrounding substring(s):" : "Full strings:",
                                      prediction_message(obMess, prMess))};

      if constexpr(std::invocable<tutor<Advisor>, Char, Char>)
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
          [message](const auto&, const auto&) { return message; },
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
      requires (I < sizeof...(T))
    static void check_tuple_elements(test_logger<Mode>& logger, const std::tuple<T...>& value, const std::tuple<U...>& prediction, const tutor<Advisor>& advisor)
    {
      const std::string message{"Element " + std::to_string(I) + " of tuple incorrect"};
      check_equality(message, logger, std::get<I>(value), std::get<I>(prediction), advisor);
      check_tuple_elements<Mode, I+1>(logger, value, prediction, advisor);
    }

    template<test_mode Mode, std::size_t I = 0, class... U, class Advisor>
    static void check_tuple_elements(test_logger<Mode>&, const std::tuple<T...>&, const std::tuple<U...>&, const tutor<Advisor>&)
    {}

  public:
    template<test_mode Mode, class... U, class Advisor>
      requires((sizeof...(T) == sizeof...(U)) && (std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>> && ...))
    static void check(test_logger<Mode>& logger, const std::tuple<T...>& value, const std::tuple<U...>& prediction, const tutor<Advisor>& advisor)
    {
      check_tuple_elements(logger, value, prediction, advisor);
    }
  };

  [[nodiscard]]
  std::string path_check_preamble(std::string_view prefix, const std::filesystem::path& path, const std::filesystem::path& prediction);

  /*! \brief Function object for default checking of files */

  struct default_file_checker
  {
    template<test_mode Mode>
    void operator()(test_logger<Mode>& logger, const std::filesystem::path& file, const std::filesystem::path& prediction) const
    {
      const auto [reducedWorking, reducedPrediction] {get_reduced_file_content(file, prediction)};

      testing::check(report_failed_read(file), logger, static_cast<bool>(reducedWorking));
      testing::check(report_failed_read(prediction), logger, static_cast<bool>(reducedPrediction));

      if(reducedWorking && reducedPrediction)
      {
        check_equality(path_check_preamble("Contents of", file, prediction), logger, reducedWorking.value(), reducedPrediction.value());
      }
    }
  };

  struct basic_file_checker
  {
    template<test_mode Mode>
    static void check_file(test_logger<Mode>& logger, const std::filesystem::path& file, const std::filesystem::path& prediction)
    {
      const auto factory{runtime::factory<default_file_checker>{{"default"}}};
      const auto checker{factory.create_or<default_file_checker>(file.extension().string())};
      std::visit([&logger, &file, &prediction](auto&& fn){ fn(logger, file, prediction); }, checker);
    }
  };

  /*! \brief Helper class fo checking (weak) equivalence of file paths */

  struct path_checker
  {
    template<test_mode Mode, class Customization, invocable_r<bool, std::filesystem::path, std::filesystem::path> FinalTokenComparison>
    static void check(test_logger<Mode>& logger, const Customization& custom, const std::filesystem::path& path, const std::filesystem::path& prediction, FinalTokenComparison compare)
    {
      namespace fs = std::filesystem;

      const auto pathType{fs::status(path).type()};
      const auto predictionType{fs::status(prediction).type()};

      if(check_equality(path_check_preamble("Path type", path, prediction), logger, pathType, predictionType))
      {
        if(!path.empty())
        {
          const auto pathFinalToken{*(--path.end())};
          const auto predictionFinalToken{*(--prediction.end())};
          if(compare(pathFinalToken, predictionFinalToken))
          {
            switch(pathType)
            {
            case fs::file_type::regular:
              check_file(logger, custom, path, prediction);
              break;
            case fs::file_type::directory:
              check_directory(logger, custom, path, prediction, compare);
              break;
            default:
              throw std::logic_error{std::string{"Detailed equivalance check for paths of type '"}
                .append(serializer<fs::file_type>::make(pathType)).append("' not currently implemented")};
            }
          }
        }
      }
    }

  private:
    constexpr static std::array<std::string_view, 2>
      excluded_files{".DS_Store", ".keep"};

    constexpr static std::array<std::string_view, 1>
      excluded_extensions{seqpat};

    template<test_mode Mode, class Customization, invocable_r<bool, std::filesystem::path, std::filesystem::path> FinalTokenComparison>
    static void check_directory(test_logger<Mode>& logger, const Customization& custom, const std::filesystem::path& dir, const std::filesystem::path& prediction, FinalTokenComparison compare)
    {
      namespace fs = std::filesystem;

      auto generator{
        [](const fs::path& dir) {
          std::vector<fs::path> paths{};
          for(const auto& p : fs::directory_iterator(dir))
          {
            if(    std::find(excluded_files.begin(),      excluded_files.end(),      p.path().filename())  == excluded_files.end()
                && std::find(excluded_extensions.begin(), excluded_extensions.end(), p.path().extension()) == excluded_extensions.end())
            {
               paths.push_back(p);
            }
          }

          std::sort(paths.begin(), paths.end());

          return paths;
        }
      };

      const std::vector<fs::path> paths{generator(dir)}, predictedPaths{generator(prediction)};

      check_equality(std::string{"Number of directory entries for "}.append(dir.generic_string()),
        logger,
        paths.size(),
        predictedPaths.size());

      const auto iters{std::mismatch(paths.begin(), paths.end(), predictedPaths.begin(), predictedPaths.end(),
          [&dir,&prediction](const fs::path& lhs, const fs::path& rhs) {
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
          check(logger, custom, paths[i], predictedPaths[i], compare);
        }
      }
    }

    template<test_mode Mode, class Customization>
    static void check_file(test_logger<Mode>& logger, const Customization& custom, const std::filesystem::path& file, const std::filesystem::path& prediction)
    {
      custom.check_file(logger, file, prediction);
    }
  };

  /*! \brief Checks equivalence of filesystem paths.

    Files are considered equivalent if they have the same name and the same contents;
    similarly directories.

   */

  template<>
  struct equivalence_checker<std::filesystem::path>
  {
    template<test_mode Mode, class Customization>
    static void check(test_logger<Mode>& logger, const Customization& custom, const std::filesystem::path& path, const std::filesystem::path& prediction)
    {
      namespace fs = std::filesystem;

      auto pred{
        [&logger](const fs::path& pathFinalToken, const fs::path& predictionFinalToken)
        {
           return check_equality("Final path token", logger, pathFinalToken, predictionFinalToken);
        }
      };

      path_checker::check(logger, custom, path, prediction, pred);
    }

    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const std::filesystem::path& path, const std::filesystem::path& prediction)
    {
      check(logger, basic_file_checker{}, path, prediction);
    }

  };

  /*! \brief Checks equivalence of filesystem paths.

    Files are considered equivalent if they have the same contents;
    similarly directories. The names of both are ignored.

   */

  template<>
  struct weak_equivalence_checker<std::filesystem::path>
  {
    template<test_mode Mode, class Customization>
    static void check(test_logger<Mode>& logger, const Customization& custom, const std::filesystem::path& path, const std::filesystem::path& prediction)
    {
      namespace fs = std::filesystem;
      path_checker::check(logger, custom, path, prediction, [](const fs::path&, const fs::path&) { return true; });
    }

    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const std::filesystem::path& path, const std::filesystem::path& prediction)
    {
      check(logger, basic_file_checker{}, path, prediction);
    }
  };

  /*! \brief Detailed comparison of the elements of two instances of std::pair<S,T>

      The elements of a std::pair<S,T> are checked using check_equality; this will recursively
      dispatch to other specializations detailed_equality_checker if appropriate for either
      S or T.
   */
  template<class T, class S>
  struct detailed_equality_checker<std::pair<T, S>> : equivalence_checker<std::pair<T, S>>
  {};

  /*! \brief Detailed comparison of the elements of two instances of std::pair<T...> */
  template<class... T>
  struct detailed_equality_checker<std::tuple<T...>> : equivalence_checker<std::tuple<T...>>
  {};

  template<class Char, class Traits, alloc Allocator>
  struct detailed_equality_checker<std::basic_string<Char, Traits, Allocator>>
  {
    using string_type = std::basic_string<Char, Traits, Allocator>;

    template<test_mode Mode, class Advisor>
    static void check(test_logger<Mode>& logger, const string_type& obtained, const string_type& prediction, tutor<Advisor> advisor)
    {
      using checker = detailed_equality_checker<std::basic_string_view<Char, Traits>>;

      checker::check(logger, std::string_view{obtained}, std::string_view{prediction}, std::move(advisor));
    }
  };

  template<class... Ts>
  struct detailed_equality_checker<std::variant<Ts...>>
  {
    using type = std::variant<Ts...>;

    template<test_mode Mode, class Advisor>
    static void check(test_logger<Mode>& logger, const type& obtained, const type& prediction, tutor<Advisor> advisor)
    {
      if(check_equality("Variant Index", logger, obtained.index(), prediction.index()))
      {
        check(logger, obtained, prediction, advisor, std::make_index_sequence<sizeof...(Ts)>());
      }
    }
  private:
    template<test_mode Mode, class Advisor, std::size_t... I>
    static void check(test_logger<Mode>& logger, const type& obtained, const type& prediction, const tutor<Advisor>& advisor, std::index_sequence<I...>)
    {
      (check<I>(logger, obtained, prediction, advisor), ...);
    }

    template<std::size_t I, test_mode Mode, class Advisor>
    static void check(test_logger<Mode>& logger, const type& obtained, const type& prediction, const tutor<Advisor>& advisor)
    {
      if(auto pObtained{std::get_if<I>(&obtained)})
      {
        if(auto pPrediction{std::get_if<I>(&prediction)})
        {
          check_equality("Variant Contents", logger, *pObtained, *pPrediction, advisor);
        }
        else
        {
          throw std::logic_error{"Inconsistant variant access"};
        }
      }
    }
  };

  template<class T>
  struct detailed_equality_checker<std::optional<T>>
  {
    using type = std::optional<T>;

    template<test_mode Mode, class Advisor>
    static void check(test_logger<Mode>& logger, const type& obtained, const type& prediction, tutor<Advisor> advisor)
    {
      if(check_equality("Has value", logger, obtained.has_value(), prediction.has_value()))
      {
        if(obtained && prediction)
        {
          check_equality("Contents of optional", logger, *obtained, *prediction, advisor);
        }
      }
    }
  };
}
