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

#include <boost/throw_exception.hpp>

namespace ndn {
namespace ntorrent {

StatsTableRecord::StatsTableRecord(const Name& recordName)
  : m_recordName(recordName)
  , m_sentInterests(0)
  , m_receivedData(0)
  , m_successRate(0)
{
}

StatsTableRecord::StatsTableRecord(const StatsTableRecord& record)
  : m_recordName(record.getRecordName())
  , m_sentInterests(record.getRecordSentInterests())
  , m_receivedData(record.getRecordReceivedData())
  , m_successRate(record.getRecordSuccessRate())
{
}

void
StatsTableRecord::incrementSentInterests()
{
  ++m_sentInterests;
  m_successRate = m_receivedData / float(m_sentInterests);
}

void
StatsTableRecord::incrementReceivedData()
{
  if (m_sentInterests == 0) {
    BOOST_THROW_EXCEPTION(Error("Computing success rate while having sent no Interests"));
  }
  ++m_receivedData;
  m_successRate = m_receivedData / float(m_sentInterests);
}

StatsTableRecord&
StatsTableRecord::operator=(const StatsTableRecord& other)
{
  m_recordName = other.getRecordName();
  m_sentInterests = other.getRecordSentInterests();
  m_receivedData = other.getRecordReceivedData();
  m_successRate = other.getRecordSuccessRate();
  return (*this);
}

bool
operator==(const StatsTableRecord& lhs, const StatsTableRecord& rhs)
{
  return (lhs.getRecordName() == rhs.getRecordName() &&
  lhs.getRecordSentInterests() == rhs.getRecordSentInterests() &&
  lhs.getRecordReceivedData() == rhs.getRecordReceivedData());
}

bool
operator!=(const StatsTableRecord& lhs, const StatsTableRecord& rhs)
{
  return (lhs.getRecordName() != rhs.getRecordName() ||
  lhs.getRecordSentInterests() != rhs.getRecordSentInterests() ||
  lhs.getRecordReceivedData() != rhs.getRecordReceivedData());
}

}  // namespace ntorrent
}  // namespace ndn
