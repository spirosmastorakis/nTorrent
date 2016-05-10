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

#include "torrent-file.hpp"
#include "util/io-util.hpp"

#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>

#include <algorithm>

#include <boost/range/adaptors.hpp>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

namespace ndn {
namespace ntorrent {

BOOST_CONCEPT_ASSERT((WireEncodable<TorrentFile>));
BOOST_CONCEPT_ASSERT((WireDecodable<TorrentFile>));
static_assert(std::is_base_of<Data::Error, TorrentFile::Error>::value,
                "TorrentFile::Error should inherit from Data::Error");

TorrentFile::TorrentFile(const Name& torrentFileName,
                         const Name& torrentFilePtr,
                         const Name& commonPrefix,
                         const std::vector<ndn::Name>& catalog)
  : Data(torrentFileName)
  , m_commonPrefix(commonPrefix)
  , m_torrentFilePtr(torrentFilePtr)
  , m_catalog(catalog)
{
}

TorrentFile::TorrentFile(const Name& torrentFileName,
                         const Name& commonPrefix,
                         const std::vector<ndn::Name>& catalog)
  : Data(torrentFileName)
  , m_commonPrefix(commonPrefix)
  , m_catalog(catalog)
{
}

TorrentFile::TorrentFile(const Block& block)
{
  this->wireDecode(block);
}

void
TorrentFile::createSuffixCatalog()
{
  for (auto i = m_catalog.begin() ; i != m_catalog.end(); ++i) {
    m_suffixCatalog.push_back((*i).getSubName(m_commonPrefix.size()));
  }
}

shared_ptr<Name>
TorrentFile::getTorrentFilePtr() const
{
  if (this->hasTorrentFilePtr()) {
    return make_shared<Name>(m_torrentFilePtr);
  }
  return nullptr;
}

void
TorrentFile::constructLongNames()
{
  for (auto i = m_suffixCatalog.begin(); i != m_suffixCatalog.end(); ++i) {
      Name commonPrefix = m_commonPrefix;
      m_catalog.push_back(commonPrefix.append((*i)));
  }
}

template<encoding::Tag TAG>
size_t
TorrentFile::encodeContent(EncodingImpl<TAG>& encoder) const
{
  // TorrentFileContent ::= CONTENT-TYPE TLV-LENGTH
  //                      Suffix+
  //                      CommonPrefix
  //                      TorrentFilePtr?

  // Suffix ::= NAME-TYPE TLV-LENGTH
  //          Name

  // CommonPrefix ::= NAME-TYPE TLV-LENGTH
  //                Name

  // TorrentFilePtr ::= NAME-TYPE TLV-LENGTH
  //                  Name

  size_t totalLength = 0;
  for (const auto& name : m_suffixCatalog |  boost::adaptors::reversed) {
    size_t fileManifestSuffixLength = 0;
    fileManifestSuffixLength += name.wireEncode(encoder);
    totalLength += fileManifestSuffixLength;
  }
  totalLength += m_commonPrefix.wireEncode(encoder);
  if (!m_torrentFilePtr.empty()) {
    size_t torrentFilePtrLength = 0;
    torrentFilePtrLength += m_torrentFilePtr.wireEncode(encoder);
    totalLength += torrentFilePtrLength;
  }

  totalLength += encoder.prependVarNumber(totalLength);
  totalLength += encoder.prependVarNumber(tlv::Content);
  return totalLength;
}

bool
TorrentFile::erase(const Name& name)
{
  auto found = std::find(m_catalog.begin(), m_catalog.end(), name);
  if (found != m_catalog.end()) {
    m_catalog.erase(found);
    return true;
  }
  return false;
}

void
TorrentFile::encodeContent()
{
  resetWire();

  EncodingEstimator estimator;
  size_t estimatedSize = encodeContent(estimator);

  EncodingBuffer buffer(estimatedSize, 0);
  encodeContent(buffer);

  setContentType(tlv::ContentType_Blob);
  setContent(buffer.block());
}

void
TorrentFile::decodeContent()
{
  // TorrentFileContent ::= CONTENT-TYPE TLV-LENGTH
  //                      Suffix+
  //                      CommonPrefix
  //                      TorrentFilePtr?

  // Suffix ::= NAME-TYPE TLV-LENGTH
  //          Name

  // CommonPrefix ::= NAME-TYPE TLV-LENGTH
  //                Name

  // TorrentFilePtr ::= NAME-TYPE TLV-LENGTH
  //                  Name

  if (getContentType() != tlv::ContentType_Blob) {
      BOOST_THROW_EXCEPTION(Error("Expected Content Type Blob"));
  }

  const Block& content = Data::getContent();
  content.parse();

  // Check whether there is a TorrentFilePtr
  auto element = content.elements_begin();
  if (content.elements_end() == element) {
    BOOST_THROW_EXCEPTION(Error("Torrent-file with empty content"));
  }
  element->parse();
  Name name(*element);
  if (name.empty())
    BOOST_THROW_EXCEPTION(Error("Empty name included in the torrent-file"));

  if (name.get(name.size() - 3) == name::Component("torrent-file")) {
    m_torrentFilePtr = name;
    ++element;
    m_commonPrefix = Name(*element);
    if (m_commonPrefix.empty()) {
      BOOST_THROW_EXCEPTION(Error("Common prefix cannot be empty"));
    }
  }
  else {
    m_commonPrefix = name;
  }
  element++;
  for (; element != content.elements_end(); ++element) {
    element->parse();
    Name fileManifestSuffix(*element);
    if (fileManifestSuffix.empty())
      BOOST_THROW_EXCEPTION(Error("Empty manifest file name included in the torrent-file"));
    this->insertToSuffixCatalog(fileManifestSuffix);
  }
  if (m_suffixCatalog.size() == 0) {
    BOOST_THROW_EXCEPTION(Error("Torrent-file with empty catalog of file manifest names"));
  }
}

void
TorrentFile::wireDecode(const Block& wire)
{
  m_catalog.clear();
  m_suffixCatalog.clear();
  Data::wireDecode(wire);
  this->decodeContent();
  this->constructLongNames();
}

void
TorrentFile::finalize()
{
  this->createSuffixCatalog();
  this->encodeContent();
  m_suffixCatalog.clear();
}

std::pair<std::vector<TorrentFile>,
          std::vector<std::pair<std::vector<FileManifest>,
                                std::vector<Data>>>>
TorrentFile::generate(const std::string& directoryPath,
                      size_t namesPerSegment,
                      size_t subManifestSize,
                      size_t dataPacketSize,
                      bool returnData)
{
  //TODO(spyros) Adapt this support subdirectories in 'directoryPath'
  BOOST_ASSERT(0 < namesPerSegment);

  std::vector<TorrentFile> torrentSegments;

  fs::path path(directoryPath);
  if (!Io::exists(path)) {
    BOOST_THROW_EXCEPTION(Error(directoryPath + ": no such directory."));
  }

  Name directoryPathName(directoryPath);
  Io::recursive_directory_iterator directoryPtr(fs::system_complete(directoryPath).string());

  std::string prefix = std::string(SharedConstants::commonPrefix) + "/NTORRENT";
  Name commonPrefix(prefix +
                    directoryPathName.getSubName(directoryPathName.size() - 1).toUri());

  Name torrentName(commonPrefix.toUri() + "/torrent-file");
  TorrentFile currentTorrentFile(torrentName, commonPrefix, {});
  std::vector<std::pair<std::vector<FileManifest>, std::vector<Data>>> manifestPairs;
  // sort all the file names lexicographically
  std::set<std::string> fileNames;
  for (auto i = directoryPtr; i != Io::recursive_directory_iterator(); ++i) {
    fileNames.insert(i->path().string());
  }
  size_t manifestFileCounter = 0u;
  for (const auto& fileName : fileNames) {
    Name manifestPrefix(prefix +
                        directoryPathName.getSubName(directoryPathName.size() - 1).toUri());
    std::pair<std::vector<FileManifest>, std::vector<Data>> currentManifestPair =
                                                    FileManifest::generate(fileName,
                                                    manifestPrefix, subManifestSize,
                                                    dataPacketSize, returnData);

    if (manifestFileCounter != 0 && 0 == manifestFileCounter % namesPerSegment) {
      torrentSegments.push_back(currentTorrentFile);
      Name currentTorrentName = torrentName;
      currentTorrentName.appendSequenceNumber(static_cast<int>(manifestFileCounter));
      currentTorrentFile = TorrentFile(currentTorrentName, commonPrefix, {});
    }
    currentTorrentFile.insert(currentManifestPair.first[0].getFullName());
    currentManifestPair.first.shrink_to_fit();
    currentManifestPair.second.shrink_to_fit();
    manifestPairs.push_back(currentManifestPair);
    ++manifestFileCounter;
  }

  // Sign and append the last torrent-file
  security::KeyChain keyChain;
  currentTorrentFile.finalize();
  keyChain.sign(currentTorrentFile, signingWithSha256());
  torrentSegments.push_back(currentTorrentFile);

  for (auto it = torrentSegments.rbegin() + 1; it != torrentSegments.rend(); ++it) {
    auto next = it - 1;
    it->setTorrentFilePtr(next->getFullName());
    it->finalize();
    keyChain.sign(*it, signingWithSha256());
  }

  torrentSegments.shrink_to_fit();
  manifestPairs.shrink_to_fit();
  return std::make_pair(torrentSegments, manifestPairs);
}

} // namespace ntorrent

} // namespace ndn
