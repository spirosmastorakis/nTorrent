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
#include "unit-test-time-fixture.hpp"

#include <set>

#include <boost/filesystem.hpp>

#include <ndn-cxx/util/dummy-client-face.hpp>
#include <ndn-cxx/util/io.hpp>

namespace ndn {
namespace ntorrent {
namespace tests {

using std::vector;
using ndn::Name;
using ndn::util::DummyClientFace;

namespace fs = boost::filesystem;

class TestTorrentManager : public TorrentManager {
 public:
  TestTorrentManager(const ndn::Name&   torrentFileName,
                     const std::string& filePath)
  : TorrentManager(torrentFileName, filePath)
  {
  }

  TestTorrentManager(const ndn::Name&   torrentFileName,
                     const std::string& filePath,
                     DummyClientFace& face)
  : TorrentManager(torrentFileName, filePath, face)
  {
  }

  std::vector<TorrentFile> torrentSegments() const {
    return m_torrentSegments;
  }

  std::vector<FileManifest> fileManifests() const {
    return m_fileManifests;
  }

  std::vector<bool> fileState(const ndn::Name& manifestName) {
    auto fout = m_fileStates[manifestName].first;
    if (nullptr != fout) {
      fout->flush();
    }
    return m_fileStates[manifestName].second;
  }

  bool writeData(const Data& data) {
    return TorrentManager::writeData(data);
  }

