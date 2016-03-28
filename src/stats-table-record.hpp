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

#include <ndn-cxx/name.hpp>

namespace ndn {
namespace ntorrent {

/**
 * @brief Represents a record of the stats table
 */
class StatsTableRecord {
public:
  class Error : public std::runtime_error
  {
  public:
    explicit
    Error(const std::string& what)
      : std::runtime_error(what)
    {
    }
  };

  /**
   * @brief Create a new empty record
   */
  StatsTableRecord() = default;

  /**
   * @brief Create a new record
   * @param recordName The name of this record
   */
  StatsTableRecord(const Name& recordName);

  /**
   * @brief Copy constructor
   * @param record An StatsTableRecord object
   */
  StatsTableRecord(const StatsTableRecord& record);

  ~StatsTableRecord() = default;

  /**
   * @brief Get the name of a record
   */
  const Name&
  getRecordName() const;

  /**
   * @brief Get the number of sent interests of a record
   */
  uint64_t
  getRecordSentInterests() const;

  /**
   * @brief Get the number of received data packets of a record
   */
  uint64_t
  getRecordReceivedData() const;

  /**
   * @brief Get the success rate of a record
   */
  double
  getRecordSuccessRate() const;

  /**
   * @brief Increment the number of sent interests for a record
   */
  void
  incrementSentInterests();

  /**
   * @brief Increment the number of received data packets for a record
   */
  void
  incrementReceivedData();

  /**
   * @brief Assignment operator
   */
  StatsTableRecord&
  operator=(const StatsTableRecord& other);

private:
  Name m_recordName;
  uint64_t m_sentInterests;
  uint64_t m_receivedData;
  double m_successRate;
};

/**
 * @brief Equality operator
 */
bool
operator==(const StatsTableRecord& lhs, const StatsTableRecord& rhs);

/**
 * @brief Inequality operator
 */
bool
operator!=(const StatsTableRecord& lhs, const StatsTableRecord& rhs);

inline const Name&
StatsTableRecord::getRecordName() const
{
  return m_recordName;
}

inline uint64_t
StatsTableRecord::getRecordSentInterests() const
{
  return m_sentInterests;
}

inline uint64_t
StatsTableRecord::getRecordReceivedData() const
{
  return m_receivedData;
}

inline double
StatsTableRecord::getRecordSuccessRate() const
{
  return m_successRate;
}


}  // namespace ntorrent
}  // namespace ndn
