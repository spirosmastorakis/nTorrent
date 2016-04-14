/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
* Copyright (c) 2016 Regents of the University of California.
*
* This file is part of the nTorrent codebase.
*
* nTorrent is free software: you can redistribute it and/or modify it under the
* terms of the GNU Lesser General Public License as published by the Free Software
* Foundation, either version 3 of the License, or (at your option) any later version.
*
* nTorrent is distributed in the hope that it will be useful, but WITHOUT ANY
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
* PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
*
* You should have received copies of the GNU General Public License and GNU Lesser
* General Public License along with nTorrent, e.g., in COPYING.md file. If not, see
* <http://www.gnu.org/licenses/>.
*
* See AUTHORS for complete list of nTorrent authors and contributors.
*/

#include "stats-table-record.hpp"

#include <vector>

namespace ndn {
namespace ntorrent {

/**
 * @brief Represents a stats table
 */
class StatsTable {
public:
  /**
   * @brief Create an empty stats table
   */
  StatsTable() = default;

  /**
   * @brief Create a stats table for a specific torrent
   * @param torrentName The name of the torrent
   */
  StatsTable(const Name& torrentName);

  ~StatsTable() = default;

  /**
   * @brief Insert a routable prefix to the stats table
   * @param prefix The prefix to be inserted
   */
  void
  insert(const Name& prefix);

  /**
   * @brief Erase a prefix from the statsTable
   * @param prefix The prefix to be erased
   * @return True if the prefix was found and erased. Otherwise, false
   */
  bool
  erase(const Name& prefix);

  /**
   * @brief Clear the stats table
   */
  void
  clear();

  /**
   * @brief Return the size of the stats table (number of prefixes included)
   */
  size_t
  size() const;

  typedef std::vector<StatsTableRecord>::const_iterator const_iterator;
  typedef std::vector<StatsTableRecord>::iterator iterator;

  /**
   * @brief Constant iterator to the beginning of the stats table
   */
  const_iterator
  begin() const;

  /**
   * @brief Iterator to the beginning of the stats table
   */
  iterator
  begin();

  /**
   * @brief Constant iterator to the end of the stats table
   */
  const_iterator
  end() const;

  /**
   * @brief Iterator to the end of the stats table
   */
  iterator
  end();

  /**
   * @brief Find a prefix on the stats table
   * @param prefix The name prefix to be searched
   * @return A constant iterator to the prefix if found. Otherwise, return StatsTable::end()
   */
  const_iterator
  find(const Name& prefix) const;

  /**
   * @brief Find a prefix on the stats table
   * @param prefix The name prefix to be searched
   * @return An iterator to the prefix if found. Otherwise, return StatsTable::end()
   */
  iterator
  find(const Name& prefix);

  /**
   * @brief Comparator used for sorting the records of the stats table
   */
  struct comparator {
    bool operator() (const StatsTableRecord& left, const StatsTableRecord& right) const
    {return left.getRecordSuccessRate() >= right.getRecordSuccessRate();}
  };

  /**
   * @brief Sort the records of the stats table on desceding success rate
   * @param comp Optional comparator function to be used for sorting.
   *             The default value is the provided comparator struct
   *
   * This method has to be called manually by the application to sort the
   * stats table.
   */
  void
  sort(std::function<bool(const StatsTableRecord&, const StatsTableRecord&)> comp = comparator());

private:
  // Set of StatsTableRecords
  std::vector<StatsTableRecord> m_statsTable;
  Name m_torrentName;
};

inline void
StatsTable::clear()
{
  m_statsTable.clear();
}

inline size_t
StatsTable::size() const
{
  return m_statsTable.size();
}

inline StatsTable::const_iterator
StatsTable::begin() const
{
  return m_statsTable.begin();
}

inline StatsTable::iterator
StatsTable::begin()
{
  return m_statsTable.begin();
}

inline StatsTable::const_iterator
StatsTable::end() const
{
  return m_statsTable.end();
}

inline StatsTable::iterator
StatsTable::end()
{
  return m_statsTable.end();
}

inline void
StatsTable::sort(std::function<bool(const StatsTableRecord&, const StatsTableRecord&)> comp)
{
  std::sort(m_statsTable.begin(), m_statsTable.end(), comp);
}

}  // namespace ntorrent
}  // namespace ndn
