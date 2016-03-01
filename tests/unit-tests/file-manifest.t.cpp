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
#include <ndn-cxx/signature.hpp>
#include <ndn-cxx/security/key-chain.hpp>

BOOST_TEST_DONT_PRINT_LOG_VALUE(std::nullptr_t)
BOOST_TEST_DONT_PRINT_LOG_VALUE(std::vector<ndn::Name>)

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

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nTorrent
} // namespace ndn
