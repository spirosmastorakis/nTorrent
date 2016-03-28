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

#include "boost-test.hpp"
#include "stats-table.hpp"

#include <ndn-cxx/name.hpp>

namespace ndn {
namespace ntorrent {
namespace tests {

BOOST_AUTO_TEST_SUITE(TestStatsTable)

BOOST_AUTO_TEST_CASE(TestCoreAPI)
{
  StatsTable table(Name("linux15.01"));
  table.insert(Name("isp1"));
  table.insert(Name("isp2"));
  table.insert(Name("isp3"));

  BOOST_CHECK(table.find(Name("isp1")) != table.end());
  BOOST_CHECK(table.find(Name("isp2")) != table.end());
  BOOST_CHECK(table.find(Name("isp3")) != table.end());
  BOOST_CHECK(table.find(Name("isp4")) == table.end());

  BOOST_CHECK_EQUAL(table.erase(Name("isp2")), true);
  BOOST_CHECK_EQUAL(table.erase(Name("isp4")), false);

  BOOST_CHECK(table.find(Name("isp1")) != table.end());
  BOOST_CHECK(table.find(Name("isp2")) == table.end());
  BOOST_CHECK(table.find(Name("isp3")) != table.end());
  BOOST_CHECK(table.find(Name("isp4")) == table.end());
}

BOOST_AUTO_TEST_CASE(TestStatsTableModification)
{
  StatsTable table(Name("linux15.01"));
  table.insert(Name("isp1"));
  table.insert(Name("isp2"));
  table.insert(Name("isp3"));

  BOOST_CHECK_EQUAL(table.size(), 3);

  auto entry = table.find(Name("isp2"));
  entry->incrementSentInterests();
  entry->incrementReceivedData();

  auto i = table.begin();
  BOOST_CHECK(!(entry == table.end()));
  BOOST_CHECK_EQUAL(i->getRecordName().toUri(), "/isp1");
  BOOST_CHECK_EQUAL(i->getRecordSuccessRate(), 0);

  i++;
  BOOST_CHECK_EQUAL(i->getRecordName().toUri(), "/isp2");
  BOOST_CHECK_EQUAL(i->getRecordSuccessRate(), 1);

  i++;
  BOOST_CHECK_EQUAL(i->getRecordName().toUri(), "/isp3");
  BOOST_CHECK_EQUAL(i->getRecordSuccessRate(), 0);

  entry = table.find(Name("isp1"));
  BOOST_CHECK(!(entry == table.end()));
  entry->incrementSentInterests();
  entry->incrementReceivedData();
  entry->incrementSentInterests();

  i = table.begin();
  BOOST_CHECK_EQUAL(i->getRecordName().toUri(), "/isp1");
  BOOST_CHECK_EQUAL(i->getRecordSuccessRate(), 0.5);

  i++;
  BOOST_CHECK_EQUAL(i->getRecordName().toUri(), "/isp2");
  BOOST_CHECK_EQUAL(i->getRecordSuccessRate(), 1);

  i++;
  BOOST_CHECK_EQUAL(i->getRecordName().toUri(), "/isp3");
  BOOST_CHECK_EQUAL(i->getRecordSuccessRate(), 0);

  entry = table.find(Name("isp3"));
  BOOST_CHECK(!(entry == table.end()));
  entry->incrementSentInterests();
  entry->incrementReceivedData();
  entry->incrementSentInterests();
  entry->incrementSentInterests();
  entry->incrementSentInterests();

  i = table.begin();
  BOOST_CHECK_EQUAL(i->getRecordName().toUri(), "/isp1");
  BOOST_CHECK_EQUAL(i->getRecordSuccessRate(), 0.5);

  i++;
  BOOST_CHECK_EQUAL(i->getRecordName().toUri(), "/isp2");
  BOOST_CHECK_EQUAL(i->getRecordSuccessRate(), 1);

  i++;
  BOOST_CHECK_EQUAL(i->getRecordName().toUri(), "/isp3");
  BOOST_CHECK_EQUAL(i->getRecordSuccessRate(), 0.25);
}

BOOST_AUTO_TEST_CASE(TestTableSort)
{
  StatsTable table(Name("linux15.01"));
  table.insert(Name("isp1"));
  table.insert(Name("isp2"));
  table.insert(Name("isp3"));

  BOOST_CHECK_EQUAL(table.size(), 3);

  auto entry = table.find(Name("isp2"));
  entry->incrementSentInterests();
  entry->incrementReceivedData();

  auto i = table.begin();
  BOOST_CHECK(!(entry == table.end()));
  BOOST_CHECK_EQUAL(i->getRecordName().toUri(), "/isp1");
  BOOST_CHECK_EQUAL(i->getRecordSuccessRate(), 0);

  i++;
  BOOST_CHECK_EQUAL(i->getRecordName().toUri(), "/isp2");
  BOOST_CHECK_EQUAL(i->getRecordSuccessRate(), 1);

  i++;
  BOOST_CHECK_EQUAL(i->getRecordName().toUri(), "/isp3");
  BOOST_CHECK_EQUAL(i->getRecordSuccessRate(), 0);

  entry = table.find(Name("isp1"));
  BOOST_CHECK(!(entry == table.end()));
  entry->incrementSentInterests();
  entry->incrementReceivedData();
  entry->incrementSentInterests();

  i = table.begin();
  BOOST_CHECK_EQUAL(i->getRecordName().toUri(), "/isp1");
  BOOST_CHECK_EQUAL(i->getRecordSuccessRate(), 0.5);

  i++;
  BOOST_CHECK_EQUAL(i->getRecordName().toUri(), "/isp2");
  BOOST_CHECK_EQUAL(i->getRecordSuccessRate(), 1);

  i++;
  BOOST_CHECK_EQUAL(i->getRecordName().toUri(), "/isp3");
  BOOST_CHECK_EQUAL(i->getRecordSuccessRate(), 0);

  entry = table.find(Name("isp3"));
  BOOST_CHECK(!(entry == table.end()));
  entry->incrementSentInterests();
  entry->incrementReceivedData();
  entry->incrementSentInterests();
  entry->incrementSentInterests();
  entry->incrementSentInterests();

  i = table.begin();
  BOOST_CHECK_EQUAL(i->getRecordName().toUri(), "/isp1");
  BOOST_CHECK_EQUAL(i->getRecordSuccessRate(), 0.5);

  i++;
  BOOST_CHECK_EQUAL(i->getRecordName().toUri(), "/isp2");
  BOOST_CHECK_EQUAL(i->getRecordSuccessRate(), 1);

  i++;
  BOOST_CHECK_EQUAL(i->getRecordName().toUri(), "/isp3");
  BOOST_CHECK_EQUAL(i->getRecordSuccessRate(), 0.25);

  table.sort();

  i = table.begin();
  BOOST_CHECK_EQUAL(i->getRecordName().toUri(), "/isp2");
  BOOST_CHECK_EQUAL(i->getRecordSentInterests(), 1);
  BOOST_CHECK_EQUAL(i->getRecordSuccessRate(), 1);

  i++;
  BOOST_CHECK_EQUAL(i->getRecordName().toUri(), "/isp1");
  BOOST_CHECK_EQUAL(i->getRecordSuccessRate(), 0.5);

  i++;
  BOOST_CHECK_EQUAL(i->getRecordName().toUri(), "/isp3");
  BOOST_CHECK_EQUAL(i->getRecordSuccessRate(), 0.25);

  BOOST_CHECK_EQUAL(table.erase(Name("isp1")), true);
  BOOST_CHECK_EQUAL(table.size(), 2);

  i = table.begin();
  BOOST_CHECK_EQUAL(i->getRecordName().toUri(), "/isp2");
  BOOST_CHECK_EQUAL(i->getRecordSuccessRate(), 1);

  i++;
  BOOST_CHECK_EQUAL(i->getRecordName().toUri(), "/isp3");
  BOOST_CHECK_EQUAL(i->getRecordSuccessRate(), 0.25);

  entry = table.find(Name("isp2"));
  BOOST_CHECK(!(entry == table.end()));
  entry->incrementSentInterests();
  entry->incrementSentInterests();
  entry->incrementSentInterests();

  table.sort();

  i = table.begin();
  BOOST_CHECK_EQUAL(i->getRecordName().toUri(), "/isp3");
  BOOST_CHECK_EQUAL(i->getRecordSuccessRate(), 0.25);

  i++;
  BOOST_CHECK_EQUAL(i->getRecordName().toUri(), "/isp2");
  BOOST_CHECK_EQUAL(i->getRecordSuccessRate(), 0.25);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace ntorrent
} // namespace ndn
