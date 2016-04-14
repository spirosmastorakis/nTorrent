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

#ifndef NDN_TESTS_UNIT_TESTS_DUMMY_PARSER_HPP
#define NDN_TESTS_UNIT_TESTS_DUMMY_PARSER_HPP

#include <ndn-cxx/data.hpp>

#include <vector>

namespace ndn {
namespace ntorrent {
namespace tests {

using std::vector;

class DummyParser {
public:
  DummyParser()
  {
  }

  ~DummyParser()
  {
  }

  static shared_ptr<Data>
  createDataPacket(const Name& packetName, const std::vector<Name>& vec)
  {
    shared_ptr<Data> data = make_shared<Data>(packetName);

    EncodingEstimator estimator;
    size_t estimatedSize = encodeContent(estimator, vec);

    EncodingBuffer buffer(estimatedSize, 0);
    encodeContent(buffer, vec);

    data->setContentType(tlv::ContentType_Blob);
    data->setContent(buffer.block());

    return data;
  }

  static shared_ptr<vector<Name>>
  decodeContent(const Block& content)
  {
    shared_ptr<vector<Name>> nameVec = make_shared<vector<Name>>();
    content.parse();
    // Decode the names (do not worry about the order)
    for (auto element = content.elements_begin(); element != content.elements_end(); element++) {
      element->parse();
      Name name(*element);
      nameVec->push_back(name);
    }
    return nameVec;
  }
private:
  template<encoding::Tag TAG>
  static size_t
  encodeContent(EncodingImpl<TAG>& encoder, const std::vector<Name>& vec)
  {
    // Content ::= CONTENT-TYPE TLV-LENGTH
    //             RoutableName+

    // RoutableName ::= NAME-TYPE TLV-LENGTH
    //                  Name

    size_t totalLength = 0;
    for (const auto& element : vec) {
      size_t nameLength = 0;
      nameLength += element.wireEncode(encoder);
      totalLength += nameLength;
    }
    totalLength += encoder.prependVarNumber(totalLength);
    totalLength += encoder.prependVarNumber(tlv::Content);
    return totalLength;
  }
};

} // namespace tests
} // namespace ntorrent
} // namespace ndn

#endif // NDN_TESTS_UNIT_TESTS_DUMMY_PARSER_HPP
