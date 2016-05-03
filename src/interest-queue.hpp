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

#include <ndn-cxx/data.hpp>
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/interest.hpp>

#include <queue>
#include <tuple>

typedef std::tuple<std::shared_ptr<ndn::Interest>, ndn::DataCallback, ndn::TimeoutCallback> queueTuple;

namespace ndn {
namespace ntorrent {

class InterestQueue
{
public:
  InterestQueue() = default;

  ~InterestQueue() = default;

  /**
   * @brief Push a tuple to the Interest Queue
   * @param interest A shared pointer to an Interest
   * @param dataReceivedCallback Callback to be called when data is received for the given
   *                             Interest
   * @param dataFailedCallback Callback to be called when we fail to retrieve data for the
   *                           given Interest
   *
   */
  void
  push(shared_ptr<Interest> interest, DataCallback dataReceivedCallback,
       TimeoutCallback dataFailedCallback);

  /**
   * @brief Pop a tuple from the Interest Queue
   * @return A tuple of a shared pointer to an Interest, a callaback for successful data
   *         retrieval and a callback for failed data retrieval
   */
  queueTuple
  pop();

  /**
   * @brief Return the size of the queue (number of tuples)
   * @return The number of tuples stored in the queue
   */
  size_t
  size() const;

  /**
   * @brief Check if the queue is empty
   * @return True if the queue is empty, otherwise false
   */
  bool
  empty() const;

  /**
   * @brief Return the top element of the Interest queue
   * @return The top tuple element of the Interest queue
   */
   queueTuple
   front() const;

private:
  std::queue<queueTuple> m_queue;
};

inline size_t
InterestQueue::size() const
{
  return m_queue.size();
}

inline bool
InterestQueue::empty() const
{
  return m_queue.empty();
}

inline queueTuple
InterestQueue::front() const
{
  return m_queue.front();
}

} // namespace ntorrent
} // namespace ndn
