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
#include "update-handler.hpp"
#include "unit-test-time-fixture.hpp"
#include "dummy-parser-fixture.hpp"

#include <ndn-cxx/name.hpp>
#include <ndn-cxx/util/dummy-client-face.hpp>
#include <ndn-cxx/util/io.hpp>

namespace ndn {
namespace ntorrent {
namespace tests {

using util::DummyClientFace;
using std::vector;

class TestUpdateHandler : public UpdateHandler {
public:
  TestUpdateHandler(Name torrentName, shared_ptr<KeyChain> keyChain,
                    shared_ptr<StatsTable> statsTable, shared_ptr<Face> face)
  : UpdateHandler(torrentName, keyChain, statsTable, face)
  {
  }

  ~TestUpdateHandler()
  {
  }

  Name
  getOwnRoutablePrefix()
  {
    return UpdateHandler::getOwnRoutablePrefix();
  }
};

class FaceFixture : public UnitTestTimeFixture
{
public:
  explicit
  FaceFixture()
  : face1(new DummyClientFace(io, {true, true}))
  , face2(new DummyClientFace(io, {true, true}))
  {
  }

public:
  shared_ptr<DummyClientFace> face1;
  shared_ptr<DummyClientFace> face2;
};

BOOST_FIXTURE_TEST_SUITE(TestUpdateHandlerClass, FaceFixture)

BOOST_AUTO_TEST_CASE(TestInitialSetup)
{
  shared_ptr<StatsTable> table1 = make_shared<StatsTable>(Name("linux15.01"));

  shared_ptr<KeyChain> keyChain = make_shared<KeyChain>();

  TestUpdateHandler handler1(Name("linux15.01"), keyChain, table1, face1);
  advanceClocks(time::milliseconds(1), 10);
  // Create a data packet containing one name as content
  shared_ptr<Data> d = DummyParser::createDataPacket(Name("/localhop/nfd/rib/routable-prefixes"),
                                                      { Name("ucla") });
  keyChain->sign(*d);
  face1->receive(*d);

  BOOST_CHECK_EQUAL(handler1.getOwnRoutablePrefix().toUri(), "/ucla");

  shared_ptr<StatsTable> table2 = make_shared<StatsTable>(Name("linux15.01"));
  TestUpdateHandler handler2(Name("linux15.01"), keyChain, table2, face2);
  advanceClocks(time::milliseconds(1), 10);
  // Create a data packet containing one name as content
  d = DummyParser::createDataPacket(Name("/localhop/nfd/rib/routable-prefixes"),
                                     { Name("arizona") });
  keyChain->sign(*d);
  face2->receive(*d);

  BOOST_CHECK_EQUAL(handler2.getOwnRoutablePrefix().toUri(), "/arizona");
}

BOOST_AUTO_TEST_CASE(TestAliveInterestExchange)
{
  shared_ptr<StatsTable> table1 = make_shared<StatsTable>(Name("linux15.01"));
  table1->insert(Name("isp1"));
  table1->insert(Name("isp2"));
  table1->insert(Name("isp3"));

  shared_ptr<KeyChain> keyChain = make_shared<KeyChain>();

  TestUpdateHandler handler1(Name("linux15.01"), keyChain, table1, face1);
  advanceClocks(time::milliseconds(1), 10);
  // Create a data packet containing one name as content
  shared_ptr<Data> d = DummyParser::createDataPacket(Name("/localhop/nfd/rib/routable-prefixes"),
                                                      { Name("ucla") });
  keyChain->sign(*d);
  face1->receive(*d);

  shared_ptr<StatsTable> table2 = make_shared<StatsTable>(Name("linux15.01"));
  table2->insert(Name("ucla"));
  TestUpdateHandler handler2(Name("linux15.01"), keyChain, table2, face2);
  advanceClocks(time::milliseconds(1), 10);
  // Create a data packet containing one name as content
  d = DummyParser::createDataPacket(Name("/localhop/nfd/rib/routable-prefixes"),
                                     { Name("arizona") });
  keyChain->sign(*d);
  face2->receive(*d);

  handler2.sendAliveInterest(table2->begin());

  advanceClocks(time::milliseconds(1), 40);
  Interest interest(Name("/NTORRENT/linux15.01/ALIVE/test"));
  face1->receive(interest);

  advanceClocks(time::milliseconds(1), 10);
  std::vector<Data> dataVec = face1->sentData;

  BOOST_CHECK_EQUAL(dataVec.size(), 1);
  BOOST_CHECK_EQUAL(dataVec[0].getName().toUri(), "/NTORRENT/linux15.01/ALIVE/test");

  auto block = dataVec[0].getContent();
  shared_ptr<vector<Name>> nameVec = DummyParser::decodeContent(block);
  auto it = nameVec->begin();

  BOOST_CHECK_EQUAL(it->toUri(), "/test");
  ++it;
  BOOST_CHECK_EQUAL(it->toUri(), "/isp3");
  ++it;
  BOOST_CHECK_EQUAL(it->toUri(), "/isp2");
  ++it;
  BOOST_CHECK_EQUAL(it->toUri(), "/isp1");

  BOOST_CHECK_EQUAL((table1->end() - 1)->getRecordName().toUri(), "/test");

  d = DummyParser::createDataPacket(Name("/NTORRENT/linux15.01/ALIVE/arizona"),
                                     { Name("isp1"), Name("isp2"), Name("isp3") });
  keyChain->sign(*d);

  advanceClocks(time::milliseconds(1), 40);
  face2->receive(*d);

  auto i = table2->begin();
  ++i;
  BOOST_CHECK_EQUAL(i->getRecordName().toUri(), "/isp1");
  ++i;
  BOOST_CHECK_EQUAL(i->getRecordName().toUri(), "/isp2");
  ++i;
  BOOST_CHECK_EQUAL(i->getRecordName().toUri(), "/isp3");

  table1->erase(Name("isp1"));
  table1->erase(Name("isp2"));

  table1->insert(Name("isp4"));
  table1->insert(Name("isp5"));
  table1->insert(Name("isp6"));
  table1->insert(Name("isp7"));
  table1->insert(Name("isp8"));
  table1->insert(Name("isp9"));

  handler2.sendAliveInterest(table2->begin());

  advanceClocks(time::milliseconds(1), 40);
  Interest interest2(Name("/NTORRENT/linux15.01/ALIVE/arizona"));
  face1->receive(interest2);

  advanceClocks(time::milliseconds(1), 10);

  dataVec = face1->sentData;
  BOOST_CHECK_EQUAL(dataVec.size(), 2);

  auto iter = dataVec.begin() + 1;
  advanceClocks(time::milliseconds(1), 30);
  face2->receive(*iter);

  i = table2->begin();
  BOOST_CHECK_EQUAL(i->getRecordName().toUri(), "/ucla");
  BOOST_CHECK_EQUAL(i->getRecordSuccessRate(), 0);
  BOOST_CHECK_EQUAL(i->getRecordSentInterests(), 0);
  BOOST_CHECK_EQUAL(i->getRecordReceivedData(), 0);
  ++i;

  BOOST_CHECK_EQUAL(i->getRecordName().toUri(), "/isp1");
  BOOST_CHECK_EQUAL(i->getRecordSuccessRate(), 0);
  BOOST_CHECK_EQUAL(i->getRecordSentInterests(), 0);
  BOOST_CHECK_EQUAL(i->getRecordReceivedData(), 0);
  ++i;

  BOOST_CHECK_EQUAL(i->getRecordName().toUri(), "/isp2");
  BOOST_CHECK_EQUAL(i->getRecordSuccessRate(), 0);
  BOOST_CHECK_EQUAL(i->getRecordSentInterests(), 0);
  BOOST_CHECK_EQUAL(i->getRecordReceivedData(), 0);
  ++i;

  BOOST_CHECK_EQUAL(i->getRecordName().toUri(), "/isp3");
  BOOST_CHECK_EQUAL(i->getRecordSuccessRate(), 0);
  BOOST_CHECK_EQUAL(i->getRecordSentInterests(), 0);
  BOOST_CHECK_EQUAL(i->getRecordReceivedData(), 0);
  ++i;

  BOOST_CHECK_EQUAL(i->getRecordName().toUri(), "/test");
  BOOST_CHECK_EQUAL(i->getRecordSuccessRate(), 0);
  BOOST_CHECK_EQUAL(i->getRecordSentInterests(), 0);
  BOOST_CHECK_EQUAL(i->getRecordReceivedData(), 0);
  ++i;

  BOOST_CHECK_EQUAL(i->getRecordName().toUri(), "/isp4");
  BOOST_CHECK_EQUAL(i->getRecordSuccessRate(), 0);
  BOOST_CHECK_EQUAL(i->getRecordSentInterests(), 0);
  BOOST_CHECK_EQUAL(i->getRecordReceivedData(), 0);
  ++i;

  BOOST_CHECK_EQUAL(i->getRecordName().toUri(), "/isp5");
  BOOST_CHECK_EQUAL(i->getRecordSuccessRate(), 0);
  BOOST_CHECK_EQUAL(i->getRecordSentInterests(), 0);
  BOOST_CHECK_EQUAL(i->getRecordReceivedData(), 0);
  ++i;

  BOOST_CHECK_EQUAL(i->getRecordName().toUri(), "/isp6");
  BOOST_CHECK_EQUAL(i->getRecordSuccessRate(), 0);
  BOOST_CHECK_EQUAL(i->getRecordSentInterests(), 0);
  BOOST_CHECK_EQUAL(i->getRecordReceivedData(), 0);
  ++i;

  BOOST_CHECK(i == table2->end());
}

BOOST_AUTO_TEST_CASE(TestNeedsUpdate)
{
  shared_ptr<StatsTable> table1 = make_shared<StatsTable>(Name("linux15.01"));
  table1->insert(Name("isp1"));
  table1->insert(Name("isp2"));
  table1->insert(Name("isp3"));

  shared_ptr<KeyChain> keyChain = make_shared<KeyChain>();

  UpdateHandler handler1(Name("linux15.01"), keyChain, table1, face1);

  BOOST_CHECK(handler1.needsUpdate());

  auto i = table1->begin() + 1;
  i->incrementSentInterests();
  i->incrementSentInterests();
  i->incrementReceivedData();

  BOOST_CHECK(handler1.needsUpdate());

  table1->insert(Name("isp4"));
  table1->insert(Name("isp5"));

  BOOST_CHECK(!handler1.needsUpdate());
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace ntorrent
} // namespace ndn
