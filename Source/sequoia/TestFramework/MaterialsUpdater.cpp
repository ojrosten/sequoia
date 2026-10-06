////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/MaterialsUpdater.hpp"

#include "sequoia/TestFramework/CoreInfrastructure.hpp"
#include "sequoia/TestFramework/FileEditors.hpp"
#include "sequoia/TestFramework/FileSystemUtilities.hpp"
#include "sequoia/TestFramework/ProjectPaths.hpp"

#include <algorithm>
#include <compare>
#include <format>
#include <ranges>
#include <tuple>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    void copy_special_files_back(const fs::path& from, const fs::path& to)
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

    struct path_info
    {
      fs::path full{}, relative{};
      fs::file_type file_type{};
    };

    [[nodiscard]]
    std::strong_ordering compare(const path_info& lhs, const path_info& rhs)
    {
      return std::tie(lhs.relative, lhs.file_type) <=> std::tie(rhs.relative, rhs.file_type);
    }

    [[nodiscard]]
    std::vector<path_info> sort_dir_entries(const fs::path& dir)
    {
      auto toPathInfo{
        [&dir](const fs::directory_entry& entry) {
          return path_info{entry.path(), fs::relative(entry.path(), dir), entry.status().type()};
        }
      };

      auto entries{
          fs::directory_iterator(dir)
        | std::views::transform(toPathInfo)
        | std::ranges::to<std::vector>()
      };

      std::ranges::sort(entries, [](const path_info& lhs, const path_info& rhs) { return compare(lhs, rhs) < 0; });
      return entries;
    }

    void update_directory(const fs::path& from, const fs::path& to, std::vector<fs::path>& deleted);

    void add_entry(const path_info& fromEntry, const fs::path& to)
    {
      if(fromEntry.file_type == fs::file_type::directory)
      {
        const auto subdir{to / fromEntry.relative};
        fs::create_directory(subdir);
        fs::copy(fromEntry.full, subdir, fs::copy_options::recursive);
      }
      else
      {
        fs::copy(fromEntry.full, to);
      }
    }

    void delete_entry(const path_info& toEntry, std::vector<fs::path>& deleted)
    {
      // Recorded before the deletion, so that a deletion which throws is still reported
      deleted.push_back(toEntry.full);
      fs::remove_all(toEntry.full);
    }

    void update_entry(const path_info& fromEntry, const path_info& toEntry, std::vector<fs::path>& deleted)
    {
      switch(fromEntry.file_type)
      {
      case fs::file_type::regular:
      {
        const auto [rFrom, rTo] {get_reduced_file_content(fromEntry.full, toEntry.full)};
        if(rFrom && rTo && (rFrom.value() != rTo.value()))
        {
          fs::copy_file(fromEntry.full, toEntry.full, fs::copy_options::overwrite_existing);
        }
        break;
      }
      case fs::file_type::directory:
        update_directory(fromEntry.full, toEntry.full, deleted);
        break;
      default:
        throw std::runtime_error{
          std::format("'{}' is of type '{}': only regular files and directories can be updated",
                      fromEntry.full.generic_string(),
                      serializer<fs::file_type>::make(fromEntry.file_type))
        };
      }
    }

    /** \brief Makes the directory `to`, whose entries are `toEntries`, mirror `fromEntries`.

        Entries are matched by name and type:
        -# An entry only in `fromEntries` is added.
        -# An entry only in `toEntries` is deleted.
        -# An entry in both is updated.

        \pre Both lists are sorted by `compare`.
     */
    void update_entries(const fs::path& to,
                        std::span<const path_info> fromEntries,
                        std::span<const path_info> toEntries,
                        std::vector<fs::path>& deleted)
    {
      while(!fromEntries.empty() && !toEntries.empty())
      {
        const auto& fromEntry{fromEntries.front()};
        const auto& toEntry  {toEntries.front()};

        const auto ordering{compare(fromEntry, toEntry)};

        if(ordering < 0)
        {
          add_entry(fromEntry, to);
          fromEntries = fromEntries.subspan(1);
        }
        else if(ordering > 0)
        {
          delete_entry(toEntry, deleted);
          toEntries = toEntries.subspan(1);
        }
        else
        {
          update_entry(fromEntry, toEntry, deleted);
          fromEntries = fromEntries.subspan(1);
          toEntries   = toEntries.subspan(1);
        }
      }

      for(const auto& fromEntry : fromEntries)
      {
        add_entry(fromEntry, to);
      }

      for(const auto& toEntry : toEntries)
      {
        delete_entry(toEntry, deleted);
      }
    }

    void update_directory(const fs::path& from, const fs::path& to, std::vector<fs::path>& deleted)
    {
      copy_special_files_back(from, to);
      update_entries(to, sort_dir_entries(from), sort_dir_entries(to), deleted);
    }
  }

  incomplete_update::incomplete_update(const std::string& message, std::vector<fs::path> deleted)
    : std::runtime_error{message}
    , m_Deleted{std::make_shared<const std::vector<fs::path>>(std::move(deleted))}
  {}

  [[nodiscard]]
  std::vector<fs::path> soft_update(const fs::path& from, const fs::path& to)
  {
    throw_unless_directory(from);
    throw_unless_directory(to);

    std::vector<fs::path> deleted{};
    try
    {
      update_directory(from, to, deleted);
    }
    catch(const std::exception& e)
    {
      throw incomplete_update{e.what(), std::move(deleted)};
    }

    return deleted;
  }
}
