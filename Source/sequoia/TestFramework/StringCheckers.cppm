////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

module;

#include "sequoia/PlatformSpecific/Macros.hpp"
#include "sequoia/TestFramework/Macros.hpp"

export module sequoia.test_framework:StringCheckers;

import std;

import :Advice;
import :BinaryRelationships;
import :CoreInfrastructure;
import :FailureInfo;
import :FreeCheckers;
import :Output;
import :ProjectPaths;
import :TestLogger;
import :TestMode;
export import sequoia.core.meta;
export import sequoia.platform_specific;
export import sequoia.text_processing;

/** \file
    \brief Specializations of the class template value_tester for `std::basic_string_view` and `std::basic_string`.

    See ConcreteTypeCheckers.hpp for how nested checks dispatch, and for a single
    header pulling in every specialization at once.
 */

export namespace sequoia::testing
{
  /** \brief Comparisons for `std::basic_string_view`
  
      Some support is offered for wide string views etc., though test failures are ultimately reported
      using normal strings, which has its limitations when the character type is bigger than a `char`. 
   */
  template<class Char, class Traits>
  struct value_tester<std::basic_string_view<Char, Traits>>
  {
    using string_view_type = std::basic_string_view<Char, Traits>;

    template<class Allocator>
    using string_type      = std::basic_string<Char, Traits, Allocator>;
  private:
    using iter_type = string_view_type::const_iterator;
    using size_type = string_view_type::size_type;

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
        std::ranges::transform(sv.begin() + pos, sv.begin() + end, std::back_inserter(mess), [](Char c) { return static_cast<char>(c); });
      }
    }

    template<class Advisor>
    static auto make_advisor(std::string_view info, string_view_type obtained, string_view_type prediction, size_type pos, const tutor<Advisor>& advisor)
    {
      if constexpr(std::invocable<tutor<Advisor>, Char, Char>)
      {
        return
	  tutor{
            [=, &advisor] (Char a, Char b) {
              auto m{build_preliminary_message(info, obtained, prediction, pos)};
              return append_advice(m, {advisor, a, b});
            },
            "\n"
          };
      }
      else
      {
        return
	  tutor{
            [=](const auto&, const auto&) {
	      return build_preliminary_message(info, obtained, prediction, pos);
	    },
            "\n"
          };
      }
    }

    [[nodiscard]]
    static std::string build_preliminary_message(std::string_view info, string_view_type obtained, string_view_type prediction, size_type pos)
    {
      constexpr size_type defaultOffset{30}, defaultCount{60}, npos{string_view_type::npos};
      const auto sz{std::ranges::min(obtained.size(), prediction.size())};

      auto newlineBackwards{ [pos](string_view_type sv){ return pos < sv.size() ? sv.rfind('\n', pos) : npos; } };

      const auto loc{std::ranges::min(newlineBackwards(prediction), newlineBackwards(obtained))};

      const size_type offset{loc < npos ? std::ranges::min(defaultOffset, pos - loc) : defaultOffset};

      const auto startPos{pos < offset ? 0 :
                             pos < sz  ? pos - offset :
		                         sz - std::ranges::min(sz, offset)};

      struct message{ std::string mess; bool trunc{}; };

      auto make{
        [](string_view_type sv, size_type lpos) -> message {
          std::string mess{lpos > 0 ? "..." : ""};

          const bool newline{(lpos < sv.size()) && (sv[lpos] == '\n')};
          if(newline) mess.append("\\n");

          const auto rpos{sv.find('\n', lpos+1)};
          const auto count{rpos == npos ? defaultCount : rpos - lpos};

          if(newline && (count == 1))
          {
            mess.append("\\n");
          }
          else
          {
            if(newline) ++lpos;
            appender(mess, sv, lpos, count);
          }

          const bool trunc{lpos + count < sv.size()};
          if(trunc) mess.append("...");

          return {mess, trunc};
        }
      };

      const auto[obMess,obTrunc]{make(obtained  , startPos)};
      const auto[prMess,prTrunc]{make(prediction, startPos)};

      const bool trunc{startPos > 0 || obTrunc || prTrunc};
      return append_lines(info,  trunc ? "Surrounding substring(s):" : "Full strings:", prediction_message(obMess, prMess));
    }
  public:
    template<test_mode Mode, class Advisor>
    static void test(equality_check_t, test_logger<Mode>& logger, string_view_type obtained, string_view_type prediction, const tutor<Advisor>& advisor)
    {
      auto iters{std::ranges::mismatch(obtained, prediction)};

      if((iters.in1 != obtained.end()) && (iters.in2 != prediction.end()))
      {
        const auto dist{std::ranges::distance(obtained.begin(), iters.in1)};
        auto adv{make_advisor("", obtained, prediction, dist, advisor)};

        const auto numLines{std::count(prediction.begin(), iters.in2, '\n')};

        const auto mess{
          [dist,numLines]() {
            std::string m{"First difference detected "};
            numLines > 0 ? m.append("on line ").append(std::to_string(numLines+1))
                         : m.append("at character ").append(std::to_string(dist));

            return m.append(":");
          }()
        };

        check(equality, mess, logger, *(iters.in1), *(iters.in2), adv);
      }
      else if((iters.in1 != obtained.end()) || (iters.in2 != prediction.end()))
      {
        auto checker{
          [&logger, obtained, prediction, &advisor](auto begin, auto iter, std::string_view state, std::string_view adjective){
            const auto dist{std::ranges::distance(begin, iter)};
            const auto info{std::string{"First "}.append(state).append(" character: ").append(display_character(*iter))};
            auto adv{make_advisor(info, obtained, prediction, dist, advisor)};

            const auto mess{append_lines("Lengths differ", std::string{"Obtained string is too "}.append(adjective))};

            check(equality, mess, logger, obtained.size(), prediction.size(), adv);
          }
        };

        if(iters.in2 != prediction.end())
        {
          checker(prediction.begin(), iters.in2, "missing", "short");
        }
        else if(iters.in1 != obtained.end())
        {
          checker(obtained.begin(), iters.in1, "excess", "long");
        }
      }
    }

    template<test_mode Mode, class Advisor, class Allocator>
    static void test(equivalence_check_t, test_logger<Mode>& logger, string_view_type obtained, string_type<Allocator> prediction, const tutor<Advisor>& advisor)
    {
      test(equality, logger, obtained, string_view_type{prediction}, advisor);
    }
  };

  /** \brief Comparisons for `std::basic_string`

      Some support is offered for wide string views etc., though test failures are ultimately reported
      using normal strings, which has its limitations when the character type is bigger than a `char`.
   */
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

}
