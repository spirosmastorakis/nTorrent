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

#include "torrent-file.hpp"

#include <ndn-cxx/data.hpp>
#include <ndn-cxx/signature.hpp>
#include <ndn-cxx/security/key-chain.hpp>

namespace ndn {

namespace ntorrent {

namespace tests {


const static uint8_t TorrentFileTest[] = {
  0x6, 0xfd,
  0x1, 0xe4,
     0x7, 0x25,
        0x8, 0x8,
           0x4e, 0x54, 0x4f, 0x52, 0x52, 0x45, 0x4e, 0x54,
        0x8, 0xa,
           0x6c, 0x69, 0x6e, 0x75, 0x78, 0x31, 0x35, 0x2e, 0x30, 0x31,
        0x8, 0xd,
          0x2e, 0x74, 0x6f, 0x72, 0x72, 0x65, 0x6e, 0x74, 0x2d, 0x66, 0x69, 0x6c, 0x65,
     0x14, 0x0,
     0x15, 0x77,
        0x7, 0x37,
           0x8, 0x8,
              0x4e, 0x54, 0x4f, 0x52, 0x52, 0x45, 0x4e, 0x54,
           0x8, 0xa,
              0x6c, 0x69, 0x6e, 0x75, 0x78, 0x31, 0x35, 0x2e, 0x30, 0x31,
           0x8, 0xd,
              0x2e, 0x74, 0x6f, 0x72, 0x72, 0x65, 0x6e, 0x74, 0x2d, 0x66, 0x69, 0x6c, 0x65,
           0x8, 0x8,
              0x73, 0x65, 0x67, 0x6d, 0x65, 0x6e, 0x74, 0x32,
           0x8, 0x6,
              0x41, 0x45, 0x33, 0x32, 0x31, 0x43,
        0x7, 0x16,
           0x8, 0x8,
              0x4e, 0x54, 0x4f, 0x52, 0x52, 0x45, 0x4e, 0x54,
           0x8, 0xa,
              0x6c, 0x69, 0x6e, 0x75, 0x78, 0x31, 0x35, 0x2e, 0x30, 0x31,
        0x7, 0x11,
           0x8, 0x5,
              0x66, 0x69, 0x6c, 0x65, 0x30,
           0x8, 0x8,
              0x31, 0x41, 0x32, 0x42, 0x33, 0x43, 0x34, 0x44,
        0x7, 0x11,
           0x8, 0x5,
              0x66, 0x69, 0x6c, 0x65, 0x31,
           0x8, 0x8,
              0x32, 0x41, 0x33, 0x42, 0x34, 0x43, 0x35, 0x45,
     0x16, 0x3c,
        0x1b, 0x1,
           0x1,
        0x1c, 0x37,
            0x7, 0x35,
               0x8, 0xc,
                  0x74, 0x6d, 0x70, 0x2d, 0x69, 0x64, 0x65, 0x6e, 0x74, 0x69, 0x74, 0x79,
               0x8, 0x4,
                  0x3d, 0xa8, 0x2c, 0x19,
               0x8, 0x3,
                  0x4b, 0x45, 0x59,
               0x8, 0x11,
                  0x6b, 0x73, 0x6b, 0x2d, 0x31, 0x34, 0x35, 0x34, 0x39, 0x37, 0x33, 0x38, 0x37, 0x36, 0x31, 0x37, 0x31,
               0x8, 0x7,
                  0x49, 0x44, 0x2d, 0x43, 0x45, 0x52, 0x54,
      0x17, 0xfd,
         0x1, 0x0, 0x37, 0x61, 0xc9, 0x3b, 0x43, 0xe8, 0xa3, 0xcd, 0xf0, 0xb1,
         0x14, 0x78, 0x50, 0x6, 0x3c, 0x8a, 0x7, 0x4d, 0xe4, 0xa8, 0xbe, 0xc1,
         0x8c, 0x40, 0xaf, 0xaa, 0x21, 0x9f, 0x58, 0xed, 0xc0, 0x99, 0x92, 0xb8,
         0xb2, 0xb8, 0xac, 0x4, 0x3f, 0xa2, 0x25, 0xd7, 0x68, 0xa9, 0x5e, 0xc6,
         0xf5, 0x20, 0x69, 0xe3, 0xf, 0x37, 0xfc, 0xc7, 0x3f, 0x72, 0x8, 0x74,
         0x8, 0x5e, 0x10, 0x4d, 0xef, 0xa3, 0x58, 0x59, 0xb6, 0x90, 0x53, 0x1c,
         0x4b, 0x22, 0xb0, 0x3e, 0x87, 0x95, 0x5b, 0xa6, 0xd, 0x84, 0x1f, 0x60,
         0xda, 0xe, 0x22, 0x29, 0xda, 0x38, 0x1c, 0x90, 0x4a, 0x2a, 0x60, 0xe4,
         0xc9, 0x36, 0xb6, 0x4f, 0x9a, 0x17, 0x69, 0x99, 0xf4, 0x84, 0xa5, 0x5b,
         0xec, 0x70, 0x34, 0x1b, 0xbc, 0x14, 0xee, 0xb1, 0xa7, 0x42, 0xb7, 0xd8,
         0x9, 0x45, 0xe0, 0x2d, 0x4, 0x8f, 0x66, 0xdb, 0x43, 0x68, 0x8e, 0x53,
         0x94, 0x1e, 0xcd, 0xaa, 0x47, 0x80, 0xaf, 0xd0, 0x1d, 0x6b, 0x21, 0x1,
         0x5a, 0x36, 0x97, 0x87, 0x44, 0xc9, 0x31, 0x57, 0xcd, 0x5d, 0xc1, 0xa5,
         0xfc, 0x35, 0xb1, 0x60, 0x29, 0xc6, 0x82, 0xe9, 0x52, 0x33, 0xac, 0xf9,
         0xb, 0xca, 0xbe, 0x62, 0x8b, 0x88, 0x49, 0x9a, 0x6f, 0xbd, 0xed, 0x1e,
         0x8c, 0x4c, 0x63, 0xfb, 0xd9, 0x20, 0xcb, 0x1c, 0x66, 0xa4, 0xb9, 0xc,
         0x21, 0x61, 0xc4, 0x56, 0x4c, 0xa8, 0x58, 0x4b, 0x7a, 0x81, 0x2c, 0xf0,
         0x2f, 0xce, 0x7d, 0x1d, 0xbf, 0x16, 0xc, 0x74, 0x35, 0x83, 0x8a, 0x22,
         0xc4, 0x2, 0xeb, 0x42, 0xd3, 0x76, 0x36, 0x8a, 0x71, 0x7b, 0x6b, 0xb8,
         0x3f, 0x7a, 0xb8, 0xc6, 0xf6, 0xca, 0xed, 0x74, 0xdb, 0xa4, 0x42, 0x65,
         0xa5, 0x96, 0xde, 0xd9, 0xfa, 0x45, 0x25, 0x9a, 0xc1, 0xde, 0xd1, 0xf6,
         0x7a, 0xaf, 0xa3, 0xfd, 0x4e, 0xa9
};

BOOST_AUTO_TEST_SUITE(TestTorrentFile)

BOOST_AUTO_TEST_CASE(CheckGettersSetters)
{

  TorrentFile file("/NTORRENT/linux15.01/.torrent-file",
                   "/NTORRENT/linux15.01/.torrent-file/segment2/AE321C",
                   "/NTORRENT/linux15.01",
                   {"/NTORRENT/linux15.01/file0/1A2B3C4D",
                   "/NTORRENT/linux15.01/file1/2A3B4C5E"});

  BOOST_CHECK_EQUAL(file.getName(), "/NTORRENT/linux15.01/.torrent-file");
  BOOST_CHECK_EQUAL(file.getCatalog().size(), 2);
  BOOST_CHECK_EQUAL(*(file.getTorrentFilePtr()), "/NTORRENT/linux15.01/.torrent-file/segment2/AE321C");
  BOOST_CHECK_EQUAL(file.getCatalog()[0], "/NTORRENT/linux15.01/file0/1A2B3C4D");
  BOOST_CHECK_EQUAL(file.getCatalog()[1], "/NTORRENT/linux15.01/file1/2A3B4C5E");

  TorrentFile file2("/NTORRENT/linux15.01/.torrent-file",
                    "/NTORRENT/linux15.01",
                    {"/NTORRENT/linux15.01/file0/1A2B3C4D"});

  BOOST_CHECK_EQUAL(file2.getName(), "/NTORRENT/linux15.01/.torrent-file");
  BOOST_CHECK(!file2.getTorrentFilePtr());
  BOOST_CHECK_EQUAL(file2.getCatalog().size(), 1);
}

BOOST_AUTO_TEST_CASE(CheckEncodeDecode)
{
  TorrentFile file("/NTORRENT/linux15.01/.torrent-file",
                   "/NTORRENT/linux15.01/.torrent-file/segment2/AE321C",
                   "/NTORRENT/linux15.01",
                   {"/NTORRENT/linux15.01/file0/1A2B3C4D",
                   "/NTORRENT/linux15.01/file1/2A3B4C5E"});

  KeyChain keyChain;
  keyChain.sign(file);

  Block wire = file.wireEncode();

  TorrentFile file2;
  file2.wireDecode(wire);

  BOOST_CHECK_EQUAL(file2.getName(), "/NTORRENT/linux15.01/.torrent-file");
  BOOST_CHECK_EQUAL(*(file2.getTorrentFilePtr()), "/NTORRENT/linux15.01/.torrent-file/segment2/AE321C");
  BOOST_CHECK_EQUAL(file2.getCatalog().size(), 2);
  BOOST_CHECK_EQUAL(file2.getCatalog()[0], "/NTORRENT/linux15.01/file0/1A2B3C4D");
  BOOST_CHECK_EQUAL(file2.getCatalog()[1], "/NTORRENT/linux15.01/file1/2A3B4C5E");
  BOOST_CHECK_EQUAL(file2.getCommonPrefix(), "/NTORRENT/linux15.01");
}

BOOST_AUTO_TEST_CASE(CheckEncodeDecodeNoTorrentFilePtr)
{
  TorrentFile file("/NTORRENT/linux15.01/.torrent-file",
                   "/NTORRENT/linux15.01",
                   {"/NTORRENT/linux15.01/file0/1A2B3C4D",
                   "/NTORRENT/linux15.01/file1/2A3B4C5E"});

  KeyChain keyChain;
  keyChain.sign(file);

  Block wire = file.wireEncode();

  TorrentFile file2;
  file2.wireDecode(wire);

  BOOST_CHECK_EQUAL(file2.getName(), "/NTORRENT/linux15.01/.torrent-file");
  BOOST_CHECK(!file2.getTorrentFilePtr());
  BOOST_CHECK_EQUAL(file2.getCatalog().size(), 2);
  BOOST_CHECK_EQUAL(file2.getCatalog()[0], "/NTORRENT/linux15.01/file0/1A2B3C4D");
  BOOST_CHECK_EQUAL(file2.getCatalog()[1], "/NTORRENT/linux15.01/file1/2A3B4C5E");
}

BOOST_AUTO_TEST_CASE(CheckEncodeDecodeEmptyTorrentFile)
{
  TorrentFile file("/NTORRENT/linux15.01/.torrent-file",
                  "",
                  {});

  KeyChain keyChain;
  keyChain.sign(file);

  Block wire = file.wireEncode();

  TorrentFile file2;

  BOOST_REQUIRE_THROW(file2.wireDecode(wire), TorrentFile::Error);
}


BOOST_AUTO_TEST_CASE(CheckEncodeDecodeEmptyCatalog)
{

  TorrentFile file("/NTORRENT/linux15.01/.torrent-file",
                  "/NTORRENT/linux15.01/.torrent-file/segment2/AE321C",
                  "/NTORRENT/linux15.01",
                  {});

  KeyChain keyChain;
  keyChain.sign(file);

  Block wire = file.wireEncode();

  TorrentFile file2;

  BOOST_REQUIRE_THROW(file2.wireDecode(wire), TorrentFile::Error);
}

BOOST_AUTO_TEST_CASE(DecodeFromWire)
{

  Block TorrentFileBlock(TorrentFileTest, sizeof(TorrentFileTest));
  TorrentFile file(TorrentFileBlock);

  BOOST_CHECK_EQUAL(file.getName(), "/NTORRENT/linux15.01/.torrent-file");
  BOOST_CHECK_EQUAL(*(file.getTorrentFilePtr()), "/NTORRENT/linux15.01/.torrent-file/segment2/AE321C");
  BOOST_CHECK_EQUAL(file.getCatalog().size(), 2);
  BOOST_CHECK_EQUAL(file.getCatalog()[0], "/NTORRENT/linux15.01/file0/1A2B3C4D");
  BOOST_CHECK_EQUAL(file.getCatalog()[1], "/NTORRENT/linux15.01/file1/2A3B4C5E");
  BOOST_CHECK_EQUAL(file.getCommonPrefix(), "/NTORRENT/linux15.01");
}

BOOST_AUTO_TEST_CASE(TestInsertErase)
{
  TorrentFile file("/NTORRENT/linux15.01/.torrent-file",
                   "/NTORRENT/linux15.01/.torrent-file/segment2/AE321C",
                   "/NTORRENT/linux15.01",
                   {"/NTORRENT/linux15.01/file0/1A2B3C4D",
                   "/NTORRENT/linux15.01/file1/2A3B4C5E"});

  file.erase("/NTORRENT/linux15.01/file0/1A2B3C4D");
  BOOST_CHECK_EQUAL(file.getCatalog()[0], "/NTORRENT/linux15.01/file1/2A3B4C5E");

  file.erase("/NTORRENT/linux15.01/file1/2A3B4C5E");
  BOOST_CHECK_EQUAL(file.getCatalog().size(), 0);

  file.insert("/NTORRENT/linux15.01/file3/AB34C5KA");
  BOOST_CHECK_EQUAL(file.getCatalog().size(), 1);
  BOOST_CHECK_EQUAL(file.getCatalog()[0], "/NTORRENT/linux15.01/file3/AB34C5KA");
}

BOOST_AUTO_TEST_CASE(TestInsertAndEncodeTwice)
{
  TorrentFile file("/NTORRENT/linux15.01/.torrent-file",
                   "/NTORRENT/linux15.01/.torrent-file/segment2/AE321C",
                   "/NTORRENT/linux15.01",
                   {"/NTORRENT/linux15.01/file0/1A2B3C4D",
                   "/NTORRENT/linux15.01/file1/2A3B4C5E"});

  KeyChain keyChain;
  keyChain.sign(file);
  Block wire = file.wireEncode();

  TorrentFile file2;
  file2.wireDecode(wire);
  BOOST_CHECK_EQUAL(file2.getCatalog().size(), 2);

  file.insert("/NTORRENT/linux15.01/file3/AB34C5KA");
  file.insert("/NTORRENT/linux15.01/file4/CB24C3GB");
  Block wire2 = file.wireEncode();

  file2.wireDecode(wire2);
  BOOST_CHECK_EQUAL(file2.getCatalog().size(), 4);

  BOOST_CHECK_EQUAL(file2.getCatalog()[0], "/NTORRENT/linux15.01/file0/1A2B3C4D");
  BOOST_CHECK_EQUAL(file2.getCatalog()[1], "/NTORRENT/linux15.01/file1/2A3B4C5E");
  BOOST_CHECK_EQUAL(file2.getCatalog()[2], "/NTORRENT/linux15.01/file3/AB34C5KA");
  BOOST_CHECK_EQUAL(file2.getCatalog()[3], "/NTORRENT/linux15.01/file4/CB24C3GB");
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests

} // namespace ntorrent

} // namespace ndn
