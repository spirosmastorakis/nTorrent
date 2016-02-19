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

#include "file-manifest.hpp"
#include "boost-test.hpp"

#include <vector>

#include <ndn-cxx/data.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>
#include <ndn-cxx/signature.hpp>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/lexical_cast.hpp>

namespace fs = boost::filesystem;

BOOST_TEST_DONT_PRINT_LOG_VALUE(std::nullptr_t)
BOOST_TEST_DONT_PRINT_LOG_VALUE(std::vector<ndn::Name>)
BOOST_TEST_DONT_PRINT_LOG_VALUE(std::vector<ndn::ntorrent::FileManifest>)
BOOST_TEST_DONT_PRINT_LOG_VALUE(std::vector<uint8_t>)
BOOST_TEST_DONT_PRINT_LOG_VALUE(std::vector<ndn::Data>::iterator)

namespace ndn {
namespace ntorrent {
namespace tests {

using std::vector;
using ndn::Name;

BOOST_AUTO_TEST_SUITE(TestFileManifest)

BOOST_AUTO_TEST_CASE(CheckPrimaryAccessorsAndManipulators)
{
  FileManifest m1("/file0/1A2B3C4D", 256, "/foo/");

  // Check the values on most simply constructed m1
  BOOST_CHECK_EQUAL("/file0/1A2B3C4D", m1.name());
  BOOST_CHECK_EQUAL(256, m1.data_packet_size());
  BOOST_CHECK_EQUAL(nullptr, m1.submanifest_ptr());
  BOOST_CHECK_EQUAL("/foo/", m1.catalog_prefix());
  BOOST_CHECK_EQUAL(vector<Name>({}), m1.catalog());

  // Append names to catalog and recheck all salient attributes
  m1.push_back("/foo/0/ABC123");
  m1.push_back("/foo/1/DEADBEFF");
  m1.push_back("/foo/2/CAFEBABE");

  BOOST_CHECK_EQUAL("/file0/1A2B3C4D", m1.name());
  BOOST_CHECK_EQUAL(256, m1.data_packet_size());
  BOOST_CHECK_EQUAL(nullptr, m1.submanifest_ptr());
  BOOST_CHECK_EQUAL("/foo/", m1.catalog_prefix());
  BOOST_CHECK_EQUAL(vector<Name>({"/foo/0/ABC123", "/foo/1/DEADBEFF", "/foo/2/CAFEBABE"}),
                    m1.catalog());

  // Remove a value from the catalog and recheck all salient attributes
  BOOST_CHECK_EQUAL(true, m1.remove("/foo/0/ABC123"));

  BOOST_CHECK_EQUAL("/file0/1A2B3C4D", m1.name());
  BOOST_CHECK_EQUAL(256, m1.data_packet_size());
  BOOST_CHECK_EQUAL(nullptr, m1.submanifest_ptr());
  BOOST_CHECK_EQUAL("/foo/", m1.catalog_prefix());
  BOOST_CHECK_EQUAL(vector<Name>({"/foo/1/DEADBEFF", "/foo/2/CAFEBABE"}), m1.catalog());

  // Try to remove a value no longer in the catalog, and recheck that all salient attributes
  BOOST_CHECK_EQUAL(false, m1.remove("/foo/0/ABC123"));

  BOOST_CHECK_EQUAL("/file0/1A2B3C4D", m1.name());
  BOOST_CHECK_EQUAL(256, m1.data_packet_size());
  BOOST_CHECK_EQUAL(nullptr, m1.submanifest_ptr());
  BOOST_CHECK_EQUAL("/foo/", m1.catalog_prefix());
  BOOST_CHECK_EQUAL(vector<Name>({"/foo/1/DEADBEFF", "/foo/2/CAFEBABE"}), m1.catalog());

  // Try to remove a value never in the catalog, and recheck that all salient attributes
  BOOST_CHECK_EQUAL(false, m1.remove("/bar/0/ABC123"));
  BOOST_CHECK_EQUAL("/file0/1A2B3C4D", m1.name());
  BOOST_CHECK_EQUAL(256, m1.data_packet_size());
  BOOST_CHECK_EQUAL(nullptr, m1.submanifest_ptr());
  BOOST_CHECK_EQUAL("/foo/", m1.catalog_prefix());
  BOOST_CHECK_EQUAL(vector<Name>({"/foo/1/DEADBEFF", "/foo/2/CAFEBABE"}),
                    m1.catalog());

  // Remove a value from the end of the list
  BOOST_CHECK_EQUAL(true, m1.remove("/foo/2/CAFEBABE"));

  BOOST_CHECK_EQUAL("/file0/1A2B3C4D", m1.name());
  BOOST_CHECK_EQUAL(256, m1.data_packet_size());
  BOOST_CHECK_EQUAL(nullptr, m1.submanifest_ptr());
  BOOST_CHECK_EQUAL("/foo/", m1.catalog_prefix());
  BOOST_CHECK_EQUAL(vector<Name>({"/foo/1/DEADBEFF"}),
                    m1.catalog());
}

BOOST_AUTO_TEST_CASE(CheckValueCtors)
{
  FileManifest m1("/file0/1A2B3C4D",
                  256,
                  "/foo/",
                  {"/foo/0/ABC123",  "/foo/1/DEADBEFF", "/foo/2/CAFEBABE"},
                  std::make_shared<Name>("/file0/1/5E6F7G8H"));

  BOOST_CHECK_EQUAL("/file0/1A2B3C4D", m1.name());
  BOOST_CHECK_EQUAL(256, m1.data_packet_size());
  BOOST_CHECK_EQUAL("/file0/1/5E6F7G8H", *m1.submanifest_ptr());
  BOOST_CHECK_EQUAL("/foo/", m1.catalog_prefix());
  BOOST_CHECK_EQUAL(vector<Name>({"/foo/0/ABC123", "/foo/1/DEADBEFF", "/foo/2/CAFEBABE"}),
                    m1.catalog());

  // Remove a value from the catalog and recheck all salient attributes
  BOOST_CHECK_EQUAL(true, m1.remove("/foo/0/ABC123"));

  BOOST_CHECK_EQUAL("/file0/1A2B3C4D", m1.name());
  BOOST_CHECK_EQUAL(256, m1.data_packet_size());
  BOOST_CHECK_EQUAL("/file0/1/5E6F7G8H", *m1.submanifest_ptr());
  BOOST_CHECK_EQUAL("/foo/", m1.catalog_prefix());
  BOOST_CHECK_EQUAL(vector<Name>({"/foo/1/DEADBEFF", "/foo/2/CAFEBABE"}), m1.catalog());

  // Try to remove a value no longer in the catalog, and recheck that all salient attributes
  BOOST_CHECK_EQUAL(false, m1.remove("/foo/0/ABC123"));

  BOOST_CHECK_EQUAL("/file0/1A2B3C4D", m1.name());
  BOOST_CHECK_EQUAL(256, m1.data_packet_size());
  BOOST_CHECK_EQUAL("/file0/1/5E6F7G8H", *m1.submanifest_ptr());
  BOOST_CHECK_EQUAL("/foo/", m1.catalog_prefix());
  BOOST_CHECK_EQUAL(vector<Name>({"/foo/1/DEADBEFF", "/foo/2/CAFEBABE"}), m1.catalog());

  // Try to remove a value never in the catalog, and recheck that all salient attributes
  BOOST_CHECK_EQUAL(false, m1.remove("/bar/0/ABC123"));
  BOOST_CHECK_EQUAL("/file0/1A2B3C4D", m1.name());
  BOOST_CHECK_EQUAL(256, m1.data_packet_size());
  BOOST_CHECK_EQUAL("/file0/1/5E6F7G8H", *m1.submanifest_ptr());
  BOOST_CHECK_EQUAL("/foo/", m1.catalog_prefix());
  BOOST_CHECK_EQUAL(vector<Name>({"/foo/1/DEADBEFF", "/foo/2/CAFEBABE"}), m1.catalog());

  // Remove a value from the end of the list
  BOOST_CHECK_EQUAL(true, m1.remove("/foo/2/CAFEBABE"));

  BOOST_CHECK_EQUAL("/file0/1A2B3C4D", m1.name());
  BOOST_CHECK_EQUAL(256, m1.data_packet_size());
  BOOST_CHECK_EQUAL("/file0/1/5E6F7G8H", *m1.submanifest_ptr());
  BOOST_CHECK_EQUAL("/foo/", m1.catalog_prefix());
  BOOST_CHECK_EQUAL(vector<Name>({"/foo/1/DEADBEFF"}), m1.catalog());
}

BOOST_AUTO_TEST_CASE(CheckAssignmentOperatorEqaulityAndCopyCtor)
{
  // Construct two manifests with the same attributes, and check that they related equal
  {
    FileManifest m1("/file0/1A2B3C4D", 256, "/foo/", vector<Name>({}), std::make_shared<Name>("/file0/1/5E6F7G8H"));
    FileManifest m2("/file0/1A2B3C4D", 256, "/foo/", vector<Name>({}), std::make_shared<Name>("/file0/1/5E6F7G8H"));

    BOOST_CHECK_EQUAL(m1, m2);
    BOOST_CHECK(!(m1 != m2));

    // Change value of one
    m1.push_back("/foo/0/ABC123");
    BOOST_CHECK_NE(m1, m2);
    BOOST_CHECK(!(m1 == m2));

    // Update other
    m2.push_back("/foo/0/ABC123");
    BOOST_CHECK_EQUAL(m1, m2);
    BOOST_CHECK(!(m1 != m2));

    // Change value again
    m1.remove("/foo/0/ABC123");
    BOOST_CHECK_NE(m1, m2);
    BOOST_CHECK(!(m1 == m2));

    m2.remove("/foo/0/ABC123");
    BOOST_CHECK_EQUAL(m1, m2);
    BOOST_CHECK(!(m1 != m2));
  }

  // Set sub-manifest pointer in one and not the other
  {
    FileManifest m1("/file0/1A2B3C4D",
                    256,
                    "/foo/",
                    vector<Name>({}),
                    std::make_shared<Name>("/file0/1/5E6F7G8H"));

    FileManifest m2("/file0/1A2B3C4D", 256, "/foo/");
    BOOST_CHECK_NE(m1, m2);
    BOOST_CHECK(!(m1 == m2));

    std::swap(m1, m2);
    BOOST_CHECK_NE(m1, m2);
    BOOST_CHECK(!(m1 == m2));
  }

  // Construct two manifests using copy ctor for one
  {
    FileManifest m1("/file0/1A2B3C4D", 256, "/foo/");
    FileManifest m2(m1);

    BOOST_CHECK_EQUAL(m1, m2);
    BOOST_CHECK(!(m1 != m2));

    // Change value of one
    m1.push_back("/foo/0/ABC123");
    BOOST_CHECK_NE(m1, m2);
    BOOST_CHECK(!(m1 == m2));

    // Update other
    m2.push_back("/foo/0/ABC123");
    BOOST_CHECK_EQUAL(m1, m2);
    BOOST_CHECK(!(m1 != m2));

    // Change value again
    m1.remove("/foo/0/ABC123");
    BOOST_CHECK_NE(m1, m2);
    BOOST_CHECK(!(m1 == m2));

    m2.remove("/foo/0/ABC123");
    BOOST_CHECK_EQUAL(m1, m2);
    BOOST_CHECK(!(m1 != m2));
  }

  // Use assignment operator
  {
    FileManifest m1("/file1/1A2B3C4D", 256, "/foo/");
    FileManifest m2("/file1/5E6F7G8H", 256, "/foo/");

    BOOST_CHECK_NE(m1, m2);
    BOOST_CHECK(!(m1 == m2));

    m2 = m1;
    BOOST_CHECK_EQUAL(m1, m2);
    BOOST_CHECK(!(m1 != m2));

    // Change value of one
    m1.push_back("/foo/0/ABC123");
    BOOST_CHECK_NE(m1, m2);
    BOOST_CHECK(!(m1 == m2));

    // Update other
    m2.push_back("/foo/0/ABC123");
    BOOST_CHECK_EQUAL(m1, m2);
    BOOST_CHECK(!(m1 != m2));

    // Change value again
    m1.remove("/foo/0/ABC123");
    BOOST_CHECK_NE(m1, m2);
    BOOST_CHECK(!(m1 == m2));

    m2.remove("/foo/0/ABC123");
    BOOST_CHECK_EQUAL(m1, m2);
    BOOST_CHECK(!(m1 != m2));
  }
}

BOOST_AUTO_TEST_CASE(CheckEncodeDecode)
{
  // Construct new FileManifest from  wire encoding of another FileManifest
  FileManifest m1("/file0/1A2B3C4D",
                  256,
                  "/foo/",
                  {"/foo/0/ABC123",  "/foo/1/DEADBEFF", "/foo/2/CAFEBABE"},
                  std::make_shared<Name>("/file0/1/5E6F7G8H"));

  KeyChain keyChain;
  m1.finalize();
  keyChain.sign(m1);
  BOOST_CHECK_EQUAL(m1, FileManifest(m1.wireEncode()));

  // Change value and be sure that wireEncoding still works
  m1.remove("/foo/2/CAFEBABE");
  m1.finalize();
  keyChain.sign(m1);
  BOOST_CHECK_EQUAL(m1, FileManifest(m1.wireEncode()));

  // Explicitly call wireEncode and ensure the value works
  FileManifest m2 = m1;
  m1.remove("/foo/0/ABC123");
  keyChain.sign(m1);
  m1.finalize();
  m2.wireDecode(m1.wireEncode());
  BOOST_CHECK_EQUAL(m1, m2);
}

BOOST_AUTO_TEST_CASE(CheckGenerateFileManifest)
{
  const size_t TEST_FILE_LEN = fs::file_size("tests/testdata/foo/bar.txt");
  const struct {
      size_t         d_dataPacketSize;
      size_t         d_subManifestSize;
      const char    *d_filePath;
      const char    *d_catalogPrefix;
      bool           d_getData;
      bool           d_shouldThrow;
  } DATA [] = {
    // Affirmative tests
    {1            , TEST_FILE_LEN, "tests/testdata/foo/bar.txt" , "/NTORRENT/foo/", true, false },
    {10           , 10           , "tests/testdata/foo/bar.txt" , "/NTORRENT/foo/", true, false },
    {10           , 1            , "tests/testdata/foo/bar.txt" , "/NTORRENT/foo/", true, false },
    {1            , 10           , "tests/testdata/foo/bar.txt" , "/NTORRENT/foo/", true, false },
    {1            , 1            , "tests/testdata/foo/bar.txt" , "/NTORRENT/foo/", true, false },
    {1024         , 1            , "tests/testdata/foo/bar1.txt", "/NTORRENT/foo/", true, false },
    {1024         , 100          , "tests/testdata/foo/bar1.txt", "/NTORRENT/foo/", true, false },
    {TEST_FILE_LEN, 1            , "tests/testdata/foo/bar.txt" , "/NTORRENT/foo/", true, false },
      // Negative tests
      //   non-existent file
    {128          , 128          , "tests/testdata/foo/fake.txt", "/NTORRENT/foo/", false, true },
      // prefix mismatch
    {128          , 128          , "tests/testdata/foo/bar.txt", "/NTORRENT/bar/",  false, true },
    //  scaling test
    // {10240         , 5120 ,         "tests/testdata/foo/huge_file", "/NTORRENT/foo/", false, false },
      // assertion failures (tests not supported on platforms)
    // {0            , 128          , "tests/testdata/foo/bar.txt", "/NTORRENT/bar/", true },
    // {128          , 0            , "tests/testdata/foo/bar.txt", "/NTORRENT/bar/", true },
  };
  enum { NUM_DATA = sizeof DATA / sizeof *DATA };
  for (int i = 0; i < NUM_DATA; ++i) {
    auto dataPacketSize  = DATA[i].d_dataPacketSize;
    auto subManifestSize = DATA[i].d_subManifestSize;
    auto filePath        = DATA[i].d_filePath;
    auto catalogPrefix   = DATA[i].d_catalogPrefix;
    auto getData         = DATA[i].d_getData;
    auto shouldThrow     = DATA[i].d_shouldThrow;

    std::pair<std::vector<FileManifest>, std::vector<Data>> manifestsDataPair;
    if (shouldThrow) {
      BOOST_CHECK_THROW(FileManifest::generate(filePath,
                                               catalogPrefix,
                                               subManifestSize,
                                               dataPacketSize,
                                               getData),
                          FileManifest::Error);
    }
    else {
      manifestsDataPair = FileManifest::generate(filePath,
                                                 catalogPrefix,
                                                 subManifestSize,
                                                 dataPacketSize,
                                                 getData);
      auto manifests = manifestsDataPair.first;
      auto data = manifestsDataPair.second;
      // Verify the basic attributes of the manifest
      size_t file_length = 0;
      for (auto it = manifests.begin(); it != manifests.end(); ++it) {
        // Verify that each file manifest is signed.
        BOOST_CHECK_NO_THROW(it->getFullName());
        BOOST_CHECK_EQUAL(it->data_packet_size(), dataPacketSize);
        BOOST_CHECK_EQUAL(it->catalog_prefix(), catalogPrefix);
        if (it != manifests.end() -1) {
          BOOST_CHECK_EQUAL(it->catalog().size(), subManifestSize);
          BOOST_CHECK_EQUAL(*(it->submanifest_ptr()), (it+1)->getFullName());
        }
        else {
          BOOST_CHECK_LE(it->catalog().size(), subManifestSize);
          BOOST_CHECK_EQUAL(it->submanifest_ptr(), nullptr);
          file_length += it->wireEncode().value_size();
        }
      }
      // confirm the manifests catalogs includes exactly the data packets
      if (getData) {
        auto data_it = data.begin();
        size_t total_manifest_length = 0;
        for (auto& m : manifests) {
          total_manifest_length += m.wireEncode().value_size();
          for (auto& data_name : m.catalog()) {
            BOOST_CHECK_EQUAL(data_name, data_it->getFullName());
            data_it++;
          }
        }
        BOOST_CHECK_EQUAL(data_it, data.end());
        std::vector<uint8_t> data_bytes;
        data_bytes.reserve(fs::file_size(filePath));
        for (const auto& d : data) {
          auto content = d.getContent();
          data_bytes.insert(data_bytes.end(), content.value_begin(), content.value_end());
        }
        data_bytes.shrink_to_fit();
        // load  the contents of the file from disk
        fs::ifstream is(filePath, fs::ifstream::binary | fs::ifstream::in);
        is >> std::noskipws;
        std::istream_iterator<uint8_t> start(is), end;
        std::vector<uint8_t> file_bytes;
        file_bytes.reserve(fs::file_size(filePath));
        file_bytes.insert(file_bytes.begin(), start, end);
        file_bytes.shrink_to_fit();
        BOOST_CHECK_EQUAL(file_bytes, data_bytes);
      }
      else {
        BOOST_CHECK(data.empty());
        BOOST_CHECK(data.capacity() == 0);
      }
    }
  }
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nTorrent
} // namespace ndn
