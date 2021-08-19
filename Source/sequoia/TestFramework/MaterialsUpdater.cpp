////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/MaterialsUpdater.hpp"

#include "sequoia/TestFramework/CoreInfrastructure.hpp"
#include "sequoia/TestFramework/FileEditors.hpp"
#include "sequoia/TestFramework/FileSystem.hpp"

namespace sequoia::testing
{
  namespace fs = std::filesystem;


  void copy_special_files(const fs::path& from, const fs::path& to)
  {
    for(auto& p : fs::recursive_directory_iterator(to))
    {
      if(fs::is_regular_file(p) && ((p.path().extension() == seqpat) || (p.path().filename() == ".keep")))
      {
        const auto predRelDir{fs::relative(p.path().parent_path(), to)};
        const auto workingSubdir{from / predRelDir};

        if(p.path().extension() == seqpat)
        {
          for(auto& w : fs::directory_iterator(workingSubdir))
          {
            if((w.path().stem() == p.path().stem()) && (w.path().extension() != seqpat))
            {
              fs::copy(p, workingSubdir, fs::copy_options::overwrite_existing);
              break;
            }
          }
        }
        else if(p.path().filename() == ".keep")
        {
          fs::copy(p, workingSubdir, fs::copy_options::overwrite_existing);
        }
      }
    }
  }

  namespace
  {
    struct path_info
    {
      fs::path full, relative;
    };

    struct compare
    {
      bool operator()(const path_info& lhs, const path_info& rhs) const
      {
        using type = std::underlying_type_t<fs::file_type>;

        return (lhs.relative == rhs.relative)
          ? static_cast<type>(fs::status(lhs.full).type()) < static_cast<type>(fs::status(rhs.full).type())
          : lhs.relative < rhs.relative;
      };
    };

    [[nodiscard]]
    std::vector<path_info> sort_dir_entries(const fs::path& dir)
    {
      std::vector<path_info> paths{};
      for(auto& entry : fs::directory_iterator(dir))
      {
        paths.push_back({entry.path(), fs::relative(entry.path(), dir)});
      }

      std::sort(paths.begin(), paths.end(), compare{});
      return paths;
    }

    using paths_iter = std::vector<path_info>::const_iterator;

    void soft_update(const fs::path& from, const fs::path& to, paths_iter fromBegin, paths_iter fromEnd, paths_iter toBegin, paths_iter toEnd)
    {
      auto equiv{
        [](const path_info& lhs, const path_info& rhs) {
          using type = std::underlying_type_t<fs::file_type>;

          return (lhs.relative == rhs.relative) && (static_cast<type>(fs::status(lhs.full).type()) == static_cast<type>(fs::status(rhs.full).type()));
        }
      };

      auto iters{std::mismatch(fromBegin, fromEnd, toBegin, toEnd, equiv)};
      for(auto fi{fromBegin}, ti{toBegin}; fi != iters.first; ++fi, ++ti)
      {
        const auto pathType{fs::status(fi->full).type()};

        switch(pathType)
        {
        case fs::file_type::regular:
        {
          const auto [rFrom, rTo] {get_reduced_file_content(fi->full, ti->full)};
          if(rFrom && rTo)
          {
            if(rFrom.value() != rTo.value())
            {
              fs::copy_file(fi->full, ti->full, fs::copy_options::overwrite_existing);
            }
          }
          break;
        }
        case fs::file_type::directory:
        {
          testing::soft_update(fi->full, ti->full);
          break;
        }
        default:
          throw std::logic_error{std::string{"Detailed equivalance check for paths of type '"}
            .append(serializer<fs::file_type>::make(pathType)).append("' not currently implemented")};
        }
      }

      if((iters.first != fromEnd) && (iters.second != toEnd))
      {
        for(; (iters.first != fromEnd) && (compare{}(*iters.first, *iters.second)); ++iters.first)
        {
          fs::copy(iters.first->full, to, fs::copy_options::recursive);
        }

        fs::remove_all(iters.second->full);
        soft_update(from, to, iters.first, fromEnd, ++iters.second, toEnd);
      }
      else if(iters.first != fromEnd)
      {
        for(; iters.first != fromEnd; ++iters.first)
        {
          fs::copy(iters.first->full, to, fs::copy_options::recursive);
        }
      }
      else if(iters.second != toEnd)
      {
        for(; iters.second != toEnd; ++iters.second)
        {
          fs::remove_all(iters.second->full);
        }
      }
    }
  }

  void soft_update(const fs::path& from, const fs::path& to)
  {
    throw_unless_exists(from);
    throw_unless_exists(to);

    if(*(--from.end()) != *(--to.end()))
      throw std::runtime_error{"Soft update requires the names of the from/to directories to be the same"};

    copy_special_files(from, to);

    const std::vector<path_info>
      sortedFromEntries{sort_dir_entries(from)},
      sortedToEntries{sort_dir_entries(to)};

    soft_update(from, to, sortedFromEntries.begin(), sortedFromEntries.end(), sortedToEntries.begin(), sortedToEntries.end());
  }

}