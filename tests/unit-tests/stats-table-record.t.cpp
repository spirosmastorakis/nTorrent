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
#include "stats-table-record.hpp"

#include <ndn-cxx/name.hpp>

namespace ndn {
namespace ntorrent {
namespace tests {

BOOST_AUTO_TEST_SUITE(TestStatsTableRecord)

BOOST_AUTO_TEST_CASE(TestCoreAPI)
{
  StatsTableRecord record(Name("isp1"));

  BOOST_CHECK_EQUAL(record.getRecordName().toUri(), "/isp1");
  BOOST_CHECK_EQUAL(record.getRecordSentInterests(), 0);
  BOOST_CHECK_EQUAL(record.getRecordReceivedData(), 0);
  BOOST_CHECK_EQUAL(record.getRecordSuccessRate(), 0);
}

BOOST_AUTO_TEST_CASE(TestIncrement)
{
  StatsTableRecord record(Name("isp1"));
  record.incrementSentInterests();

  BOOST_CHECK_EQUAL(record.getRecordName().toUri(), "/isp1");
  BOOST_CHECK_EQUAL(record.getRecordSentInterests(), 1);
  BOOST_CHECK_EQUAL(record.getRecordReceivedData(), 0);
  BOOST_CHECK_EQUAL(record.getRecordSuccessRate(), 0);

  record.incrementReceivedData();

  BOOST_CHECK_EQUAL(record.getRecordName().toUri(), "/isp1");
  BOOST_CHECK_EQUAL(record.getRecordSentInterests(), 1);
  BOOST_CHECK_EQUAL(record.getRecordReceivedData(), 1);
  BOOST_CHECK_EQUAL(record.getRecordSuccessRate(), 1);
}

BOOST_AUTO_TEST_CASE(TestIncrementThrow)
{
  StatsTableRecord record(Name("isp1"));
  BOOST_REQUIRE_THROW(record.incrementReceivedData(), StatsTableRecord::Error);

  BOOST_CHECK_EQUAL(record.getRecordName().toUri(), "/isp1");
  BOOST_CHECK_EQUAL(record.getRecordSentInterests(), 0);
  BOOST_CHECK_EQUAL(record.getRecordReceivedData(), 0);
  BOOST_CHECK_EQUAL(record.getRecordSuccessRate(), 0);
}

BOOST_AUTO_TEST_CASE(TestEqualityOperator)
{
  StatsTableRecord record1(Name("isp1"));
  record1.incrementSentInterests();
  record1.incrementReceivedData();

  StatsTableRecord record2(Name("isp1"));
  record2.incrementSentInterests();
  record2.incrementReceivedData();

  BOOST_CHECK(record1 == record2);
}

BOOST_AUTO_TEST_CASE(TestInequalityOperator)
{
  StatsTableRecord record1(Name("isp2"));
  record1.incrementSentInterests();
  record1.incrementReceivedData();

  StatsTableRecord record2(Name("isp1"));
  record2.incrementSentInterests();
  record2.incrementReceivedData();

  BOOST_CHECK(record1 != record2);

  StatsTableRecord record3(Name("isp1"));
  record3.incrementSentInterests();

  BOOST_CHECK(record2 != record3);
}

BOOST_AUTO_TEST_CASE(TestCopyConstructor)
{
  StatsTableRecord record1(Name("isp2"));
  record1.incrementSentInterests();
  record1.incrementReceivedData();

  StatsTableRecord record2(record1);

  BOOST_CHECK(record1 == record2);

  record2.incrementSentInterests();

  BOOST_CHECK(record1 != record2);

  record1.incrementSentInterests();

  BOOST_CHECK(record1 == record2);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace ntorrent
} // namespace ndn
