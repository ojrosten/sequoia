////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/Output.hpp"

#include "sequoia/FileSystem/FileSystem.hpp"
#include "sequoia/TextProcessing/Characters.hpp"
#include "sequoia/TextProcessing/Patterns.hpp"
#include "sequoia/TextProcessing/Substitutions.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <limits>
#include <optional>
#include <ranges>
#include <span>
#include <type_traits>

#ifndef _MSC_VER
  #include "cxxabi.h"
#endif

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    constexpr auto npos{std::string::npos};
    using size_type = std::string::size_type;

    /** Whether `c` cannot be part of an identifier. Identifiers may hold multi-byte UTF-8 characters, so no byte
        beyond ASCII is a delimiter.
     */
    constexpr auto is_word_delimiter{[](char c){ return is_ascii(c) && !is_identifier_character(c); }};

    /** Whether a number, a digit or a `-` then a digit, begins at `pos`. */
    [[nodiscard]]
    bool begins_number(std::string_view text, size_type pos)
    {
      auto digitAt{[text](size_type i){ return (i < text.size()) && is_digit(text[i]); }};
      return digitAt(pos) || ((pos < text.size()) && (text[pos] == '-') && digitAt(pos + 1));
    }

    /** Erases every balanced parenthesised group immediately followed by a number, a digit or a `-` then a digit:
        the type a demangler writes before a literal, as in `(E)1`, `(unsigned char)3` or `(short)-6`. Groups within
        other parentheses are examined too; an unmatched `(` is left in place, and the search continues after it.
     */
    std::string& remove_literal_casts(std::string& name)
    {
      size_type from{};
      while(from < name.size())
      {
        const auto [open, close]{find_matched_delimiters(name, '(', ')', from)};
        if(open == npos)
          break;

        if(begins_number(name, close))
          name.erase(open, close - open);

        from = open + 1;
      }

      return name;
    }

    /** Where the fields of a floating-point type would lie in its bit pattern, were it laid out as IEEE 754's binary
        interchange encodings are, with the significand's integer bit stored explicitly if `ExplicitIntegerBit`, as
        x87 stores it. Counting from the least significant bit: the fraction, the integer bit if explicit, the
        exponent, then the sign.
     */
    template<std::floating_point T, bool ExplicitIntegerBit>
    struct candidate_layout
    {
      constexpr static std::size_t fraction_bits{std::numeric_limits<T>::digits - 1};
      constexpr static std::size_t exponent_bits{
        std::bit_width(static_cast<unsigned>(std::numeric_limits<T>::max_exponent))
      };

      constexpr static bool explicit_integer_bit{ExplicitIntegerBit};

      constexpr static std::size_t integer_bit   {fraction_bits};
      constexpr static std::size_t exponent_start{fraction_bits + explicit_integer_bit};
      constexpr static std::size_t sign_bit      {exponent_start + exponent_bits};
      constexpr static std::size_t bit_count     {sign_bit + 1};
      constexpr static std::size_t digit_count   {bit_count / 4};
    };

    /** Whether `T` stores -1.5 and -(1.5 + epsilon) as `candidate_layout<T, ExplicitIntegerBit>` predicts, in native
        byte order: the sign bit set, the exponent holding its bias, the integer bit set if it is explicit, the top
        fraction bit set and, for the second value, the lowest. False if the layout's bits do not fill whole bytes or
        exceed `T`, or if they fill fewer bytes than `T` on a platform that is not little-endian.
     */
    template<std::floating_point T, bool ExplicitIntegerBit>
    [[nodiscard]]
    consteval bool stores_as_candidate_layout()
    {
      using layout = candidate_layout<T, ExplicitIntegerBit>;
      constexpr std::size_t valueBytes{layout::bit_count / 8};

      // Only the bytes holding the value are compared: a constant evaluation cannot read padding
      struct alignas(T) value_representation
      {
        std::array<std::byte, valueBytes> bytes;
      };

      constexpr bool wholeBytes  {layout::bit_count % 8 == 0};
      constexpr bool fitsStorage {sizeof(value_representation) == sizeof(T)};
      constexpr bool fillsStorage{valueBytes == sizeof(T)};
      if constexpr(!wholeBytes || !fitsStorage || (!fillsStorage && (std::endian::native != std::endian::little)))
      {
        return false;
      }
      else
      {
        constexpr auto predicted{
          [](bool lowestFractionBit){
            std::array<std::byte, valueBytes> bytes{};
            auto setBit{
              [&bytes](std::size_t bitFromBottom){
                const auto byteFromBottom{bitFromBottom / 8};
                const auto byteIndex{
                  (std::endian::native == std::endian::little) ? byteFromBottom : valueBytes - 1 - byteFromBottom
                };

                bytes[byteIndex] |= std::byte{1} << (bitFromBottom % 8);
              }
            };

            constexpr unsigned bias{std::numeric_limits<T>::max_exponent - 1};
            for(const auto bit : std::views::iota(std::size_t{}, layout::exponent_bits))
            {
              if((bias >> bit) & 1u)
                setBit(layout::exponent_start + bit);
            }

            setBit(layout::sign_bit);
            setBit(layout::fraction_bits - 1);
            if constexpr(ExplicitIntegerBit)
              setBit(layout::integer_bit);

            if(lowestFractionBit)
              setBit(0);

            return bytes;
          }
        };

        constexpr auto stored{
          [](T value){ return std::bit_cast<value_representation>(value).bytes; }
        };

        constexpr T oneAndAHalf{static_cast<T>(1.5)};
        return (stored(-oneAndAHalf) == predicted(false))
            && (stored(-(oneAndAHalf + std::numeric_limits<T>::epsilon())) == predicted(true));
      }
    }

    /** A type whose values are stored in one of IEEE 754's binary interchange encodings, of the widths `<stdfloat>`
        provides: binary16, binary32, binary64 or binary128. The type has that encoding's precision, `digits`, and
        maximum exponent, emax, which is one less than `max_exponent`; and the compiler confirms that it stores two
        probe values as the encoding prescribes.
     */
    template<class T>
    concept binary_interchange_encoding
      =    std::floating_point<T>
        && (std::numeric_limits<T>::radix == 2)
        && (   ((std::numeric_limits<T>::digits ==  11) && (std::numeric_limits<T>::max_exponent ==    16))
            || ((std::numeric_limits<T>::digits ==  24) && (std::numeric_limits<T>::max_exponent ==   128))
            || ((std::numeric_limits<T>::digits ==  53) && (std::numeric_limits<T>::max_exponent ==  1024))
            || ((std::numeric_limits<T>::digits == 113) && (std::numeric_limits<T>::max_exponent == 16384)))
        && stores_as_candidate_layout<T, false>();

    /** A type whose values are stored in x87's 80-bit extended encoding, which holds the significand's integer bit
        explicitly. IEEE 754 fixes only a minimum precision and range for an extended format, not its encoding; the
        compiler confirms that the type stores two probe values as x87 does. Requiring little-endian excludes m68k's
        extended encoding, which has the same precision and maximum exponent but a different layout.
     */
    template<class T>
    concept x87_extended_encoding
      =    std::floating_point<T>
        && (std::numeric_limits<T>::radix        ==     2)
        && (std::numeric_limits<T>::digits       ==    64)
        && (std::numeric_limits<T>::max_exponent == 16384)
        && (std::endian::native == std::endian::little)
        && stores_as_candidate_layout<T, true>();

    /** A type whose values are stored in an encoding `bit_layout` describes. The encoding is all that this file
        requires of IEEE 754: bit patterns are read and built bit by bit, and IEEE 754's rules for arithmetic, which
        `-ffast-math` relaxes, are relied on only by `std::format`, which classifies a value to print an infinity or a
        NaN. Nor is `is_iec559` relied on: it stays true under `-ffast-math`.
     */
    template<class T>
    concept known_encoding = binary_interchange_encoding<T> || x87_extended_encoding<T>;

    static_assert(known_encoding<float> && known_encoding<double>,
                  "Only long double may fall back to leaving a bit pattern unread");

    /** Where the fields of a floating-point type lie in its bit pattern, counting from the least significant bit:
        the fraction lowest, then x87's explicit integer bit, if any, then the exponent, then the sign in the top bit.
     */
    template<known_encoding T>
    struct bit_layout : candidate_layout<T, x87_extended_encoding<T>>
    {};

    /** Whether bit `bitFromBottom` of the pattern `hex` is set; a bit above the pattern's top is clear, and a
        character that is not a hexadecimal digit reads as zero.
     */
    [[nodiscard]]
    bool pattern_bit(std::string_view hex, std::size_t bitFromBottom)
    {
      if(bitFromBottom >= 4 * hex.size())
        return false;

      const char& digit{hex[hex.size() - 1 - bitFromBottom / 4]};
      unsigned nibble{};
      std::from_chars(&digit, &digit + 1, nibble, 16);
      return ((nibble >> (bitFromBottom % 4)) & 1u) != 0;
    }

    /** Reads `hex` as the bit pattern of a `T`, spelled as the Itanium ABI mangles a floating-point literal: two
        hexadecimal digits per byte, the most significant byte first.
        \returns
        -# If `T` has a `known_encoding`, `hex` is a non-empty, even number of hexadecimal digits, spelling no more
           bytes than `T` has, and the pattern is a value of `T`: the `T` whose bits are the pattern, zero-extended at
           its most significant end;
        -# Otherwise, none. Every pattern of a binary interchange encoding is a value; an x87 pattern whose exponent is
           non-zero and whose integer bit is clear is not.

        The zero-extension assumes that `T`, read as an integer in native byte order, holds its value in its least
        significant bytes and any padding above them: on x86-64, x87's 80-bit `long double` is mangled as 20 digits
        and occupies 16 bytes.
     */
    template<std::floating_point T>
    [[nodiscard]]
    std::optional<T> from_bit_pattern(std::string_view hex)
    {
      static_assert((std::endian::native == std::endian::little) || (std::endian::native == std::endian::big));

      if constexpr(!known_encoding<T>)
      {
        return std::nullopt;
      }
      else
      {
        const bool wholeBytes{!hex.empty() && (hex.size() % 2 == 0) && std::ranges::all_of(hex, is_hex_digit)};
        if(!wholeBytes || (hex.size() > 2 * sizeof(T)))
          return std::nullopt;

        if constexpr(x87_extended_encoding<T>)
        {
          using layout = bit_layout<T>;
          auto isSet{[hex](std::size_t bitFromBottom){ return pattern_bit(hex, bitFromBottom); }};
          const bool exponentNonZero{
            std::ranges::any_of(std::views::iota(layout::exponent_start, layout::sign_bit), isSet)
          };

          if(exponentNonZero && !isSet(layout::integer_bit))
            return std::nullopt;
        }

        auto toByte{
          [](const char& firstDigit){
            std::uint8_t value{};
            std::from_chars(&firstDigit, &firstDigit + 2, value, 16);
            return static_cast<std::byte>(value);
          }
        };

        // TO DO: std::views::chunk(2), handing each pair to toByte, once libc++ has chunk (P2442)
        std::array<std::byte, sizeof(T)> bytes{};
        std::ranges::copy_backward(hex | std::views::stride(2) | std::views::transform(toByte), bytes.end());

        if constexpr(std::endian::native == std::endian::little)
          std::ranges::reverse(bytes);

        return std::bit_cast<T>(bytes);
      }
    }

    /** Spells a value as MSVC spells a floating-point template argument: fixed notation, six decimal places. The
        other toolchains' spellings are converted to this one, since MSVC's has already lost any further digits.
     */
    constexpr auto to_fixed_notation{[](std::floating_point auto value){ return std::format("{:.6f}", value); }};

    /** The value of type `typeName` whose bit pattern `hex` spells, in fixed notation; none if `typeName` is not
        `float`, `double` or `long double`, or if `from_bit_pattern` reads no value from `hex`.
     */
    [[nodiscard]]
    std::optional<std::string> bit_pattern_to_fixed_notation(std::string_view typeName, std::string_view hex)
    {
      if(typeName == "float")
        return from_bit_pattern<float>(hex).transform(to_fixed_notation);

      if(typeName == "double")
        return from_bit_pattern<double>(hex).transform(to_fixed_notation);

      if(typeName == "long double")
        return from_bit_pattern<long double>(hex).transform(to_fixed_notation);

      return std::nullopt;
    }

    /** The position of the first character at or after `from` that may begin a literal: a decimal digit, or the
        first character after `)[`, where libstdc++ writes a bit pattern. npos if there is none before the last
        character.
     */
    [[nodiscard]]
    size_type next_literal_start(std::string_view name, size_type from)
    {
      if(from + 1 >= name.size())
        return npos;

      auto beginsBitPattern{[name](size_type pos){ return (name[pos - 1] == '[') && (name[pos - 2] == ')'); }};
      auto mayBeginLiteral{
        [name, &beginsBitPattern](size_type pos){ return is_digit(name[pos]) || beginsBitPattern(pos); }
      };

      const auto candidates{std::views::iota(from, name.size() - 1)};
      const auto found     {std::ranges::find_if(candidates, mayBeginLiteral)};

      return found == candidates.end() ? npos : *found;
    }

    /** Respells libstdc++'s floating-point literal, a type name in parentheses and a bit pattern in brackets,
        `(double)[3ff8000000000000]`, as its value in fixed notation; `patternStart` is the position after the `[`.
        \returns
        -# The position after the respelling;
        -# The position of the closing `]`, if the type is not `float`, `double` or `long double`, or the pattern is
           not a value of it;
        -# `patternStart`, if `)[` does not precede it, no `(` precedes that, or no `]` follows.
     */
    [[nodiscard]]
    size_type respell_bit_pattern_literal(std::string& name, size_type patternStart)
    {
      const auto closeBracket{name.find(']', patternStart)};
      const auto openParen   {name.rfind('(', patternStart - 1)};
      const auto closeParen  {patternStart - 2};
      if((closeBracket == npos) || (openParen == npos) || (name[closeParen] != ')'))
        return patternStart;

      const auto typeName{std::string_view{name}.substr(openParen + 1, closeParen - openParen - 1)};
      const auto hex     {std::string_view{name}.substr(patternStart, closeBracket - patternStart)};

      const auto value{bit_pattern_to_fixed_notation(typeName, hex)};
      if(!value)
        return closeBracket;

      name.replace(openParen, closeBracket + 1 - openParen, *value);
      return openParen + value->size();
    }

    /** Respells the number `std::strtold` reads at `start`, where the next character is `x`, as its value in fixed
        notation: libc++abi writes a floating-point literal in hexadecimal, `0x1.8p+0`.
        \returns
        -# The position of the respelling's last character;
        -# `start`, if the next character is not `x` or `std::strtold` reads nothing.
     */
    [[nodiscard]]
    size_type respell_hex_float(std::string& name, size_type start)
    {
      if((start + 1 >= name.size()) || (name[start + 1] != 'x'))
        return start;

      char* end{};
      const auto first {name.data() + start};
      const auto value {std::strtold(first, &end)};
      const auto length{std::ranges::distance(first, end)};
      if(length == 0)
        return start;

      const auto fixed{to_fixed_notation(value)};
      name.replace(start, length, fixed);
      return start + fixed.size() - 1;
    }

    /** Erases the suffix at `pos` of the literal before it, as in `1ul`: the characters up to, not including, the
        next `,`, `>` or `}`.
        \returns
        -# The position after that delimiter;
        -# `pos`, if no letter is at `pos` or no delimiter follows it.
     */
    [[nodiscard]]
    size_type erase_literal_suffix(std::string& name, size_type pos)
    {
      if((pos >= name.size()) || !is_alphabetic(name[pos]))
        return pos;

      const auto suffixEnd{name.find_first_of(",>}", pos)};
      if(suffixEnd == npos)
        return pos;

      name.erase(pos, suffixEnd - pos);
      return pos + 1;
    }

    /** For each `<`, space or `{`, finds the first literal after it, past any other characters, and normalises the
        literal: libstdc++'s bit patterns and libc++abi's hexadecimal floats are respelled in fixed notation, and a
        suffix is erased.
     */
    std::string& process_literals(std::string& name)
    {
      size_type pos{};
      while(pos < name.size())
      {
        const auto open{name.find_first_of("< {", pos)};
        if(open == npos)
          break;

        pos = next_literal_start(name, open + 1);
        if(pos == npos)
          break;

        const bool continuesIdentifier{!is_word_delimiter(name[pos - 1])};
        if(continuesIdentifier)
        {
          const auto identifierEnd{std::ranges::find_if(name.begin() + pos, name.end(), is_word_delimiter)};
          pos = std::ranges::distance(name.begin(), identifierEnd);
          continue;
        }

        if(name[pos - 1] == '[')
          pos = respell_bit_pattern_literal(name, pos);

        pos = respell_hex_float(name, pos);

        if(pos + 1 < name.size())
        {
          const auto digitsEnd{std::ranges::find_if_not(name.begin() + pos + 1, name.end(), is_digit)};
          pos = std::ranges::distance(name.begin(), digitsEnd);
        }

        pos = erase_literal_suffix(name, pos);
      }

      return name;
    }

    /** Respells the dynamic extent of every `span` in `name`: libc++ and libstdc++ write the extent as the maximum
        `std::size_t`, MSVC as `-1`, which is the spelling kept. A `span` whose `<` has no matching `>` is left
        unchanged.
     */
    std::string& process_spans(std::string& name)
    {
      constexpr std::string_view spanOpening{"::span<"};
      const auto dynamicExtent{std::to_string(std::dynamic_extent)};
      for(auto start{name.find(spanOpening)}; start != npos; start = name.find(spanOpening, start + 1))
      {
        const auto [open, close]{find_matched_delimiters(name, '<', '>', start)};
        if(close == open)
          continue;

        const auto closingBracket{close - 1};
        const auto lastComma     {name.rfind(',', closingBracket)};
        if((lastComma == npos) || (lastComma < open))
          continue;

        const auto extentStart {name.find_first_not_of(' ', lastComma + 1)};
        const auto extentLength{closingBracket - extentStart};
        if(std::string_view{name}.substr(extentStart, extentLength) == dynamicExtent)
          name.replace(extentStart, extentLength, "-1");
      }

      return name;
    }

    /** Respells the MS STL's array iterators, which are class templates, as the pointers libc++ and libstdc++ use:
        `std::_Array_const_iterator<T, N>` as `T const*` and `std::_Array_iterator<T, N>` as `T*`.
     */
    void process_array_iterators(std::string& name)
    {
      auto respell{
        [&name](std::string_view opening, std::string_view pointerSuffix){
          for(auto start{name.find(opening)}; start != npos; start = name.find(opening, start))
          {
            const auto [open, close]{find_matched_delimiters(name, '<', '>', start)};
            if(close <= open)
              break;

            const auto arguments{std::string_view{name}.substr(open + 1, close - open - 2)};
            const auto pointer  {std::format("{}{}", arguments.substr(0, arguments.rfind(',')), pointerSuffix)};
            name.replace(start, close - start, pointer);

            const auto end{start + pointer.size()};
            if((end + 1 < name.size()) && (name[end] == ' ') && (name[end + 1] == '>'))
              name.erase(end, 1);
          }
        }
      };

      respell("std::_Array_const_iterator<", " const*");
      respell("std::_Array_iterator<",       "*");
    }

    /** The respellings with which the clang and gcc overloads of `tidy_name` both finish, into MSVC's spelling where
        there is one:
        -# Where `long` and `long long` have the same size, each is spelled `long long`;
        -# `true` and `false`, as template arguments or braced members, are spelled `1` and `0`;
        -# The parenthesised type before a literal is erased, as `remove_literal_casts` erases it.
     */
    std::string& tidy_name(std::string& name)
    {
      if constexpr(sizeof(unsigned long) == sizeof(unsigned long long))
      {
        // Collapse before expanding; the other way round, the collapse undoes the expansion
        replace_all(name, is_word_delimiter, "long long", is_word_delimiter, "long");
        replace_all(name, is_word_delimiter, "long",      is_word_delimiter, "long long");

        // The expansion cannot see that `long double` is not a `long`
        replace_all(name, is_word_delimiter, "long long double", is_word_delimiter, "long double");
      }

      // It is a pity to have to make the following substitutions, but it appears
      // to be by far the easiest way to ensure compiler-independent de-mangling.

      replace_all(name, " <{", "true",  ",>}", "1");
      replace_all(name, " <{", "false", ",>}", "0");

      return remove_literal_casts(name);
    }

    /** Whether the mangled name may hold an Itanium source-name - a name's length, then the name - spelling `name`.
        Every occurrence of that text counts, since the digit before one may end a previous name, as in `3ns24inff`.
     */
    [[nodiscard]]
    bool may_hold_source_name(std::string_view mangled, std::string_view name)
    {
      return mangled.contains(std::format("{}{}", name.size(), name));
    }

    /** The code of a floating-point type in an Itanium mangled literal - `L`, the code, the bit pattern, `E` - and
        libc++abi's spelling of a NaN of the type, a spelling without a sign.
     */
    template<std::floating_point T>
    struct itanium_literal;

    template<>
    struct itanium_literal<float>
    {
      constexpr static char             code         {'f'};
      constexpr static std::string_view libcxxabi_nan{"nanf"};
    };

    template<>
    struct itanium_literal<double>
    {
      constexpr static char             code         {'d'};
      constexpr static std::string_view libcxxabi_nan{"nan"};
    };

    template<>
    struct itanium_literal<long double>
    {
      constexpr static char             code         {'e'};
      constexpr static std::string_view libcxxabi_nan{"nanL"};
    };

    enum class nan_sign { positive, negative };

    /** The sign of the NaN of `T` whose bit pattern `hex` spells, at the full width of `T`, most significant digit
        first; none if `hex` spells no NaN of `T`, or if `T` has no `known_encoding`. No `T` is formed from the bits,
        so `hex` may be any text. Nor does any floating-point operation classify them, so the result holds under
        `-ffast-math`, where `std::isnan` may report a NaN as a number.
     */
    template<std::floating_point T>
    [[nodiscard]]
    std::optional<nan_sign> sign_of_nan(std::string_view hex)
    {
      if constexpr(!known_encoding<T>)
      {
        return std::nullopt;
      }
      else
      {
        using layout = bit_layout<T>;
        if((hex.size() != layout::digit_count) || !std::ranges::all_of(hex, is_hex_digit))
          return std::nullopt;

        auto isSet{[hex](std::size_t bitFromBottom){ return pattern_bit(hex, bitFromBottom); }};
        const bool isNan{   std::ranges::all_of(std::views::iota(layout::exponent_start, layout::sign_bit), isSet)
                         && std::ranges::any_of(std::views::iota(std::size_t{}, layout::fraction_bits), isSet)};
        if(!isNan)
          return std::nullopt;

        return isSet(layout::sign_bit) ? nan_sign::negative : nan_sign::positive;
      }
    }

    /** `nan` or `-nan`, whichever sign every NaN among the mangled name's literals of type `T` shares; none if there
        is no such NaN, or if there are NaNs of both signs.
     */
    template<std::floating_point T>
    [[nodiscard]]
    std::optional<std::string_view> nan_spelling(std::string_view mangled)
    {
      bool positive{}, negative{};
      const std::string literalStart{'L', itanium_literal<T>::code};
      for(auto pos{mangled.find(literalStart)}; pos != npos; pos = mangled.find(literalStart, pos + 1))
      {
        const auto hexStart{pos + literalStart.size()};
        if(const auto sign{sign_of_nan<T>(mangled.substr(hexStart, mangled.find('E', hexStart) - hexStart))})
          (*sign == nan_sign::negative ? negative : positive) = true;
      }

      if(positive == negative)
        return std::nullopt;

      return negative ? "-nan" : "nan";
    }

    /** The respelling of libc++abi's non-finite literals that `demangle` promises. */
    [[nodiscard]]
    std::string respell_non_finite_literals(std::string demangled, std::string_view mangled)
    {
      auto respell{
        [&demangled, mangled](std::string_view from, std::string_view to){
          if(!may_hold_source_name(mangled, from))
            replace_all(demangled, is_word_delimiter, from, is_word_delimiter, to);
        }
      };

      auto respellNans{
        [&respell, mangled]<std::floating_point T>(std::type_identity<T>){
          if(const auto spelling{nan_spelling<T>(mangled)})
            respell(itanium_literal<T>::libcxxabi_nan, *spelling);
        }
      };

      respell("inff", "inf");
      respell("infL", "inf");

      respellNans(std::type_identity<double>     {});
      respellNans(std::type_identity<float>      {});
      respellNans(std::type_identity<long double>{});

      return demangled;
    }

    /** The position of the colon with which MSVC ends the type of the member of a class-type template argument that
        starts at `start`; npos if that member has no type before its value.
     */
    [[nodiscard]]
    size_type member_type_end(std::string_view name, size_type start)
    {
      constexpr std::string_view delimiters{"<>(){},:"};
      std::size_t depth{};
      for(auto pos{name.find_first_of(delimiters, start)}; pos != npos; pos = name.find_first_of(delimiters, pos + 1))
      {
        switch(name[pos])
        {
        case '<':
        case '(':
          ++depth;
          break;
        case '>':
        case ')':
          if(!depth)
            return npos;
          --depth;
          break;
        case ':':
          if((pos + 1 < name.size()) && (name[pos + 1] == ':'))
            ++pos;
          else if(!depth)
            return pos;
          break;
        default:
          if(!depth)
            return npos;
        }
      }

      return npos;
    }

    /** Removes the type MSVC writes before each member of a class-type template argument: `{int:1,float:2.000000}`
        becomes `{1,2.000000}`.
     */
    std::string& remove_member_types(std::string& name)
    {
      for(auto pos{name.find_first_of("{,")}; pos != npos; pos = name.find_first_of("{,", pos + 1))
      {
        if(const auto end{member_type_end(name, pos + 1)}; end != npos)
          name.erase(pos + 1, end - pos);
      }

      return name;
    }

    [[nodiscard]]
    std::string nullable_type_message(const bool holdsValue)
    {
      return std::string{holdsValue ? "not " : ""}.append("null");
    }
  }

  [[nodiscard]]
  std::string footer()
  {
    return "=======================================\n";
  }

  [[nodiscard]]
  std::string instability_footer()
  {
    return "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n";
  }

  void end_block(std::string& s, const line_breaks newlines, std::string_view footer)
  {
    if(!s.empty())
    {
      std::size_t n{};
      for(; n < std::ranges::min(s.size(), newlines.value()); ++n)
      {
        if(s[s.size() - 1 - n] != '\n') break;
      }

      for(; n<newlines.value(); ++n)
      {
        s.append("\n");
      }

      s.append(footer);
    }
  }

  [[nodiscard]]
  std::string end_block(std::string_view s, const line_breaks newlines, std::string_view footer)
  {
    std::string text{s};
    end_block(text, newlines, footer);

    return text;
  }

  [[nodiscard]]
  std::string emphasise(std::string_view s)
  {
    if(s.empty()) return "";

    constexpr std::string_view emph{"--"};
    return std::format("{}{}{}", emph, s, emph);
  }

  [[nodiscard]]
  std::string exception_message(std::string_view tag,
                                const fs::path& filename,
                                const uncaught_exception_info& info,
                                std::string_view exceptionMessage)
  {
    auto mess{append_lines(std::format("Error -- {} Exception:", tag), exceptionMessage).append("\n")};

    if(info)
    {
      const std::string_view suffix{info->uncaught_exceptions ? "during last check" : "after check completed"};
      append_lines(
        mess,
        std::string{"Exception thrown "}.append(suffix),
        "Last Recorded Message:\n",
        info->message
      );
    }
    else
    {
      append_lines(mess, "Exception thrown before any checks performed in file", filename.generic_string());
    }

    return mess;
  }

  [[nodiscard]]
  std::string operator_message(std::string_view op, std::string_view opRetVal)
  {
    return std::string{"operator"}.append(op).append(" returned ").append(opRetVal);
  }

  [[nodiscard]]
  std::string equality_operator_failure_message()
  {
    return operator_message("==", "false");
  }

  [[nodiscard]]
  std::string pointer_prediction_message()
  {
    return "Pointers both non-null, but they point to different addresses";
  }

  [[nodiscard]]
  std::string default_prediction_message(std::string_view obtained, std::string_view prediction)
  {
    return append_lines(std::string{"Obtained : "}.append(obtained), std::string{"Predicted: "}.append(prediction));
  }

  [[nodiscard]]
  std::string prediction_message(const std::string& obtained, const std::string& prediction)
  {
    return default_prediction_message(obtained, prediction);
  }

  [[nodiscard]]
  std::string nullable_type_message(const bool obtainedHoldsValue, const bool predictedHoldsValue)
  {
    return std::string{"Obtained : "}.append(nullable_type_message(obtainedHoldsValue)).append("\n")
               .append("Predicted: ").append(nullable_type_message(predictedHoldsValue));
  }

  [[nodiscard]]
  fs::path path_for_reporting(const fs::path& file, const fs::path& repository)
  {
    auto append{
      [](fs::path lhs, const fs::path& rhs){
        lhs /= rhs;
        return lhs;
      }
    };

    if(file.is_relative())
    {
      const auto firstKept{std::ranges::find_if_not(file, [](const fs::path& p) { return p == ".."; })};
      return std::ranges::fold_left(firstKept, file.end(), fs::path{}, append);
    }

    if(repository.is_absolute())
    {
      const auto repositoryDirectory{repository.has_filename() ? repository : repository.parent_path()};
      const auto firstBeyond{std::ranges::mismatch(file, repositoryDirectory).in1};
      return std::ranges::fold_left(firstBeyond, file.end(), back(repositoryDirectory), append);
    }

    return file;
  }

  [[nodiscard]]
  std::string report_line(std::string_view message, const fs::path& repository, const std::source_location loc)
  {
    return append_lines(std::format("{}, Line {}", path_for_reporting(loc.file_name(), repository).generic_string(), loc.line()), message).append("\n");
  }

  [[nodiscard]]
  std::string tidy_name(std::string name, clang_type)
  {
    replace_all(name, "::__1::", "::");
    replace_all(name, "::__fs::", "::");
    replace_all_recursive(name, ">>", "> >");
    process_literals(name);
    process_spans(name);

    return tidy_name(name);
  }

  [[nodiscard]]
  std::string tidy_name(std::string name, gcc_type)
  {
    replace_all(name, "__cxx11::", "");
    replace_all(name, "_V2::", "");  
    replace_all_recursive(name, ">>", "> >");
    process_literals(name);
    process_spans(name);

    return tidy_name(name);
  }

  [[nodiscard]]
  std::string tidy_name(std::string name, msvc_type)
  {
    auto peel{
      [](std::string& s, std::string_view prefix){
        if(s.size() >= prefix.size())
        {
          std::string_view sv{s};
          auto start{sv.substr(0, prefix.size())};
          if(start == prefix)
            s.erase(0, prefix.size());
        }
      }
    };

    peel(name, "struct ");
    peel(name, "class ");
    peel(name, "enum ");

    replace_all(name, "<,{", "struct ", "", "");
    replace_all(name, "<,{", "class ",  "", "");
    replace_all(name, "<,{", "enum ",   "", "");

    remove_member_types(name);
    replace_all(name, "nan(ind)",  "nan");
    replace_all(name, "nan(snan)", "nan");

    replace_all(name, ",", ", ");
    replace_all(name, " ,", ",");

    replace_all(name, " & __ptr64", "&");
    replace_all(name, " * __ptr64", "*");

    replace_all(name, "`anonymous namespace'", "(anonymous namespace)");

#ifdef _MSC_VER
    if constexpr(sizeof(__int64) == sizeof(long))
    {
      replace_all(name, "__int64", "long");
    }
    else if constexpr(sizeof(__int64) == sizeof(long long))
    {
      replace_all(name, "__int64", "long long");
    }
#endif

    replace_all(name, "__cdecl(void)", "()");
    replace_all(name, "__cdecl", "");
    replace_all(name, ")(void)", ")()");

    process_array_iterators(name);

    return name;
  }

  [[nodiscard]]
  std::string tidy_name(std::string name, other_compiler_type)
  {
    return name;
  }

  [[nodiscard]]
  std::string demangle(const std::type_info& info)
  {
    return tidy_name(demangle(std::string{info.name()}), compiler_constant{});
  }

  [[nodiscard]]
  std::string demangle(std::string mangled)
  {
    if constexpr(with_clang_v || with_gcc_v)
    {
      struct cxa_demangler
      {
        cxa_demangler(const std::string& name)
          : data{demangle(name)}
        {}

        ~cxa_demangler() { std::free(data); }

#ifndef _MSC_VER
        char* demangle(const std::string& name)
        {
          return abi::__cxa_demangle(name.data(), 0, 0, &status);
        }
#else
        char* demangle(const std::string&) { return nullptr; }
#endif

        int status{-1};
        char* data;
      };

      cxa_demangler c{mangled};

      if(!c.status)
        return respell_non_finite_literals(c.data, mangled);
    }

    return mangled;
  }
}
