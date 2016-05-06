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

#include "update-handler.hpp"
#include "util/logging.hpp"

#include <ndn-cxx/security/signing-helpers.hpp>

namespace ndn {
namespace ntorrent {

void
UpdateHandler::sendAliveInterest(StatsTable::iterator iter)
{
  Name prependedComponents(SharedConstants::commonPrefix);
  Name interestName = Name(prependedComponents.toUri() + "/NTORRENT" + m_torrentName.toUri() +
                           "/ALIVE" + m_ownRoutablePrefix.toUri());

  shared_ptr<Interest> i = make_shared<Interest>(interestName);

  // Create and set the forwarding hint
  Delegation del;
  del.preference = 1;
  del.name = iter->getRecordName();

  DelegationList list({del});

  i->setForwardingHint(list);

  LOG_DEBUG << "Sending ALIVE Interest: " << i << std::endl;

  m_face->expressInterest(*i, bind(&UpdateHandler::decodeDataPacketContent, this, _1, _2),
                          bind(&UpdateHandler::tryNextRoutablePrefix, this, _1),
                          bind(&UpdateHandler::tryNextRoutablePrefix, this, _1));
}

shared_ptr<Data>
UpdateHandler::createDataPacket(const Name& name)
{
  // Parse the sender's routable prefix contained in the name
  Name sendersRoutablePrefix = name.getSubName(2 + 2 + m_torrentName.size());

  if (m_statsTable->find(sendersRoutablePrefix) == m_statsTable->end()) {
    m_statsTable->insert(sendersRoutablePrefix);
  }

  shared_ptr<Data> data = make_shared<Data>(name);

  EncodingEstimator estimator;
  size_t estimatedSize = encodeContent(estimator);

  EncodingBuffer buffer(estimatedSize, 0);
  encodeContent(buffer);

  data->setContentType(tlv::ContentType_Blob);
  data->setContent(buffer.block());

  return data;
}

template<encoding::Tag TAG>
size_t
UpdateHandler::encodeContent(EncodingImpl<TAG>& encoder) const
{
  // Content ::= CONTENT-TYPE TLV-LENGTH
  //             RoutableName+

  // RoutableName ::= NAME-TYPE TLV-LENGTH
  //                  Name

  size_t totalLength = 0;
  // Encode the names of the first five entries of the stats table
  uint32_t namesEncoded = 0;
  for (const auto& entry : *m_statsTable) {
    if (namesEncoded >= MAX_NUM_OF_ENCODED_NAMES) {
      break;
    }
    size_t nameLength = 0;
    nameLength += entry.getRecordName().wireEncode(encoder);
    totalLength += nameLength;
    ++namesEncoded;
  }
  totalLength += encoder.prependVarNumber(totalLength);
  totalLength += encoder.prependVarNumber(tlv::Content);
  return totalLength;
}

void
UpdateHandler::decodeDataPacketContent(const Interest& interest, const Data& data)
{
  // Content ::= CONTENT-TYPE TLV-LENGTH
  //             RoutableName+

  // RoutableName ::= NAME-TYPE TLV-LENGTH
  //                  Name

  LOG_INFO << "ALIVE data packet received: " << data.getName() << std::endl;

  if (data.getContentType() != tlv::ContentType_Blob) {
      BOOST_THROW_EXCEPTION(Error("Expected Content Type Blob"));
  }

  const Block& content = data.getContent();
  content.parse();

  // Decode the names (maintain their ordering)
  for (auto element = content.elements_end() - 1; element != content.elements_begin() - 1; element--) {
    element->parse();
    Name name(*element);
    if (name.empty()) {
      BOOST_THROW_EXCEPTION(Error("Empty routable name was received"));
    }
    if (m_statsTable->find(name) == m_statsTable->end()) {
      m_statsTable->insert(name);
    }
  }
}

bool
UpdateHandler::needsUpdate()
{
  if (m_statsTable->size() < MIN_NUM_OF_ROUTABLE_NAMES) {
    return true;
  }
  for (auto i = m_statsTable->begin(); i != m_statsTable->end(); i++) {
    if (i->getRecordSuccessRate() >= 0.5) {
      return false;
    }
  }
  return true;
}

void
UpdateHandler::learnOwnRoutablePrefix(OnOwnRoutablePrefixFailed onOwnRoutablePrefixFailed)
{
  Interest i(Name("/localhop/nfd/rib/routable-prefixes"));
  i.setInterestLifetime(time::milliseconds(100));

  // parse the first contained routable prefix and set it as the ownRoutablePrefix
  auto prefixReceived = [this] (const Interest& interest, const Data& data) {
    const Block& content = data.getContent();
    content.parse();

    auto element = content.elements_begin();
    element->parse();
    Name ownRoutablePrefix(*element);
    m_ownRoutablePrefix = ownRoutablePrefix;
    LOG_DEBUG << "Own routable prefix received: " << m_ownRoutablePrefix << std::endl;
  };

  auto prefixRetrievalFailed = [this, onOwnRoutablePrefixFailed] (const Interest&) {
    ++m_ownRoutablPrefixRetries;
    LOG_ERROR << "Own Routable Prefix Retrieval Failed. Trying again." << std::endl;
    // If we fail, we will retry OWN_ROUTABLE_PREFIX_RETRIES times
    if (m_ownRoutablPrefixRetries < OWN_ROUTABLE_PREFIX_RETRIES) {
      this->learnOwnRoutablePrefix(onOwnRoutablePrefixFailed);
    }
    else {
      onOwnRoutablePrefixFailed();
    }
  };

  //TODO(Spyros): For now, we do nothing when we receive a NACK
  m_face->expressInterest(i, prefixReceived, nullptr, prefixRetrievalFailed);
}

void
UpdateHandler::onInterestReceived(const InterestFilter& filter, const Interest& interest)
{
  LOG_INFO << "ALIVE Interest Received: " << interest.getName().toUri() << std::endl;
  shared_ptr<Data> data = this->createDataPacket(interest.getName());
  m_keyChain->sign(*data, signingWithSha256());
  m_face->put(*data);
}

void
UpdateHandler::onRegisterFailed(const Name& prefix, const std::string& reason)
{
 LOG_ERROR << "ERROR: Failed to register prefix \""
            << prefix << "\" in local hub's daemon (" << reason << ")"
            << std::endl;
  m_face->shutdown();
}

void
UpdateHandler::tryNextRoutablePrefix(const Interest& interest)
{
  const Name& name = interest.getForwardingHint().begin()->name;
  auto iter = m_statsTable->find(name);

  if (iter != m_statsTable->end()) {
    if (iter + 1 == m_statsTable->end()) {
      iter = m_statsTable->begin();
    }
    else {
      ++iter;
    }
  }
  else {
    iter = m_statsTable->begin();
  }

  shared_ptr<Interest> newInterest = make_shared<Interest>(interest);

  // Create and set the forwarding hint
  Delegation del;
  del.preference = 1;
  del.name = iter->getRecordName();

  DelegationList list({del});

  newInterest->setForwardingHint(list);

  m_face->expressInterest(*newInterest, bind(&UpdateHandler::decodeDataPacketContent, this, _1, _2),
                          bind(&UpdateHandler::tryNextRoutablePrefix, this, _1),
                          bind(&UpdateHandler::tryNextRoutablePrefix, this, _1));
}

} // namespace ntorrent
} // namespace ndn
