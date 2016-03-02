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

#include "torrent-manager.hpp"
#include "torrent-file.hpp"

#include <boost/filesystem.hpp>

#include <ndn-cxx/util/io.hpp>

namespace ndn {
namespace ntorrent {
namespace tests {

using std::vector;
using ndn::Name;

namespace fs = boost::filesystem;

class TestTorrentManager : public TorrentManager {
 public:
  TestTorrentManager(const ndn::Name&   torrentFileName,
                     const std::string& filePath)
  : TorrentManager(torrentFileName, filePath)
  {}

  std::vector<TorrentFile> torrentSegments() const {
    return m_torrentSegments;
  }

  std::vector<FileManifest> fileManifests() const {
    return m_fileManifests;
  }

  std::vector<bool> fileState(const ndn::Name& manifestName) {
    return m_fileStates[manifestName].second;
  }
};

BOOST_AUTO_TEST_SUITE(TestTorrentManagerInitialize)

BOOST_AUTO_TEST_CASE(CheckInitializeComplete)
{
  vector<FileManifest> manifests;
  vector<TorrentFile> torrentSegments;
  std::string filePath = "tests/testdata/";
  // get torrent files and manifests
  {
    auto temp = TorrentFile::generate("tests/testdata/foo",
                                      1024,
                                      1024,
                                      1024,
                                      false);
    torrentSegments = temp.first;
    auto temp1      = temp.second;
    for (const auto& ms : temp1) {
      for (const auto& m : ms.first) {
        manifests.push_back(m);
      }
    }
  }
  // write the torrent segments  and manifests to disk
  std::string dirPath = ".appdata/foo/";
  boost::filesystem::create_directories(dirPath);
  std::string torrentPath = dirPath + "torrent_files/";
  boost::filesystem::create_directory(torrentPath);
  auto fileNum = 0;
  for (const auto& t : torrentSegments) {
    fileNum++;
    auto filename = torrentPath + to_string(fileNum);
    io::save(t, filename);
  }
  fileNum = 0;
  auto manifestPath = dirPath + "manifests/";
  boost::filesystem::create_directory(manifestPath);
  for (const auto& m : manifests) {
    fileNum++;
    auto filename = manifestPath + to_string(fileNum);
    io::save(m, filename);
  }
  // Initialize and verify
  TestTorrentManager manager("/NTORRENT/foo/torrent-file/sha256digest=02c737fd4c6e7de4b4825b089f39700c2dfa8fd2bb2b91f09201e357c4463253",
                             filePath);
  manager.Initialize();

  // Check that the torrent segments and file manifests match (content and order)
  BOOST_CHECK(manager.torrentSegments() == torrentSegments);
  BOOST_CHECK(manager.fileManifests()   == manifests);
  // next check the data packet state vectors
  for (auto m : manager.fileManifests()) {
    auto fileState = manager.fileState(m.getFullName());
    BOOST_CHECK(fileState.size() == m.catalog().size());
    for (auto s : fileState) {
      BOOST_CHECK(s);
    }
  }
  fs::remove_all(dirPath);
}

BOOST_AUTO_TEST_CASE(CheckInitializeEmpty)
{
  TestTorrentManager manager("/NTORRENT/foo/torrent-file/sha256digest=02c737fd4c6e7de4b4825b089f39700c2dfa8fd2bb2b91f09201e357c4463253",
                             "tests/testdata/");
  manager.Initialize();
  BOOST_CHECK(manager.torrentSegments() == vector<TorrentFile>());
  BOOST_CHECK(manager.fileManifests()   == vector<FileManifest>());
}

BOOST_AUTO_TEST_CASE(CheckInitializeNoManifests)
{
  vector<TorrentFile> torrentSegments;
  std::string filePath = "tests/testdata/";
  // get torrent files and manifests
  {
    auto temp = TorrentFile::generate("tests/testdata/foo",
                                      1024,
                                      1024,
                                      1024,
                                      false);
    torrentSegments = temp.first;
  }
  // write the torrent segments but no manifests to disk
  std::string dirPath = ".appdata/foo/";
  boost::filesystem::create_directories(dirPath);
  std::string torrentPath = dirPath + "torrent_files/";
  boost::filesystem::create_directory(torrentPath);
  auto fileNum = 0;
  for (const auto& t : torrentSegments) {
    fileNum++;
    auto filename = torrentPath + to_string(fileNum);
    io::save(t, filename);
  }
  // Initialize and verify
  TestTorrentManager manager("/NTORRENT/foo/torrent-file/sha256digest=02c737fd4c6e7de4b4825b089f39700c2dfa8fd2bb2b91f09201e357c4463253",
                             filePath);
  manager.Initialize();

  // Check that the torrent segments and file manifests match (content and order)
  BOOST_CHECK(manager.torrentSegments() == torrentSegments);
  BOOST_CHECK(manager.fileManifests()   == vector<FileManifest>());

  fs::remove_all(dirPath);
}

BOOST_AUTO_TEST_CASE(CheckInitializeMissingManifests)
{
  vector<FileManifest> manifests;
  vector<TorrentFile> torrentSegments;
  std::string filePath = "tests/testdata/";
  // get torrent files and manifests
  {
    auto temp = TorrentFile::generate("tests/testdata/foo",
                                      1024,
                                      1024,
                                      1024,
                                      false);
    torrentSegments = temp.first;
    auto temp1      = temp.second;
    temp1.pop_back(); // remove the manifests for the last file
    for (const auto& ms : temp1) {
      for (const auto& m : ms.first) {
        manifests.push_back(m);
      }
    }
  }
  // write the torrent segments  and manifests to disk
  std::string dirPath = ".appdata/foo/";
  boost::filesystem::create_directories(dirPath);
  std::string torrentPath = dirPath + "torrent_files/";
  boost::filesystem::create_directories(torrentPath);
  auto fileNum = 0;
  for (const auto& t : torrentSegments) {
    fileNum++;
    auto filename = torrentPath + to_string(fileNum);
    io::save(t, filename);

  }
  fileNum = 0;
  auto manifestPath = dirPath + "manifests/";
  boost::filesystem::create_directory(manifestPath);
  for (const auto& m : manifests) {
    fileNum++;
    auto filename = manifestPath + to_string(fileNum);
    io::save(m, filename);
  }
  // Initialize and verify
  TestTorrentManager manager("/NTORRENT/foo/torrent-file/sha256digest=02c737fd4c6e7de4b4825b089f39700c2dfa8fd2bb2b91f09201e357c4463253",
                             filePath);
  manager.Initialize();

  // Check that the torrent segments and file manifests match (content and order)
  BOOST_CHECK(manager.torrentSegments() == torrentSegments);
  BOOST_CHECK(manager.fileManifests()   == manifests);
  // next check the data packet state vectors
  for (auto m : manager.fileManifests()) {
    auto fileState = manager.fileState(m.getFullName());
    BOOST_CHECK(fileState.size() == m.catalog().size());
    for (auto s : fileState) {
      BOOST_CHECK(s);
    }
  }
  fs::remove_all(dirPath);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nTorrent
} // namespace ndn