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

#include <limits>

#include <boost/assert.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/irange.hpp>
#include <boost/throw_exception.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>

#include <ndn-cxx/encoding/tlv.hpp>

using std::vector;
using std::streamsize;
using boost::irange;

namespace fs = boost::filesystem;

namespace ndn {
namespace ntorrent {

BOOST_CONCEPT_ASSERT((boost::EqualityComparable<FileManifest>));
BOOST_CONCEPT_ASSERT((WireEncodable<FileManifest>));
BOOST_CONCEPT_ASSERT((WireDecodable<FileManifest>));
static_assert(std::is_base_of<Data::Error, FileManifest::Error>::value,
                "FileManifest::Error should inherit from Data::Error");

static ndn::Name
get_name_of_manifest(const std::string& filePath, const Name& manifestPrefix)
{
  Name full_path(fs::system_complete(filePath).string());
  // Search the filePath for the leading component that matches
  auto name_component_iter = std::find(full_path.rbegin(),
                                       full_path.rend(),
                                       *manifestPrefix.rbegin());

  if (full_path.rend() == name_component_iter) {
    BOOST_THROW_EXCEPTION(FileManifest::Error("No matching name component between" +
                                              manifestPrefix.toUri() + " and "     +
                                              full_path.toUri()));
  }
  ndn::Name manifestName;
  // Rebuild the name to be the suffix from the matching component
  for (auto it = (name_component_iter.base() - 1); full_path.end() != it; ++it) {
    manifestName.append(*it);
  }
  return manifestName;
}

static std::vector<Data>
packetize_file(const fs::path& filePath,
               const ndn::Name& commonPrefix,
               size_t dataPacketSize,
               size_t subManifestSize,
               size_t subManifestNum)
{
  BOOST_ASSERT(0 < dataPacketSize);
  size_t APPROX_BUFFER_SIZE = std::numeric_limits<int>::max(); // 2 * 1024 * 1024 *1024
  auto file_size = fs::file_size(filePath);
  auto start_offset = subManifestNum * subManifestSize * dataPacketSize;
  // determine the number of bytes in this submanifest
  auto subManifestLength = subManifestSize * dataPacketSize;
  auto remainingFileLength = file_size - start_offset;
  subManifestLength = remainingFileLength < subManifestLength
                    ? remainingFileLength
                    : subManifestLength;
  std::vector<Data> packets;
  packets.reserve(subManifestLength/dataPacketSize + 1);
  fs::ifstream fs(filePath, fs::ifstream::binary);
  if (!fs) {
    BOOST_THROW_EXCEPTION(FileManifest::Error("IO Error when opening" + filePath.string()));
  }
  // ensure that buffer is large enough to contain whole packets
  // buffer size is either the entire file or the smallest number of data packets >= 2 GB
  auto buffer_size =
    subManifestLength < APPROX_BUFFER_SIZE   ?
    subManifestLength                        :
    APPROX_BUFFER_SIZE % dataPacketSize == 0 ?
    APPROX_BUFFER_SIZE :
    APPROX_BUFFER_SIZE + dataPacketSize - (APPROX_BUFFER_SIZE % dataPacketSize);
  std::vector<char> file_bytes;
  file_bytes.reserve(buffer_size);
  size_t bytes_read = 0;
  fs.seekg(start_offset);
  while(fs && bytes_read < subManifestLength && !fs.eof()) {
    // read the file into the buffer
    fs.read(&file_bytes.front(), buffer_size);
    auto read_size = fs.gcount();
    if (fs.bad() || read_size < 0) {

      BOOST_THROW_EXCEPTION(FileManifest::Error("IO Error when reading" + filePath.string()));
    }
    bytes_read += read_size;
    char *curr_start = &file_bytes.front();
    for (size_t i = 0u; i < buffer_size; i += dataPacketSize) {
      // Build a packet from the data
      Name packetName = commonPrefix;
      packetName.appendSequenceNumber(packets.size());
      Data d(packetName);
      auto content_length = i + dataPacketSize > buffer_size ? buffer_size - i : dataPacketSize;
      d.setContent(encoding::makeBinaryBlock(tlv::Content, curr_start, content_length));
      curr_start += content_length;
      // append to the collection
      packets.push_back(d);
    }
    file_bytes.clear();
    // recompute the buffer_size
    buffer_size =
      subManifestLength - bytes_read < APPROX_BUFFER_SIZE ?
      subManifestLength - bytes_read                      :
      APPROX_BUFFER_SIZE % dataPacketSize == 0            ?
      APPROX_BUFFER_SIZE                                  :
      APPROX_BUFFER_SIZE + dataPacketSize - (APPROX_BUFFER_SIZE % dataPacketSize);
  }
  fs.close();
  packets.shrink_to_fit();
  security::KeyChain key_chain;
  // sign all the packets
  for (auto& p : packets) {
    key_chain.sign(p, signingWithSha256());
  }
  return packets;
}

// CLASS METHODS
std::pair<std::vector<FileManifest>, std::vector<Data>>
FileManifest::generate(const std::string& filePath,
                       const Name&        manifestPrefix,
                       size_t             subManifestSize,
                       size_t             dataPacketSize,
                       bool               returnData)
{
  BOOST_ASSERT(0 < subManifestSize);
  BOOST_ASSERT(0 < dataPacketSize);
  std::vector<FileManifest> manifests;
  fs::path path(filePath);
  if (!fs::exists(path)) {
    BOOST_THROW_EXCEPTION(Error(filePath + ": no such file."));
  }
  size_t file_length = fs::file_size(filePath);
  // If the file_length is not evenly divisible by subManifestSize add 1, otherwise 0
  size_t numSubManifests = file_length / (subManifestSize * dataPacketSize) +
                              !!(file_length % (subManifestSize * dataPacketSize));
  // Find the prefix for the Catalog
  auto manifestName = get_name_of_manifest(filePath, manifestPrefix);
  std::vector<Data> allPackets;
  if (returnData) {
    allPackets.reserve(numSubManifests * subManifestSize);
  }
  manifests.reserve(numSubManifests);
  for (auto subManifestNum : irange<size_t>(0, numSubManifests)) {
    auto curr_manifest_name = manifestName;
    curr_manifest_name.append("manifest" + boost::lexical_cast<std::string>(subManifestNum));
    // append the packet number
    curr_manifest_name.appendSequenceNumber(manifests.size());
    auto subManifestPrefix = manifestPrefix;
    subManifestPrefix.appendNumber(subManifestNum);
    FileManifest curr_manifest(curr_manifest_name, dataPacketSize, manifestPrefix);
    auto packets = packetize_file(path,
                                  subManifestPrefix,
                                  dataPacketSize,
                                  subManifestSize,
                                  subManifestNum);
    if (returnData) {
      allPackets.insert(allPackets.end(), packets.begin(), packets.end());
    }
    curr_manifest.reserve(packets.size());
    // Collect all the Data packets into the sub-manifests
    for (const auto& p: packets) {
      curr_manifest.push_back(p.getFullName());
    }
    // append the last manifest
    manifests.push_back(curr_manifest);
  }
  allPackets.shrink_to_fit();
  manifests.shrink_to_fit();
  // Set all the submanifest_ptrs and sign all the manifests
  security::KeyChain key_chain;
  manifests.back().finalize();
  key_chain.sign(manifests.back(), signingWithSha256());
  for (auto it = manifests.rbegin() + 1; it != manifests.rend(); ++it) {
    auto next = it - 1;
    it->set_submanifest_ptr(std::make_shared<Name>(next->getFullName()));
    it->finalize();
    key_chain.sign(*it, signingWithSha256());
  }
  return {manifests, allPackets};
}

void
FileManifest::wireDecode(const Block& wire)
{
  Data::wireDecode(wire);
  this->decodeContent();
}

template<ndn::encoding::Tag TAG>
size_t
FileManifest::encodeContent(ndn::EncodingImpl<TAG>& encoder) const {
  // ManifestContent ::= CONTENT-TYPE TLV-LENGTH
  //                 DataPacketName*
  //                 CatalogPrefix
  //                 DataPacketSize
  //                 FileManifestPtr?

  // DataPacketName ::= NAME-TYPE TLV-LENGTH
  //                Name

  // CatalogPrefix ::= NAME-TYPE TLV-LENGTH
  //               Name

    // DataPacketSize ::= CONTENT-TYPE TLV-LENGTH
  //                nonNegativeInteger

  // FileManifestPtr ::= NAME-TYPE TLV-LENGTH
  //                 Name

  size_t totalLength = 0;

  // build suffix catalog
  vector<Name> suffixCatalog;
  suffixCatalog.reserve(m_catalog.size());
  for (auto name: m_catalog) {
    if (!m_catalogPrefix.isPrefixOf(name)) {
      BOOST_THROW_EXCEPTION(Error(name.toUri() + " does not have the prefix "
                                               + m_catalogPrefix.toUri()));
    }
    name = name.getSubName(m_catalogPrefix.size());
    if (name.empty()) {
      BOOST_THROW_EXCEPTION(Error("Manifest cannot include empty string"));
    }
    suffixCatalog.push_back(name);
  }

  for (const auto& name : suffixCatalog |  boost::adaptors::reversed) {
    totalLength += name.wireEncode(encoder);
  }

  totalLength += m_catalogPrefix.wireEncode(encoder);

  totalLength += prependNonNegativeIntegerBlock(encoder, tlv::Content, m_dataPacketSize);

  if (nullptr != m_submanifestPtr) {
    totalLength += m_submanifestPtr->wireEncode(encoder);
  }

  totalLength += encoder.prependVarNumber(totalLength);
  totalLength += encoder.prependVarNumber(tlv::Content);
  return totalLength;

}

// MANIPULATORS
void
FileManifest::push_back(const Name& name)
{
  BOOST_ASSERT(name != m_catalogPrefix);
  BOOST_ASSERT(m_catalogPrefix.isPrefixOf(name));
  // TODO(msweatt) Change this to use the copy constructor once bug fixed in Name()
  m_catalog.push_back(name.toUri());
}

bool
FileManifest::remove(const ndn::Name& name) {
  const auto it = std::find(m_catalog.begin(), m_catalog.end(), name);
  if (m_catalog.end() == it) {
    return false;
  }
  m_catalog.erase(it);
  return true;
}

void
FileManifest::finalize() {
  m_catalog.shrink_to_fit();
  encodeContent();
}

void FileManifest::encodeContent() {
  // Name
  //     <file_name>/ImplicitDigest
  // Content
  //     MetaData
  //     DataPtr*
  //     ManifestPointer?
  //
  // DataPtr := HashValue
  // ManifestPtr := HashValue
  // HashValue   := OCTET[32]

  // MetaData    := Property*
  // Property    := DataSize | Signature
  onChanged();

  EncodingEstimator estimator;
  size_t estimatedSize = encodeContent(estimator);

  EncodingBuffer buffer(estimatedSize, 0);
  encodeContent(buffer);

  setContentType(tlv::ContentType_Blob);
  setContent(buffer.block());
}

void
FileManifest::decodeContent() {
  // ManifestContent ::= CONTENT-TYPE TLV-LENGTH
  //                 DataPacketName*
  //                 CatalogPrefix
  //                 DataPacketSize
  //                 FileManifestPtr?


  // DataPacketName ::= NAME-TYPE TLV-LENGTH
  //                Name

  // CatalogPrefix ::= NAME-TYPE TLV-LENGTH
  //               Name

  // DataPacketSize ::= CONTENT-TYPE TLV-LENGTH
  //                nonNegativeInteger

  // FileManifestPtr ::= NAME-TYPE TLV-LENGTH
  //                 Name

  if (getContentType() != tlv::ContentType_Blob) {
    BOOST_THROW_EXCEPTION(Error("Expected Content Type Blob"));
  }

  const Block& content = Data::getContent();
  content.parse();

  auto element = content.elements_begin();
  if (content.elements_end() == element) {
    BOOST_THROW_EXCEPTION(Error("FileManifest with empty content"));
  }
  if (element->type() == tlv::Name) {
    Name name(*element);
    m_submanifestPtr = std::make_shared<Name>(name);
    ++element;
  }

  // DataPacketSize
  m_dataPacketSize = readNonNegativeInteger(*element);
  ++element;
  // CatalogPrefix
  m_catalogPrefix = Name(*element);
  ++element;
  // Catalog
  m_catalog.clear();
  for (; element != content.elements_end(); ++element) {
    element->parse();
    Name name = m_catalogPrefix;
    name.append(Name(*element));
    if (name == m_catalogPrefix) {
      BOOST_THROW_EXCEPTION(Error("Empty name included in a FileManifest"));
    }
    push_back(name);
  }
}

bool operator==(const FileManifest& lhs, const FileManifest& rhs) {
  return lhs.name()             == rhs.name()
      && lhs.data_packet_size() == rhs.data_packet_size()
      && (lhs.submanifest_ptr() == rhs.submanifest_ptr() /* shallow  equality */
         || ( nullptr != lhs.submanifest_ptr()
           && nullptr != rhs.submanifest_ptr()
           && *rhs.submanifest_ptr() == *lhs.submanifest_ptr()
         )
      )
      && lhs.catalog()          == rhs.catalog();
}

bool operator!=(const FileManifest& lhs, const FileManifest& rhs) {
  return lhs.name()             != rhs.name()
      || lhs.data_packet_size() != rhs.data_packet_size()
      || (lhs.submanifest_ptr() != rhs.submanifest_ptr() /* shallow  equality */
         && (nullptr == lhs.submanifest_ptr()
         || nullptr == rhs.submanifest_ptr()
         || *rhs.submanifest_ptr() != *lhs.submanifest_ptr()
        )
      )
      || lhs.catalog()          != rhs.catalog();
}

}  // end ntorrent
}  // end ndn