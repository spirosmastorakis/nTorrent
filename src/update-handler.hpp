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

#ifndef UPDATE_HANDLER_H
#define UPDATE_HANDLER_H

#include "stats-table.hpp"

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/security/key-chain.hpp>

namespace ndn {
namespace ntorrent {

class UpdateHandler {
public:
  class Error : public tlv::Error
  {
  public:
    explicit
    Error(const std::string& what)
      : tlv::Error(what)
    {
    }
  };

  UpdateHandler(Name torrentName, shared_ptr<KeyChain> keyChain,
                shared_ptr<StatsTable> statsTable, shared_ptr<Face> face);

  ~UpdateHandler();

  /**
   * @brief Send an ALIVE Interest
   * @param routablePrefix The routable prefix to be included in the LINK object attached
   *        to this Interest
   */
  void
  sendAliveInterest(StatsTable::iterator iter);

  /**
   * @brief Check whether we need to send out an "ALIVE" interest
   * @return True if an "ALIVE" interest should be sent out, otherwise false
   *
   * Returns true if we have less than MIN_NUM_OF_ROUTABLE_NAMES prefixes in the stats table
   * or all the routable prefixes has success rate less than 0.5. Otherwise, it returns false
   */
  bool
  needsUpdate();

  enum {
    // Maximum number of names to be encoded as a response to an "ALIVE" Interest
    MAX_NUM_OF_ENCODED_NAMES = 5,
    // Minimum number of routable prefixes that the peer would like to have
    MIN_NUM_OF_ROUTABLE_NAMES = 5,
  };

protected:
  // Used for testing purposes
  const Name&
  getOwnRoutablePrefix();

private:
  template<encoding::Tag TAG>
  size_t
  encodeContent(EncodingImpl<TAG>& encoder) const;

  void
  onInterestReceived(const InterestFilter& filter, const Interest& interest);

  void
  onRegisterFailed(const Name& prefix, const std::string& reason);

  /**
   * @brief Encode the first MAX_NUM_OF_ENCODED_NAMES prefixes of the table into a data packet
   * @param name The name of the data packet
   * @return A shared pointer to the created data packet
   *
   */
  shared_ptr<Data>
  createDataPacket(const Name& name);

  /**
   * @brief Given a received data packet, decode the contained routable name prefixes
   *        and insert them to the table (if not already there)
   * @param interest The interest that retrieved the data packet
   * @param data A shared pointer to the received data packet
   *
   */
  void
  decodeDataPacketContent(const Interest& interest, const Data& data);

  /**
   * @brief Send an Interest to the local NFD to get the routable prefixes under which the
   * published data is available
   */
  void
  learnOwnRoutablePrefix();

  void
  tryNextRoutablePrefix(const Interest& interest);

private:
  Name m_torrentName;
  shared_ptr<KeyChain> m_keyChain;
  shared_ptr<StatsTable> m_statsTable;
  shared_ptr<Face> m_face;
  Name m_ownRoutablePrefix;
};

inline
UpdateHandler::UpdateHandler(Name torrentName, shared_ptr<KeyChain> keyChain,
                             shared_ptr<StatsTable> statsTable, shared_ptr<Face> face)
: m_torrentName(torrentName)
, m_keyChain(keyChain)
, m_statsTable(statsTable)
, m_face(face)
{
  this->learnOwnRoutablePrefix();
  m_face->setInterestFilter(Name("/NTORRENT" + m_torrentName.toUri() + "/ALIVE"),
                            bind(&UpdateHandler::onInterestReceived, this, _1, _2),
                            RegisterPrefixSuccessCallback(),
                            bind(&UpdateHandler::onRegisterFailed, this, _1, _2));
}

inline
UpdateHandler::~UpdateHandler()
{
}

inline const Name&
UpdateHandler::getOwnRoutablePrefix()
{
  return m_ownRoutablePrefix;
}

} // namespace ntorrent
} // namespace ndn

#endif // UPDATE_HANDLER_H
