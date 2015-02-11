/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2015 - Christian Kreuzberger - based on ndnSIM
 *
 * This file is part of amus-ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

#ifndef NDN_FILECONSUMER_CBR_H
#define NDN_FILECONSUMER_CBR_H


#include "ndn-file-consumer.hpp"

namespace ns3 {
namespace ndn {




/**
 * @ingroup ndn-apps
 * @brief Ndn application for sending out Interest packets at a "constant" rate (Poisson process)
 */
class FileConsumerWdw : public FileConsumer {
public:
  static TypeId
  GetTypeId();

  /**
   * \brief Default constructor
   * Sets up randomizer function and packet sequence number
   */
  FileConsumerWdw();
  virtual ~FileConsumerWdw();


  virtual void
  StartApplication();


  enum CongestionWindowPhase { SlowStart = 0, AdditiveIncrease = 1, MultiplicativeDecrease = 2 };


protected:
  virtual bool
  SendPacket();

  virtual void
  OnTimeout(uint32_t seqNo);

  virtual void
  AfterData(bool manifest, uint32_t seq_nr);


  virtual void
  IncrementWindow();

  virtual void
  DecrementWindow();

  virtual uint32_t
  GetMaxConSeqNo();


  double m_windowSize;
  unsigned int m_maxWindowSize;
  unsigned int m_windowThreshold;
  unsigned int m_inFlight;



  unsigned int m_lastWindowSize;


  uint32_t lastSeqNoRecv;
  uint32_t preLastSeqNoRecv;
  bool m_hadWrongSeqOrder;
  bool m_hadTimeout;


  unsigned int received_packets_during_this_window;
  unsigned int timeouts_during_this_window;




  CongestionWindowPhase m_cwndPhase;



};

} // namespace ndn
} // namespace ns3

#endif
