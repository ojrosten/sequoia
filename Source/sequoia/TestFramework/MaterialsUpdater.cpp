////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for MaterialsUpdater.hpp
*/

#include "sequoia/TestFramework/MaterialsUpdater.hpp"

#include "sequoia/TestFramework/CoreInfrastructure.hpp"
#include "sequoia/TestFramework/FileEditors.hpp"
#include "sequoia/TestFramework/FileSystemUtilities.hpp"
#include "sequoia/TestFramework/ProjectPaths.hpp"

#include <algorithm>

namespace sequoia::testing
{
  namespace fs = std::filesystem;


  void copy_special_files(const fs::path& from, const fs::path& to)
  {
    for(auto& p : fs::directory_iterator(to))
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
      [[nodiscard]]
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

      std::ranges::sort(paths, compare{});
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

      auto iters{std::ranges::mismatch(fromBegin, fromEnd, toBegin, toEnd, equiv)};
      for(auto fi{fromBegin}, ti{toBegin}; fi != iters.in1; ++fi, ++ti)
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

      if(iters.in1 != fromEnd)
      {
        for(; (iters.in1 != fromEnd) && ((iters.in2 == toEnd) || (compare{}(*iters.in1, *iters.in2))); ++iters.in1)
        {
          if(fs::is_directory(iters.in1->full))
          {
            const auto subdir{to / iters.in1->relative};
            fs::create_directory(subdir);
            fs::copy(iters.in1->full, subdir, fs::copy_options::recursive);
          }
          else
          {
            fs::copy(iters.in1->full, to);
          }
        }

        if(iters.in2 != toEnd)
        {
          while((iters.in2 != toEnd) && compare{}(*iters.in2, *iters.in1))
          {
            fs::remove_all(iters.in2->full);
            ++iters.in2;
          }

          soft_update(from, to, iters.in1, fromEnd, iters.in2, toEnd);
        }
      }
      else if(iters.in2 != toEnd)
      {
        for(; iters.in2 != toEnd; ++iters.in2)
        {
          fs::remove_all(iters.in2->full);
        }
      }
    }
  }

  void soft_update(const fs::path& from, const fs::path& to)
  {
    throw_unless_exists(from);
    throw_unless_exists(to);

    copy_special_files(from, to);

    const std::vector<path_info>
      sortedFromEntries{sort_dir_entries(from)},
      sortedToEntries{sort_dir_entries(to)};

    soft_update(from, to, sortedFromEntries.begin(), sortedFromEntries.end(), sortedToEntries.begin(), sortedToEntries.end());
  }

}