  bool writeTorrentSegment(const TorrentFile& segment, const std::string& path) {
    return TorrentManager::writeTorrentSegment(segment, path);
  }
  bool writeFileManifest(const FileManifest& manifest, const std::string& path) {
    return TorrentManager::writeFileManifest(manifest, path);
  }
};

class FaceFixture : public UnitTestTimeFixture
{
public:
  explicit
  FaceFixture(bool enableRegistrationReply = true)
    : face(io, { true, enableRegistrationReply })
  {
  }

public:
  DummyClientFace face;
};

class FacesNoRegistrationReplyFixture : public FaceFixture
{
public:
  FacesNoRegistrationReplyFixture()
    : FaceFixture(false)
  {
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
      manifests.insert(manifests.end(), ms.first.begin(), ms.first.end());
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
  //fileNum = 0;
  auto manifestPath = dirPath + "manifests/";
  boost::filesystem::create_directory(manifestPath);
  for (const auto& m : manifests) {
    fs::path filename = manifestPath + m.file_name() + "/" + to_string(m.submanifest_number());
    boost::filesystem::create_directories(filename.parent_path());
    io::save(m, filename.string());
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
    for (const  auto& ms : temp1) {
      manifests.insert(manifests.end(), ms.first.begin(), ms.first.end());
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
  auto manifestPath = dirPath + "manifests/";
  boost::filesystem::create_directory(manifestPath);
  for (const auto& m : manifests) {
    fs::path filename = manifestPath + m.file_name() + to_string(m.submanifest_number());
    boost::filesystem::create_directory(filename.parent_path());
    io::save(m, filename.string());
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

BOOST_FIXTURE_TEST_SUITE(TestTorrentManagerNetworkingStuff, FaceFixture)

BOOST_AUTO_TEST_CASE(TestDownloadingTorrentFile)
{
  vector<FileManifest> manifests;
  vector<TorrentFile> torrentSegments;
  std::string filePath = ".appdata/foo/";
  // get torrent files and manifests
  {
    auto temp = TorrentFile::generate("tests/testdata/foo", 1, 10, 10, false);

    torrentSegments = temp.first;
    auto temp1      = temp.second;
    temp1.pop_back(); // remove the manifests for the last file
    for (const auto& ms : temp1) {
      for (const auto& m : ms.first) {
        manifests.push_back(m);
      }
    }
  }

  TestTorrentManager manager("/NTORRENT/foo/torrent-file/sha256digest=946b92641d2b87bf4f5913930be20e3789ff5fb5d72739614f93f677d90fbd9d",
                             filePath, face);
  manager.Initialize();

  // Test download torrent file segments
  uint32_t counter = 0;
  manager.downloadTorrentFile(filePath + "torrent_files", [&counter, &torrentSegments] (const ndn::Name& name) {
                                          BOOST_CHECK_EQUAL(torrentSegments[counter].getName(), name);
                                          counter++;
                                        },
                                        bind([] {
                                          BOOST_FAIL("Unexpected failure");
                                        }));

  for (auto i = torrentSegments.begin(); i != torrentSegments.end(); i++) {
    advanceClocks(time::milliseconds(1), 40);
    face.receive(dynamic_cast<Data&>(*i));
  }

  fs::remove_all(filePath);
}

BOOST_AUTO_TEST_CASE(TestDownloadingFileManifests)
{
  vector<FileManifest> manifests;
  vector<TorrentFile> torrentSegments;
  std::string filePath = ".appdata/foo/";
  // get torrent files and manifests
  {
    auto temp = TorrentFile::generate("tests/testdata/foo", 1, 10, 10, false);

    torrentSegments = temp.first;
    auto temp1      = temp.second;
    temp1.pop_back(); // remove the manifests for the last file
    for (const auto& ms : temp1) {
      for (const auto& m : ms.first) {
        manifests.push_back(m);
      }
    }
  }

  TestTorrentManager manager("/NTORRENT/foo/torrent-file/sha256digest=946b92641d2b87bf4f5913930be20e3789ff5fb5d72739614f93f677d90fbd9d",
                             filePath, face);
  manager.Initialize();

  // Test download manifest segments -- 2 files (the first one segment, the second multiple)
  int counter = 0;
  manager.download_file_manifest(manifests[0].getFullName(), filePath + "manifests",
                                [&counter, &manifests]
                                (const std::vector<ndn::Name>& vec) {
                                  uint32_t packetIndex = 0;
                                  for (auto j = vec.begin(); j != vec.end(); j++) {
                                    BOOST_CHECK_EQUAL(manifests[counter].catalog()[packetIndex],
                                                      *j);
                                    packetIndex++;
                                  }
                                  counter++;
                                },
                                [](const ndn::Name& name, const std::string& reason) {
                                  BOOST_FAIL("Unexpected failure");
                                });

  advanceClocks(time::milliseconds(1), 40);
  face.receive(dynamic_cast<Data&>(manifests[0]));

  manager.download_file_manifest(manifests[1].getFullName(), filePath + "manifests",
                                [&counter, &manifests]
                                (const std::vector<ndn::Name>& vec) {
                                  uint32_t packetIndex = 0;
                                  for (auto j = vec.begin(); j != vec.end(); j++) {
                                    BOOST_CHECK_EQUAL(manifests[counter].catalog()[packetIndex],
                                                      *j);
                                    // if we have read all the packet names from a
                                    // segment, move to the next one
                                    if (packetIndex == manifests[counter].catalog().size() - 1) {
                                      packetIndex = 0;
                                      counter++;
                                    }
                                    else {
                                      packetIndex++;
                                    }
                                  }
                                },
                                [](const ndn::Name& name, const std::string& reason) {
                                  BOOST_FAIL("Unexpected failure");
                                });

  for (auto i = manifests.begin() + 1; i != manifests.end(); i++) {
    advanceClocks(time::milliseconds(1), 40);
    face.receive(dynamic_cast<Data&>(*i));
  }

  fs::remove_all(filePath);
}

BOOST_AUTO_TEST_CASE(TestDownloadingDataPackets)
{
  std::string filePath = ".appdata/foo/";
  TestTorrentManager manager("/NTORRENT/foo/torrent-file/sha256digest=946b92641d2b87bf4f5913930be20e3789ff5fb5d72739614f93f677d90fbd9d",
                             filePath, face);
  manager.Initialize();

  Name dataName("/test/ucla");

  // Download data successfully
  manager.download_data_packet(dataName,
                              [&dataName] (const ndn::Name& name) {
                                BOOST_CHECK_EQUAL(name, dataName);
                              },
                              [](const ndn::Name& name, const std::string& reason) {
                                BOOST_FAIL("Unexpected failure");
                              });

  auto data = make_shared<Data>(dataName);
  SignatureSha256WithRsa fakeSignature;
  fakeSignature.setValue(encoding::makeEmptyBlock(tlv::SignatureValue));
  data->setSignature(fakeSignature);
  data->wireEncode();

  advanceClocks(time::milliseconds(1), 40);
  face.receive(*data);

  // Fail to download data
  manager.download_data_packet(dataName,
                              [] (const ndn::Name& name) {
                                BOOST_FAIL("Unexpected failure");
                              },
                              [&dataName](const ndn::Name& name, const std::string& reason) {
                                BOOST_CHECK_EQUAL(name, dataName);
                              });

  advanceClocks(time::milliseconds(1), 2100);

  fs::remove_all(filePath);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(CheckTorrentManagerUtilities)

BOOST_AUTO_TEST_CASE(CheckWriteDataComplete)
{
  vector<FileManifest> manifests;
  vector<TorrentFile>  torrentSegments;
  // for each file, the data packets
  std::vector<vector<Data>> fileData;
  std::string filePath = "tests/testdata/temp";
  // get torrent files and manifests
  {
    auto temp = TorrentFile::generate("tests/testdata/foo",
                                      1024,
                                      1024,
                                      1024,
                                      true);
    torrentSegments = temp.first;
    auto temp1      = temp.second;
    for (const auto& ms : temp1) {
      manifests.insert(manifests.end(), ms.first.begin(), ms.first.end());
      fileData.push_back(ms.second);
    }
  }
  // write the torrent segments and manifests to disk
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
  //fileNum = 0;
  auto manifestPath = dirPath + "manifests/";
  boost::filesystem::create_directory(manifestPath);
  for (const auto& m : manifests) {
    fs::path filename = manifestPath + m.file_name() + to_string(m.submanifest_number());
    boost::filesystem::create_directory(filename.parent_path());
    io::save(m, filename.string());
  }
  // Initialize manager
  TestTorrentManager manager("/NTORRENT/foo/torrent-file/sha256digest=02c737fd4c6e7de4b4825b089f39700c2dfa8fd2bb2b91f09201e357c4463253",
                             filePath);
  manager.Initialize();
  // check that initially there is no data on disk
  for (auto m : manager.fileManifests()) {
    auto fileState = manager.fileState(m.getFullName());
    BOOST_CHECK(fileState.empty());
  }
  // write all data to disk (for each file manifest)
  auto manifest_it = manifests.begin();
  for (auto& data : fileData) {
    for (auto& d : data) {
      BOOST_CHECK(manager.writeData(d));
    }
    // check that the  state is updated appropriately
    auto fileState = manager.fileState(manifest_it->getFullName());
    for (auto s : fileState) {
      BOOST_CHECK(s);
    }
    ++manifest_it;
  }
  // get the file names (ascending)
  std::set<std::string> fileNames;
  for (auto i = fs::recursive_directory_iterator(filePath + "/foo");
       i != fs::recursive_directory_iterator();
       ++i) {
    fileNames.insert(i->path().string());
  }
  // verify file by file that the data packets are written correctly
  auto f_it = fileData.begin();
  for (auto f : fileNames) {
    // read file from disk
    std::vector<uint8_t> file_bytes;
    fs::ifstream is(f, fs::ifstream::binary | fs::ifstream::in);
    is >> std::noskipws;
    std::istream_iterator<uint8_t> start(is), end;
    file_bytes.insert(file_bytes.end(), start, end);
    std::vector<uint8_t> data_bytes;
    // get content from data packets
    for (const auto& d : *f_it) {
      auto content = d.getContent();
      data_bytes.insert(data_bytes.end(), content.value_begin(), content.value_end());
    }
    BOOST_CHECK(data_bytes == file_bytes);
    ++f_it;
  }
  fs::remove_all(filePath);
  fs::remove_all(".appdata");
}

BOOST_AUTO_TEST_CASE(CheckWriteTorrentComplete)
{
  const struct {
      const char    *d_directoryPath;
      const char    *d_initialSegmentName;
      size_t         d_namesPerSegment;
      size_t         d_subManifestSize;
      size_t         d_dataPacketSize;
  } DATA [] = {
    {"tests/testdata/foo", "/NTORRENT/foo/torrent-file/sha256digest=02c737fd4c6e7de4b4825b089f39700c2dfa8fd2bb2b91f09201e357c4463253", 1024, 1024, 1024},
    {"tests/testdata/foo", "/NTORRENT/foo/torrent-file/sha256digest=96d900d6788465f9a7b00191581b004c910d74b3762d141ec0e82173731bc9f4",    1,    1, 1024},
  };
  enum { NUM_DATA = sizeof DATA / sizeof *DATA };
  for (int i = 0; i < NUM_DATA; ++i) {
    auto directoryPath      = DATA[i].d_directoryPath;
    Name initialSegmentName = DATA[i].d_initialSegmentName;
    auto namesPerSegment    = DATA[i].d_namesPerSegment;
    auto dataPacketSize     = DATA[i].d_dataPacketSize;
    auto subManifestSize    = DATA[i].d_subManifestSize;

    vector<TorrentFile>  torrentSegments;
    std::string filePath = "tests/testdata/temp";
    // get torrent files
    {
      auto temp = TorrentFile::generate(directoryPath,
                                        namesPerSegment,
                                        subManifestSize,
                                        dataPacketSize,
                                        false);
      torrentSegments = temp.first;
    }
    // Initialize manager
    TestTorrentManager manager(initialSegmentName,
                               filePath);
    manager.Initialize();
    std::string dirPath = ".appdata/foo/";
    std::string torrentPath = dirPath + "torrent_files/";
    BOOST_CHECK(manager.torrentSegments().empty());
    for (const auto& t : torrentSegments) {
      BOOST_CHECK(manager.writeTorrentSegment(t, torrentPath));
    }
    BOOST_CHECK(manager.torrentSegments() == torrentSegments);
    // check that initializing a new manager also gets all the torrent torrentSegments
    TestTorrentManager manager2(initialSegmentName,
                                filePath);
    manager2.Initialize();
    BOOST_CHECK(manager2.torrentSegments() == torrentSegments);

    // start anew
    fs::remove_all(torrentPath);
    fs::create_directories(torrentPath);
    manager.Initialize();
    BOOST_CHECK(manager.torrentSegments().empty());

    // check that there is no dependence on the order of torrent segments
    // randomize the order of the torrent segments
    auto torrentSegmentsRandom = torrentSegments;
    std::random_shuffle(torrentSegmentsRandom.begin(), torrentSegmentsRandom.end());
    for (const auto& t : torrentSegmentsRandom) {
      BOOST_CHECK(manager.writeTorrentSegment(t, torrentPath));
    }
    BOOST_CHECK(manager.torrentSegments() == torrentSegments);
    fs::remove_all(".appdata");
  }
}

BOOST_AUTO_TEST_CASE(CheckWriteManifestComplete)
{
  std::string dirPath = ".appdata/foo/";
  std::string torrentPath = dirPath + "torrent_files/";
  std::string manifestPath = dirPath + "manifests/";

  const struct {
     const char    *d_directoryPath;
     const char    *d_initialSegmentName;
     size_t         d_namesPerSegment;
     size_t         d_subManifestSize;
     size_t         d_dataPacketSize;
  } DATA [] = {
   {"tests/testdata/foo", "/NTORRENT/foo/torrent-file/sha256digest=02c737fd4c6e7de4b4825b089f39700c2dfa8fd2bb2b91f09201e357c4463253", 1024, 1024, 1024},
   {"tests/testdata/foo", "/NTORRENT/foo/torrent-file/sha256digest=02c737fd4c6e7de4b4825b089f39700c2dfa8fd2bb2b91f09201e357c4463253",     128,  128, 1024},
  };
  enum { NUM_DATA = sizeof DATA / sizeof *DATA };
  for (int i = 0; i < NUM_DATA; ++i) {
    auto directoryPath      = DATA[i].d_directoryPath;
    Name initialSegmentName = DATA[i].d_initialSegmentName;
    auto namesPerSegment    = DATA[i].d_namesPerSegment;
    auto dataPacketSize     = DATA[i].d_dataPacketSize;
    auto subManifestSize    = DATA[i].d_subManifestSize;

    vector<FileManifest> manifests;
    vector<TorrentFile>  torrentSegments;

    std::string filePath = "tests/testdata/temp";
    // get torrent files and manifests
    {
       auto temp = TorrentFile::generate(directoryPath,
                                         namesPerSegment,
                                         subManifestSize,
                                         dataPacketSize,
                                         false);
      torrentSegments = temp.first;
      auto temp1      = temp.second;
      for (const auto& ms : temp1) {
        manifests.insert(manifests.end(), ms.first.begin(), ms.first.end());
      }
    }
    TestTorrentManager manager(initialSegmentName,
                              filePath);
    manager.Initialize();
    for (const auto& t : torrentSegments) {
      manager.writeTorrentSegment(t, torrentPath);
    }

    BOOST_CHECK(manager.fileManifests().empty());
    for (const auto& m : manifests) {
      BOOST_CHECK(manager.writeFileManifest(m, manifestPath));
    }
    BOOST_CHECK(manager.fileManifests() == manifests);

    TestTorrentManager manager2(initialSegmentName,
                                filePath);

    manager2.Initialize();
    BOOST_CHECK(manager2.fileManifests() == manifests);

    // start anew
    fs::remove_all(manifestPath);
    fs::create_directories(manifestPath);
    manager.Initialize();
    BOOST_CHECK(manager.fileManifests().empty());

    // check that there is no dependence on the order of torrent segments
    // randomize the order of the torrent segments
    auto fileManifestsRandom = manifests;
    std::random_shuffle(fileManifestsRandom.begin(), fileManifestsRandom.end());
    for (const auto& m : fileManifestsRandom) {
      BOOST_CHECK(manager.writeFileManifest(m, manifestPath));
    }
    BOOST_CHECK(manager2.fileManifests() == manifests);
    fs::remove_all(".appdata");
  }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nTorrent
} // namespace ndn
