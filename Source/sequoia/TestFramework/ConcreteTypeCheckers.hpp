////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Useful specializations for the class template value_tester.

    The specializations in this header are for various types defined in std. Internally,
    check(equality,...) / (check,equivalence,...) is used meaning that there will be automatic,
    recursive dispatch to
    other specializations of value_tester, if appropriate. For example,
    consider two instances of std::pair<my_type1, mytype2>, x and y. The utilities in this
    header means the call

    check(equality, "descripion", logger, x, y);

    will automatically call

    check(equality, "automatically enhanced desciption", logger, x.first, y,first)

    and similarly for the second element. In turn, this nested check_equality will use
    a specialization of the value_tester of my_type1, should it exist. As
    usual, if the specialization for T does not exist, but T may be interpreted as
    a container holding a type U, then everything will simply work, provided either that
    there exists a specialization of the value_tester for U or U is serializable.
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
  /*! \brief Checks equality of std::basic_string_view */
  template<class Char, class Traits>
  struct value_tester<std::basic_string_view<Char, Traits>>
  {
    using string_view_type = std::basic_string_view<Char, Traits>;
  private:
    using iter_type = typename string_view_type::const_iterator;
    using size_type = typename string_view_type::size_type;

    static void appender(std::string& mess, string_view_type sv, size_type pos, size_type count)
    {
      if constexpr(sizeof(char) >= sizeof(Char))
      {
        mess.append(sv.substr(pos, count));
      }
      else
      {
        if(pos > sv.size()) throw std::out_of_range{"pos out of range"};
        const auto end{count > sv.size() - pos ? sv.size() : pos + count};
        std::transform(sv.begin() + pos, sv.begin() + end, std::back_inserter(mess), [](Char c) { return static_cast<char>(c); });
      }
    }

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

          const auto pos{(lpos < sv.size()) && (sv[lpos] == '\n') ? lpos + 1 : lpos};
          if(pos == lpos + 1) mess.append("\\n");

          const auto rpos{sv.find('\n', lpos+1)};
          const auto count{rpos == npos ? defaultCount : rpos - lpos};

          if((count == 1) && (sv[lpos] == '\n'))
          {
            mess.append("\\n");
          }
          else
          {
            appender(mess, sv, pos, count);
          }

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
    static void test(equality_check_t, test_logger<Mode>& logger, string_view_type obtained, string_view_type prediction, const tutor<Advisor>& advisor)
    {
      auto iters{std::mismatch(obtained.begin(), obtained.end(), prediction.begin(), prediction.end())};

      if((iters.first != obtained.end()) && (iters.second != prediction.end()))
      {
        const auto dist{std::distance(obtained.begin(), iters.first)};
        auto adv{make_advisor("", obtained, prediction, dist, advisor)};

        const auto numLines{std::count(prediction.begin(), iters.second, '\n')};

        const auto mess{
          [dist,numLines]() {
            std::string m{"First difference detected "};
            numLines > 0 ? m.append("on line ").append(std::to_string(numLines+1))
                         : m.append("at character ").append(std::to_string(dist));

            return m.append(":");
          }()
        };

        check(equality, mess, logger, *(iters.first), *(iters.second), adv);
      }
      else if((iters.first != obtained.end()) || (iters.second != prediction.end()))
      {
        auto checker{
          [&logger, obtained, prediction, &advisor](auto begin, auto iter, std::string_view state, std::string_view adjective){
            const auto dist{std::distance(begin, iter)};
            const auto info{std::string{"First "}.append(state).append(" character: ").append(display_character(*iter))};
            auto adv{make_advisor(info, obtained, prediction, dist, advisor)};

            const auto mess{append_lines("Lengths differ", std::string{"Obtained string is too "}.append(adjective))};

            check(equality, mess, logger, obtained.size(), prediction.size(), adv);
          }
        };

        if(iters.second != prediction.end())
        {
          checker(prediction.begin(), iters.second, "missing", "short");
        }
        else if(iters.first != obtained.end())
        {
          checker(obtained.begin(), iters.first, "excess", "long");
        }
      }
    }
  };

  template<class Char, class Traits, alloc Allocator>
  struct value_tester<std::basic_string<Char, Traits, Allocator>>
  {
    using string_type = std::basic_string<Char, Traits, Allocator>;
    using string_view_type = std::basic_string_view<Char, Traits>;

    template<test_mode Mode, class Advisor>
    static void test(equality_check_t, test_logger<Mode>& logger, const string_type& obtained, const string_type& prediction, tutor<Advisor> advisor)
    {
      using tester = value_tester<std::basic_string_view<Char, Traits>>;

      tester::test(equality_check_t{}, logger, string_view_type{obtained}, string_view_type{prediction}, std::move(advisor));
    }

    template<test_mode Mode, std::size_t N, class Advisor>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const string_type& obtained, char const (&prediction)[N], tutor<Advisor> advisor)
    {
       using tester = value_tester<std::basic_string_view<Char, Traits>>;

       tester::test(equality_check_t{}, logger, string_view_type{obtained}, string_view_type{prediction}, std::move(advisor));
    }

    template<test_mode Mode, class Advisor>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const string_type& obtained, std::basic_string_view<Char, Traits> prediction, tutor<Advisor> advisor)
    {
       using tester = value_tester<std::basic_string_view<Char, Traits>>;

       tester::test(equality_check_t{}, logger, string_view_type{obtained}, string_view_type{prediction}, std::move(advisor));
    }
  };

  /*! \brief Checks equivalence of std::pair<S,T> and std::pair<U,V> where,
      after removing references and cv-qualifiers, S,U and T,V are each the same
   */
  template<class S, class T>
  struct value_tester<std::pair<S, T>>
  {
    template<class CheckType, test_mode Mode, class U, class V, class Advisor>
      requires (   std::is_same_v<std::remove_cvref_t<S>, std::remove_cvref_t<U>>
                && std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<V>>)
    static void test(CheckType flavour, test_logger<Mode>& logger, const std::pair<S, T>& value, const std::pair<U, V>& prediction, const tutor<Advisor>& advisor)
    {
      check_elements(flavour, logger, value, prediction, std::move(advisor));
    }

  template<class CheckType, test_mode Mode, class Advisor>
  static void test(equality_check_t, test_logger<Mode>& logger, const std::pair<S, T>& value, const std::pair<S, T>& prediction, const tutor<Advisor>& advisor)
  {
    check_elements(equality, logger, value, prediction, std::move(advisor));
  }

  private:
    template<class CheckType, test_mode Mode, class U, class V, class Advisor>
    static void check_elements(CheckType, test_logger<Mode>& logger, const std::pair<S, T>& value, const std::pair<U, V>& prediction, const tutor<Advisor>& advisor)
    {
      check(CheckType{}, "First element of pair is incorrect", logger, value.first, prediction.first, advisor);
      check(CheckType{}, "Second element of pair is incorrect", logger, value.second, prediction.second, advisor);
    }
  };

  /*! \brief Checks equivalence of std::tuple<T...> and std::tuple<U...> where T... and U...
      are the same size and, after removing references and cv-qualifiers, the respective elements
      are of the same type
   */
  template<class... T>
  struct value_tester<std::tuple<T...>>
  {
  private:
    template<std::size_t I = 0, class CheckType, test_mode Mode, class... U, class Advisor>
      requires (I < sizeof...(T))
    static void check_tuple_elements(CheckType flavour, test_logger<Mode>& logger, const std::tuple<T...>& value, const std::tuple<U...>& prediction, const tutor<Advisor>& advisor)
    {
      const std::string message{"Element " + std::to_string(I) + " of tuple incorrect"};
      check(equality, message, logger, std::get<I>(value), std::get<I>(prediction), advisor);
      check_tuple_elements<I+1>(flavour, logger, value, prediction, advisor);
    }

    template<std::size_t I = 0, class CheckType, test_mode Mode, class... U, class Advisor>
    static void check_tuple_elements(CheckType, test_logger<Mode>&, const std::tuple<T...>&, const std::tuple<U...>&, const tutor<Advisor>&)
    {}

  public:
    template<class CheckType, test_mode Mode, class... U, class Advisor>
      requires ((sizeof...(T) == sizeof...(U)) && (std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>> && ...))
    static void test(CheckType flavour, test_logger<Mode>& logger, const std::tuple<T...>& value, const std::tuple<U...>& prediction, const tutor<Advisor>& advisor)
    {
      check_tuple_elements(flavour, logger, value, prediction, advisor);
    }

    template<class CheckType, test_mode Mode, class Advisor>
    static void test(equality_check_t, test_logger<Mode>& logger, const std::tuple<T...>& value, const std::tuple<T...>& prediction, const tutor<Advisor>& advisor)
    {
      check_tuple_elements(equality, logger, value, prediction, advisor);
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
        check(equality, path_check_preamble("Contents of", file, prediction), logger, reducedWorking.value(), reducedPrediction.value());
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

  /*! \brief Checks equivalence of filesystem paths.

    Files are considered equivalent if they have the same name and the same contents;
    similarly directories.

    Files are considered weakly equivalent if they have the same contents;
    similarly directories. The names of both are ignored.

   */

  template<>
  struct value_tester<std::filesystem::path>
  {
    template<class ValueBasedCustomization, test_mode Mode>
    static void test(general_equivalence_check_t<ValueBasedCustomization> checker, test_logger<Mode>& logger, const std::filesystem::path& path, const std::filesystem::path& prediction)
    {
      namespace fs = std::filesystem;

      auto pred{
        [&logger](const fs::path& pathFinalToken, const fs::path& predictionFinalToken)
        {
           return check(equality, "Final path token", logger, pathFinalToken, predictionFinalToken);
        }
      };

      check_path(logger, checker.customizer, path, prediction, pred);
    }

    template<test_mode Mode>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const std::filesystem::path& path, const std::filesystem::path& prediction)
    {
      test(general_equivalence_check_t<basic_file_checker>{}, logger, path, prediction);
    }

    template<class ValueBasedCustomization, test_mode Mode>
    static void test(general_weak_equivalence_check_t<ValueBasedCustomization> checker, test_logger<Mode>& logger, const std::filesystem::path& path, const std::filesystem::path& prediction)
    {
      namespace fs = std::filesystem;
      check_path(logger, checker.customizer, path, prediction, [](const fs::path&, const fs::path&) { return true; });
    }

    template<test_mode Mode>
    static void test(weak_equivalence_check_t, test_logger<Mode>& logger, const std::filesystem::path& path, const std::filesystem::path& prediction)
    {
      test(general_weak_equivalence_check_t<basic_file_checker>{}, logger, path, prediction);
    }
  private:
    constexpr static std::array<std::string_view, 2>
      excluded_files{".DS_Store", ".keep"};

    constexpr static std::array<std::string_view, 1>
      excluded_extensions{seqpat};

    template<test_mode Mode, class Customization, invocable_r<bool, std::filesystem::path, std::filesystem::path> FinalTokenComparison>
    static void check_path(test_logger<Mode>& logger, const Customization& custom, const std::filesystem::path& path, const std::filesystem::path& prediction, FinalTokenComparison compare)
    {
      namespace fs = std::filesystem;

      const auto pathType{fs::status(path).type()};
      const auto predictionType{fs::status(prediction).type()};

      if(check(equality, path_check_preamble("Path type", path, prediction), logger, pathType, predictionType))
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

    template<test_mode Mode, class Customization, invocable_r<bool, std::filesystem::path, std::filesystem::path> FinalTokenComparison>
    static void check_directory(test_logger<Mode>& logger, const Customization& custom, const std::filesystem::path& dir, const std::filesystem::path& prediction, FinalTokenComparison compare)
    {
      namespace fs = std::filesystem;

      auto generator{
        [](const fs::path& dir) {
          std::vector<fs::path> paths{};
          for(const auto& p : fs::directory_iterator(dir))
          {
            if(std::find(excluded_files.begin(),      excluded_files.end(),      p.path().filename()) == excluded_files.end()
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

      check(equality, std::string{"Number of directory entries for "}.append(dir.generic_string()),
        logger,
        paths.size(),
        predictedPaths.size());

      const auto iters{std::mismatch(paths.begin(), paths.end(), predictedPaths.begin(), predictedPaths.end(),
          [&dir,&prediction](const fs::path& lhs, const fs::path& rhs) {
            return fs::relative(lhs, dir) == fs::relative(rhs, prediction);
          })};
      if((iters.first != paths.end()) && (iters.second != predictedPaths.end()))
      {
        check(equality, "First directory entry mismatch", logger, *iters.first, *iters.second);
      }
      else if(iters.first != paths.end())
      {
        check(equality, "First directory entry mismatch", logger, *iters.first, fs::path{});
      }
      else if(iters.second != predictedPaths.end())
      {
        check(equality, "First directory entry mismatch", logger, fs::path{}, *iters.second);
      }
      else
      {
        for(std::size_t i{}; i < paths.size(); ++i)
        {
          check_path(logger, custom, paths[i], predictedPaths[i], compare);
        }
      }
    }

    template<test_mode Mode, class Customization>
    static void check_file(test_logger<Mode>& logger, const Customization& custom, const std::filesystem::path& file, const std::filesystem::path& prediction)
    {
      custom.check_file(logger, file, prediction);
    }
  };

  template<class... Ts>
  struct value_tester<std::variant<Ts...>>
  {
    using type = std::variant<Ts...>;

    template<class CheckType, test_mode Mode, class Advisor>
    static void test(CheckType flavour, test_logger<Mode>& logger, const type& obtained, const type& prediction, tutor<Advisor> advisor)
    {
      if(check(flavour, "Variant Index", logger, obtained.index(), prediction.index()))
      {
        check_value(flavour, logger, obtained, prediction, advisor, std::make_index_sequence<sizeof...(Ts)>());
      }
    }
  private:
    template<class CheckType, test_mode Mode, class Advisor, std::size_t... I>
    static void check_value(CheckType flavour, test_logger<Mode>& logger, const type& obtained, const type& prediction, const tutor<Advisor>& advisor, std::index_sequence<I...>)
    {
      (check_value<I>(flavour, logger, obtained, prediction, advisor), ...);
    }

    template<std::size_t I, class CheckType, test_mode Mode, class Advisor>
    static void check_value(CheckType flavour, test_logger<Mode>& logger, const type& obtained, const type& prediction, const tutor<Advisor>& advisor)
    {
      if(auto pObtained{std::get_if<I>(&obtained)})
      {
        if(auto pPrediction{std::get_if<I>(&prediction)})
        {
          check(flavour, "Variant Contents", logger, *pObtained, *pPrediction, advisor);
        }
        else
        {
          throw std::logic_error{"Inconsistant variant access"};
        }
      }
    }
  };

  template<class T>
  struct value_tester<std::optional<T>>
  {
    using type = std::optional<T>;

    template<class CheckType, test_mode Mode, class Advisor>
    static void test(CheckType flavour, test_logger<Mode>& logger, const type& obtained, const type& prediction, tutor<Advisor> advisor)
    {
      if(check(flavour, "Has value", logger, obtained.has_value(), prediction.has_value()))
      {
        if(obtained && prediction)
        {
          check(flavour, "Contents of optional", logger, *obtained, *prediction, advisor);
        }
      }
    }
  };
}
