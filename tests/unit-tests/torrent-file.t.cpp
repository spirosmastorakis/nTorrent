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
#include "file-manifest.hpp"

#include <ndn-cxx/data.hpp>
#include <ndn-cxx/signature.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/util/io.hpp>

#include <boost/range/adaptors.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

namespace fs = boost::filesystem;

BOOST_TEST_DONT_PRINT_LOG_VALUE(std::nullptr_t)

namespace ndn {

namespace ntorrent {

namespace tests {


const static uint8_t TorrentFileTest[] = {
  0x6, 0xfd, // data
  0x1, 0xea,
     0x7, 0x2c, // name
        0x8, 0x8,
           0x4e, 0x54, 0x4f, 0x52, 0x52, 0x45, 0x4e, 0x54,
        0x8, 0xa,
           0x6c, 0x69, 0x6e, 0x75, 0x78, 0x31, 0x35, 0x2e, 0x30, 0x31,
        0x8, 0xc,
           0x74, 0x6f, 0x72, 0x72, 0x65, 0x6e, 0x74, 0x2d, 0x66, 0x69, 0x6c, 0x65,
        0x8, 0x6,
           0x41, 0x42, 0x32, 0x43, 0x44, 0x41,
      0x14, 0x0, // meta-info
      0x15, 0x76,  // content
          0x7, 0x36,
             0x8, 0x8,
                0x4e, 0x54, 0x4f, 0x52, 0x52, 0x45, 0x4e, 0x54,
             0x8, 0xa,
                0x6c, 0x69, 0x6e, 0x75, 0x78, 0x31, 0x35, 0x2e, 0x30, 0x31,
             0x8, 0xc,
                0x74, 0x6f, 0x72, 0x72, 0x65, 0x6e, 0x74, 0x2d, 0x66, 0x69, 0x6c, 0x65,
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
      0x16, 0x3c, // SignatureInfo
          0x1b, 0x1, // SignatureType
              0x1,
          0x1c, 0x37, // KeyLocator
              0x7, 0x35,
                 0x8, 0xc,
                    0x74, 0x6d, 0x70, 0x2d, 0x69, 0x64, 0x65, 0x6e, 0x74, 0x69, 0x74, 0x79,
                 0x8, 0x4,
                    0x3d, 0xa8, 0x2c, 0x19,
                 0x8, 0x3,
                    0x4b, 0x45, 0x59,
                 0x8, 0x11,
                    0x6b, 0x73, 0x6b, 0x2d, 0x31, 0x34, 0x35, 0x34, 0x39, 0x37, 0x33, 0x38,
                    0x37, 0x36, 0x31, 0x37, 0x31,
                 0x8, 0x7,
                    0x49, 0x44, 0x2d, 0x43, 0x45, 0x52, 0x54,
      0x17, 0xfd, // SignatureValue
          0x1, 0x0, 0x3d, 0xb6, 0x32, 0x59, 0xbd, 0xe5, 0xad, 0x27, 0x1b, 0xc2, 0x68,
          0x3d, 0x7a, 0xd3, 0x4e, 0xf, 0x40, 0xa5, 0xf8, 0x80, 0x4b, 0xf2, 0xf9, 0xb1,
          0x7, 0xea, 0x56, 0x84, 0xd, 0x94, 0xd, 0xf9, 0x88, 0xd4, 0xe0, 0xb6, 0x63, 0xac,
          0x3a, 0xdc, 0x17, 0xc2, 0xff, 0xde, 0xc, 0xc3, 0xef, 0xb, 0x3b, 0x1a, 0xef, 0xb,
          0x7, 0x99, 0x9b, 0xb7, 0xe6, 0x4a, 0x68, 0xf8, 0xda, 0x62, 0x29, 0xfd, 0xb1, 0xb1,
          0xe9, 0xe3, 0x3a, 0x42, 0x1f, 0xbd, 0xad, 0x96, 0xc4, 0x5f, 0x34, 0x55, 0x9, 0xba,
          0x7a, 0x82, 0x2d, 0x9c, 0x7b, 0x52, 0x9d, 0xf, 0xd6, 0xea, 0xc3, 0x60, 0xbf, 0x75,
          0xfb, 0x0, 0x1b, 0xb8, 0x4, 0xa8, 0x6e, 0x95, 0x5a, 0xcf, 0xca, 0x90, 0x20, 0x21,
          0x58, 0x27, 0x2c, 0xef, 0xed, 0x23, 0x37, 0xfa, 0xd, 0xc6, 0x3, 0x7b, 0xd6, 0xef,
          0x76, 0x3, 0xd7, 0x22, 0xd8, 0x6f, 0xa2, 0x7d, 0xc2, 0x84, 0xee, 0x65, 0xbf, 0x63,
          0xe2, 0x27, 0xc4, 0xeb, 0xb7, 0xb2, 0xa6, 0xa0, 0x58, 0x68, 0x1a, 0x3a, 0x85, 0xd8,
          0x8d, 0xd8, 0xfb, 0x52, 0x9b, 0xba, 0x22, 0x2d, 0xb2, 0x4b, 0x9e, 0x3d, 0xf5, 0x61,
          0x83, 0x6f, 0x78, 0x67, 0x85, 0x3d, 0xf, 0x24, 0x47, 0xbf, 0x8c, 0xdf, 0xdf, 0x8a,
          0xc8, 0xa9, 0xed, 0xaf, 0x8b, 0x49, 0xdf, 0x2d, 0x2a, 0xfe, 0x2f, 0x4b, 0xc5, 0xe6,
          0x7e, 0xee, 0x35, 0xf1, 0x6, 0x88, 0x9c, 0xa0, 0x25, 0xf, 0x6, 0x56, 0xf4, 0x72,
          0x89, 0x2f, 0x95, 0x64, 0x39, 0x38, 0x21, 0xc3, 0x75, 0xef, 0x80, 0x5a, 0x73, 0x1a,
          0xec, 0xb9, 0x6d, 0x3, 0x47, 0xac, 0x64, 0x7f, 0x85, 0xbe, 0xb9, 0xab, 0x87, 0x9f,
          0xd2, 0x9c, 0xe7, 0x9c, 0x86, 0xd5, 0x59, 0x65, 0x73, 0x24, 0x15, 0x3e, 0xfc, 0x94,
          0xf8, 0x7, 0x26, 0x9b, 0x4f, 0x6e, 0x1b, 0x1c
};

BOOST_AUTO_TEST_SUITE(TestTorrentFile)

BOOST_AUTO_TEST_CASE(CheckGettersSetters)
{

  TorrentFile file("/ndn/multicast/NTORRENT/linux15.01/torrent-file/AB2CDA",
                   "/ndn/multicast/NTORRENT/linux15.01/torrent-file/segment2/AE321C",
                   "/ndn/multicast/NTORRENT/linux15.01",
                   {"/ndn/multicast/NTORRENT/linux15.01/file0/1A2B3C4D",
                   "/ndn/multicast/NTORRENT/linux15.01/file1/2A3B4C5E"});

  BOOST_CHECK_EQUAL(file.getName(), "/ndn/multicast/NTORRENT/linux15.01/torrent-file/AB2CDA");
  BOOST_CHECK_EQUAL(file.getCatalog().size(), 2);
  BOOST_CHECK_EQUAL(*(file.getTorrentFilePtr()),
                   "/ndn/multicast/NTORRENT/linux15.01/torrent-file/segment2/AE321C");
  BOOST_CHECK_EQUAL(file.getCatalog()[0], "/ndn/multicast/NTORRENT/linux15.01/file0/1A2B3C4D");
  BOOST_CHECK_EQUAL(file.getCatalog()[1], "/ndn/multicast/NTORRENT/linux15.01/file1/2A3B4C5E");

  TorrentFile file2("/ndn/multicast/NTORRENT/linux15.01/torrent-file/AB2CDA",
                    "/ndn/multicast/NTORRENT/linux15.01",
                    {"/ndn/multicast/NTORRENT/linux15.01/file0/1A2B3C4D"});

  BOOST_CHECK_EQUAL(file2.getName(), "/ndn/multicast/NTORRENT/linux15.01/torrent-file/AB2CDA");
  BOOST_CHECK(!file2.getTorrentFilePtr());
  BOOST_CHECK_EQUAL(file2.getCatalog().size(), 1);
}

BOOST_AUTO_TEST_CASE(CheckEncodeDecode)
{
  TorrentFile file("/ndn/multicast/NTORRENT/linux15.01/torrent-file/AB2CDA",
                   "/ndn/multicast/NTORRENT/linux15.01/torrent-file/segment2/AE321C",
                   "/ndn/multicast/NTORRENT/linux15.01",
                   {"/ndn/multicast/NTORRENT/linux15.01/file0/1A2B3C4D",
                   "/ndn/multicast/NTORRENT/linux15.01/file1/2A3B4C5E"});

  file.finalize();
  KeyChain keyChain;
  keyChain.sign(file);

  Block wire = file.wireEncode();

  TorrentFile file2;
  file2.wireDecode(wire);

  BOOST_CHECK_EQUAL(file2.getName(), "/ndn/multicast/NTORRENT/linux15.01/torrent-file/AB2CDA");
  BOOST_CHECK_EQUAL(*(file2.getTorrentFilePtr()),
                   "/ndn/multicast/NTORRENT/linux15.01/torrent-file/segment2/AE321C");
  BOOST_CHECK_EQUAL(file2.getCatalog().size(), 2);
  BOOST_CHECK_EQUAL(file2.getCatalog()[0], "/ndn/multicast/NTORRENT/linux15.01/file0/1A2B3C4D");
  BOOST_CHECK_EQUAL(file2.getCatalog()[1], "/ndn/multicast/NTORRENT/linux15.01/file1/2A3B4C5E");
  BOOST_CHECK_EQUAL(file2.getCommonPrefix(), "/ndn/multicast/NTORRENT/linux15.01");
}

BOOST_AUTO_TEST_CASE(CheckEncodeDecodeNoTorrentFilePtr)
{
  TorrentFile file("/ndn/multicast/NTORRENT/linux15.01/torrent-file/AB2CDA",
                   "/ndn/multicast/NTORRENT/linux15.01",
                   {"/ndn/multicast/NTORRENT/linux15.01/file0/1A2B3C4D",
                   "/ndn/multicast/NTORRENT/linux15.01/file1/2A3B4C5E"});

  file.finalize();
  KeyChain keyChain;
  keyChain.sign(file);

  Block wire = file.wireEncode();

  TorrentFile file2;
  file2.wireDecode(wire);

  BOOST_CHECK_EQUAL(file2.getName(), "/ndn/multicast/NTORRENT/linux15.01/torrent-file/AB2CDA");
  BOOST_CHECK(!file2.getTorrentFilePtr());
  BOOST_CHECK_EQUAL(file2.getCatalog().size(), 2);
  BOOST_CHECK_EQUAL(file2.getCatalog()[0], "/ndn/multicast/NTORRENT/linux15.01/file0/1A2B3C4D");
  BOOST_CHECK_EQUAL(file2.getCatalog()[1], "/ndn/multicast/NTORRENT/linux15.01/file1/2A3B4C5E");
}

BOOST_AUTO_TEST_CASE(CheckEncodeDecodeEmptyTorrentFile)
{
  TorrentFile file("/ndn/multicast/NTORRENT/linux15.01/torrent-file/AB2CDA",
                  "",
                  {});

  file.finalize();
  KeyChain keyChain;
  keyChain.sign(file);

  Block wire = file.wireEncode();

  TorrentFile file2;

  BOOST_REQUIRE_THROW(file2.wireDecode(wire), TorrentFile::Error);
}


BOOST_AUTO_TEST_CASE(CheckEncodeDecodeEmptyCatalog)
{

  TorrentFile file("/ndn/multicast/NTORRENT/linux15.01/torrent-file/AB2CDA",
                  "/ndn/multicast/NTORRENT/linux15.01/torrent-file/segment2/AE321C",
                  "/ndn/multicast/NTORRENT/linux15.01",
                  {});

  file.finalize();
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

  BOOST_CHECK_EQUAL(file.getName(), "/NTORRENT/linux15.01/torrent-file/AB2CDA");
  BOOST_CHECK_EQUAL(*(file.getTorrentFilePtr()),
                    "/NTORRENT/linux15.01/torrent-file/segment2/AE321C");
  BOOST_CHECK_EQUAL(file.getCatalog().size(), 2);
  BOOST_CHECK_EQUAL(file.getCatalog()[0], "/NTORRENT/linux15.01/file0/1A2B3C4D");
  BOOST_CHECK_EQUAL(file.getCatalog()[1], "/NTORRENT/linux15.01/file1/2A3B4C5E");
  BOOST_CHECK_EQUAL(file.getCommonPrefix(), "/NTORRENT/linux15.01");
}

BOOST_AUTO_TEST_CASE(TestInsertErase)
{
  TorrentFile file("/ndn/multicast/NTORRENT/linux15.01/torrent-file/AB2CDA",
                   "/ndn/multicast/NTORRENT/linux15.01/torrent-file/segment2/AE321C",
                   "/ndn/multicast/NTORRENT/linux15.01",
                   {"/ndn/multicast/NTORRENT/linux15.01/file0/1A2B3C4D",
                   "/ndn/multicast/NTORRENT/linux15.01/file1/2A3B4C5E"});

  file.erase("/ndn/multicast/NTORRENT/linux15.01/file0/1A2B3C4D");
  BOOST_CHECK_EQUAL(file.getCatalog()[0], "/ndn/multicast/NTORRENT/linux15.01/file1/2A3B4C5E");

  file.erase("/ndn/multicast/NTORRENT/linux15.01/file1/2A3B4C5E");
  BOOST_CHECK_EQUAL(file.getCatalog().size(), 0);

  file.insert("/ndn/multicast/NTORRENT/linux15.01/file3/AB34C5KA");
  BOOST_CHECK_EQUAL(file.getCatalog().size(), 1);
  BOOST_CHECK_EQUAL(file.getCatalog()[0], "/ndn/multicast/NTORRENT/linux15.01/file3/AB34C5KA");
}

BOOST_AUTO_TEST_CASE(TestInsertAndEncodeTwice)
{
  TorrentFile file("/ndn/multicast/NTORRENT/linux15.01/torrent-file/AB2CDA",
                   "/ndn/multicast/NTORRENT/linux15.01/torrent-file/segment2/AE321C",
                   "/ndn/multicast/NTORRENT/linux15.01",
                   {"/ndn/multicast/NTORRENT/linux15.01/file0/1A2B3C4D",
                   "/ndn/multicast/NTORRENT/linux15.01/file1/2A3B4C5E"});

  file.finalize();
  KeyChain keyChain;
  keyChain.sign(file);
  Block wire = file.wireEncode();

  TorrentFile file2;
  file2.wireDecode(wire);
  BOOST_CHECK_EQUAL(file2.getCatalog().size(), 2);

  file.insert("/ndn/multicast/NTORRENT/linux15.01/file3/AB34C5KA");
  file.insert("/ndn/multicast/NTORRENT/linux15.01/file4/CB24C3GB");
  file.finalize();
  Block wire2 = file.wireEncode();

  file2.wireDecode(wire2);
  BOOST_CHECK_EQUAL(file2.getCatalog().size(), 4);

  BOOST_CHECK_EQUAL(file2.getCatalog()[0], "/ndn/multicast/NTORRENT/linux15.01/file0/1A2B3C4D");
  BOOST_CHECK_EQUAL(file2.getCatalog()[1], "/ndn/multicast/NTORRENT/linux15.01/file1/2A3B4C5E");
  BOOST_CHECK_EQUAL(file2.getCatalog()[2], "/ndn/multicast/NTORRENT/linux15.01/file3/AB34C5KA");
  BOOST_CHECK_EQUAL(file2.getCatalog()[3], "/ndn/multicast/NTORRENT/linux15.01/file4/CB24C3GB");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_CASE(TestTorrentFileGenerator)
{
  const struct {
      size_t         d_dataPacketSize;
      size_t         d_subManifestSize;
      const char     *d_directoryPath;
      size_t         d_namesPerSegment;
      bool           d_shouldThrow;
  } DATA [] = {
    // Affirmative tests
    {1024           , 100          , "tests/testdata/foo" , 1, false },
    {512            , 80           , "tests/testdata/foo" , 3, false },
    {256            , 50           , "tests/testdata/foo" , 2, false },
    {2048           , 20           , "tests/testdata/foo" , 4, false },
    {512            , 80           , "tests/testdata/foo" , 1, false },
    {1024           , 50           , "tests/testdata/foo" , 1, false },
    {2048           , 128          , "tests/testdata/foo",  2, false },
    // Negative tests
    // non-existent directory
    {128          , 128            , "tests/testdata/foo-fake", 2, true },
  };
  enum { NUM_DATA = sizeof DATA / sizeof *DATA };
  for (int i = 0; i < NUM_DATA; ++i) {
    auto dataPacketSize  = DATA[i].d_dataPacketSize;
    auto subManifestSize = DATA[i].d_subManifestSize;
    auto directoryPath   = DATA[i].d_directoryPath;
    auto namesPerSegment = DATA[i].d_namesPerSegment;
    auto shouldThrow     = DATA[i].d_shouldThrow;


    std::pair<std::vector<TorrentFile>,
              std::vector<std::pair<std::vector<FileManifest>,
                                    std::vector<Data>>>> torrentFilePair1;

    std::pair<std::vector<TorrentFile>,
              std::vector<std::pair<std::vector<FileManifest>,
                                    std::vector<Data>>>> torrentFilePair2;

    if (shouldThrow) {
      BOOST_REQUIRE_THROW(TorrentFile::generate(directoryPath,
                                              namesPerSegment,
                                              subManifestSize,
                                              dataPacketSize),
                          TorrentFile::Error);

      BOOST_REQUIRE_THROW(TorrentFile::generate(directoryPath,
                                                namesPerSegment,
                                                subManifestSize,
                                                dataPacketSize,
                                                true),
                          TorrentFile::Error);
    }
    else {
      torrentFilePair1 = TorrentFile::generate(directoryPath,
                                              namesPerSegment,
                                              subManifestSize,
                                              dataPacketSize);

      torrentFilePair2 = TorrentFile::generate(directoryPath,
                                              namesPerSegment,
                                              subManifestSize,
                                              dataPacketSize,
                                              true);

      auto torrentFileSegments = torrentFilePair1.first;
      auto manifestPairs1 = torrentFilePair1.second;
      auto manifestPairs2 = torrentFilePair2.second;

      // Check that generate has not returned any data packets unless otherwise specified
      for (auto j = manifestPairs1.begin(); j != manifestPairs1.end(); ++j) {
        BOOST_CHECK_EQUAL((*j).second.size(), 0);
      }

      Name directoryPathName(directoryPath);
      fs::recursive_directory_iterator directoryPtr(fs::system_complete(directoryPath).string());
      int numberOfDirectoryFiles = 0;
      for (fs::recursive_directory_iterator j = directoryPtr;
           j != fs::recursive_directory_iterator(); ++j) {
        numberOfDirectoryFiles++;
      }
      // Verify the basic attributes of the torrent-file
      for (auto it = torrentFileSegments.begin(); it != torrentFileSegments.end(); ++it) {
        // Verify that each file torrent-file is signed
        BOOST_CHECK_NO_THROW(it->getFullName());
        BOOST_CHECK_EQUAL(it->getCommonPrefix(),
                          Name("/ndn/multicast/NTORRENT" +
                               directoryPathName.getSubName(
                                 directoryPathName.size() - 1).toUri()));
        BOOST_CHECK_EQUAL(*it, TorrentFile(it->wireEncode()));
        if (it != torrentFileSegments.end() - 1) {
          BOOST_CHECK_EQUAL(it->getCatalog().size(), namesPerSegment);
          BOOST_CHECK_EQUAL(*(it->getTorrentFilePtr()), (it+1)->getFullName());
        }
        else {
          BOOST_CHECK_LE(it->getCatalog().size(), subManifestSize);
          BOOST_CHECK(!(it->getTorrentFilePtr()));
        }
      }
      int myDiv = (numberOfDirectoryFiles / namesPerSegment);
      if (numberOfDirectoryFiles % namesPerSegment == 0) {
        BOOST_CHECK_EQUAL(torrentFileSegments.size(), myDiv);
      }
      else {
        BOOST_CHECK_EQUAL(torrentFileSegments.size(), (myDiv + 1));
      }

      std::vector<uint8_t> dataBytes;
      for (auto j = manifestPairs2.begin() ; j != manifestPairs2.end(); ++j) {
        for (auto d : (*j).second) {
          auto content = d.getContent();
          dataBytes.insert(dataBytes.end(), content.value_begin(), content.value_end());
        }
      }
      // load  the contents of the directory files from disk
      std::vector<uint8_t> directoryFilesBytes;
      fs::recursive_directory_iterator directoryPtr2(fs::system_complete(directoryPath).string());

      std::set<std::string> fileNames;
      for (auto j = directoryPtr2; j != fs::recursive_directory_iterator(); ++j)  {
        fileNames.insert(j->path().string());
      }
      for (const auto& fileName : fileNames) {
        fs::ifstream is(fileName, fs::ifstream::binary | fs::ifstream::in);
        is >> std::noskipws;
        std::istream_iterator<uint8_t> start(is), end;
        std::vector<uint8_t> fileBytes(start, end);
        directoryFilesBytes.insert(directoryFilesBytes.end(), fileBytes.begin(), fileBytes.end());
      }
      // confirm that they are equal
      BOOST_CHECK_EQUAL_COLLECTIONS(directoryFilesBytes.begin(), directoryFilesBytes.end(),
                                    dataBytes.begin(), dataBytes.end());
      // test that we can write the torrent file to the disk and read it out again
      {
        std::string dirPath = "tests/testdata/temp/";
        boost::filesystem::create_directory(dirPath);
        auto fileNum = 0;
        for (auto &s : torrentFileSegments) {
          fileNum++;
          auto filename = dirPath + to_string(fileNum);
          io::save(s, filename);
        }
        // read them back out
        fileNum = 0;
        for (auto &s : torrentFileSegments) {
          fileNum++;
          auto filename = dirPath + to_string(fileNum);
          auto torrent_ptr = io::load<TorrentFile>(filename);
          BOOST_CHECK_NE(torrent_ptr, nullptr);
          BOOST_CHECK_EQUAL(s, *torrent_ptr);
        }
        fs::remove_all(dirPath);
      }
    }
  }
}

} // namespace tests

} // namespace ntorrent

} // namespace ndn
