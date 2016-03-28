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

#include "stats-table.hpp"

namespace ndn {
namespace ntorrent {

StatsTable::StatsTable(const Name& torrentName)
  : m_torrentName(torrentName)
{
}

void
StatsTable::insert(const Name& prefix)
{
  m_statsTable.push_back(StatsTableRecord(prefix));
}

StatsTable::const_iterator
StatsTable::find(const Name& prefix) const
{
  for (auto i = m_statsTable.begin(); i != m_statsTable.end(); ++i) {
    if (i->getRecordName() == prefix) {
      return i;
    }
  }
  return StatsTable::end();
}

StatsTable::iterator
StatsTable::find(const Name& prefix)
{
  for (auto i = m_statsTable.begin(); i != m_statsTable.end(); ++i) {
    if (i->getRecordName() == prefix) {
      return i;
    }
  }
  return StatsTable::end();
}

bool
StatsTable::erase(const Name& prefix)
{
  for (auto i = m_statsTable.begin(); i != m_statsTable.end(); ++i) {
    if (i->getRecordName() == prefix) {
        m_statsTable.erase(i);
        return true;
    }
  }
  return false;
}

}  // namespace ntorrent
}  // namespace ndn
