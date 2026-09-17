////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/FileSystem/FileSystem.hpp"
#include "sequoia/TestFramework/FileSystemUtilities.hpp"

#include "sequoia/TestFramework/Output.hpp"
#include "sequoia/TextProcessing/Substitutions.hpp"

#include <algorithm>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    template<std::predicate<std::filesystem::path> Pred>
    void throw_if(const std::filesystem::path& p, std::string_view message, Pred pred)
    {
      if(pred(p))
      {
        throw std::runtime_error{p.empty() ? std::string{message} : p.generic_string().append(" ").append(message)};
      }
    }
  }

  [[nodiscard]]
  std::string serializer<fs::path>::make(const fs::path& p)
  {
    return p.generic_string();
  }

  [[nodiscard]]
  std::string serializer<fs::file_type>::make(const fs::file_type& val)
  {
    using ft = fs::file_type;
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

  void throw_unless_exists(const fs::path& p, std::string_view message)
  {
    throw_if(
      p,
      append_lines(p.empty() ? "File path is empty" :"not found", message),
      [](const fs::path& pth){ return !fs::exists(pth); });
  }

  void throw_unless_directory(const fs::path& p, std::string_view message)
  {
    throw_if(p, append_lines(p.empty() ? "File path is empty" : "is not a directory", message), [](const fs::path& pth){ return !fs::is_directory(pth); });
  }

  void throw_unless_regular_file(const fs::path& p, std::string_view message)
  {
    throw_unless_exists(p, message);
    throw_if(p, append_lines(" is not a regular file", message), [](const fs::path& pth){ return !fs::is_regular_file(pth); });
  }

  [[nodiscard]]
  fs::path find_in_tree(const fs::path& root, const fs::path& toFind)
  {
    throw_unless_directory(root, "");

    using dir_iter = fs::recursive_directory_iterator;

    if(const auto toFindLen{std::ranges::distance(toFind)}; toFindLen)
    {
      for(const auto& i : dir_iter{root})
      {
        const auto p{i.path()};
        if(const auto entryPathLen{std::ranges::distance(p)}; entryPathLen >= toFindLen)
        {
          auto entryIter{p.end()}, toFindIter{toFind.begin()};

          // MSVC 17.5.3 objects to std::ranges::advance at runtime since
          // it asserts that path::iterator isn't bidirectional
          using diff_t = std::remove_const_t<decltype(toFindLen)>;
          for(diff_t n{}; n < toFindLen; ++n) --entryIter;

          while(entryIter != p.end())
          {
            if(*entryIter != *toFindIter++) break;

            ++entryIter;
          }

          if(entryIter == p.end()) return p;
        }
      }
    }

    return {};
  }

  [[nodiscard]]
  fs::path rebase_from(const fs::path& p, const fs::path& dir)
  {
    if(p.empty())
      return p;

    if(dir.empty())
      throw std::runtime_error{"Trying to rebase from an empty path"};

    if(fs::exists(dir) && !fs::is_directory(dir))
      throw std::runtime_error{"Trying to rebase from something other than a directory"};

    if(p.is_absolute() && dir.is_absolute())
      return fs::relative(p, dir);

    // A trailing separator iterates as an empty final component, which would match nothing
    // in the directory and would survive from the path into the result.
    const auto trimmed{[](const fs::path& q) { return q.has_filename() ? q : q.parent_path(); }};
    const fs::path trimmedPath{trimmed(p)}, trimmedDir{trimmed(dir)};

    const auto firstKept{std::ranges::find_if_not(trimmedPath, [](const fs::path& pth) { return pth == ".."; })};
    if(firstKept == trimmedPath.end())
      throw std::runtime_error{"Path comprises nothing but ../"};

    // An overlap is a prefix of the path which is also a suffix of the directory: the path
    // was spelt from an ancestor of the directory, and what follows the overlap lies beneath
    // the directory. The longest overlap starts earliest in the directory, so the first
    // suffix which matches wins.
    const auto join{[](fs::path lhs, const fs::path& rhs){ return lhs /= rhs; }};
    for(auto suffixBegin{trimmedDir.begin()}; suffixBegin != trimmedDir.end(); ++suffixBegin)
    {
      const auto [dirIter, rebasedPathIter]{std::ranges::mismatch(suffixBegin, trimmedDir.end(), firstKept, trimmedPath.end())};
      if(dirIter == trimmedDir.end())
        return std::ranges::fold_left(rebasedPathIter, trimmedPath.end(), fs::path{}, join);
    }

    return std::ranges::fold_left(firstKept, trimmedPath.end(), fs::path{}, join);
  }
}
