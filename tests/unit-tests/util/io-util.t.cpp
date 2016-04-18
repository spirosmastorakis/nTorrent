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

#include "../boost-test.hpp"
#include "util/io-util.hpp"

namespace ndn {
namespace ntorrent {
namespace tests {

BOOST_AUTO_TEST_SUITE(TestIoUtils)

BOOST_AUTO_TEST_CASE(TestFindType)
{
  BOOST_CHECK_EQUAL(IoUtil::findType(Name("NTORRENT/linux/torrent-file/sha256digest")), 0);

  Name n1("NTORRENT/linux/torrent-file");
  n1.appendSequenceNumber(0);
  n1 = Name(n1.toUri() + "/sha256digest");

  BOOST_CHECK_EQUAL(IoUtil::findType(n1), 0);

  Name n2("NTORRENT/linux/file0");
  n2.appendSequenceNumber(0);
  n2 = Name(n2.toUri() + "/sha256digest");

  BOOST_CHECK_EQUAL(IoUtil::findType(n2), 1);

  Name n3("NTORRENT/linux/file0");
  n3.appendSequenceNumber(0);
  n3.appendSequenceNumber(0);
  n3 = Name(n3.toUri() + "/sha256digest");

  BOOST_CHECK_EQUAL(IoUtil::findType(n3), 2);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nTorrent
} // namespace ndn
