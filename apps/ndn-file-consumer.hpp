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

#ifndef NDN_MANIFESTCONSUMER_CBR_H
#define NDN_MANIFESTCONSUMER_CBR_H

#include "ns3/ndnSIM/model/ndn-common.hpp"

#include "ndn-app.hpp"

#include "ns3/random-variable.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"

#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.hpp"

#include "ns3/traced-callback.h"
#include "ns3/ptr.h"
#include "ns3/simple-ref-count.h"

namespace ns3 {
namespace ndn {




/**
 * @ingroup ndn-apps
 * @brief Ndn application for sending out Interest packets at a "constant" rate (Poisson process)
 */
class FileConsumer : public App {
public:
  static TypeId
  GetTypeId();

  /**
   * \brief Default constructor
   * Sets up randomizer function and packet sequence number
   */
  FileConsumer();
  virtual ~FileConsumer();

  virtual void
  StartApplication();

  virtual void
  StopApplication();

  enum SequenceStatus { NotRequested = 0, Requested = 1, TimedOut = 2, Received = 3 };

protected:
  UniformVariable m_rand; ///< @brief nonce generator

  virtual void
  OnData(shared_ptr<const Data> data);

  virtual void
  OnManifest(long fileSize);

  virtual void
  OnFileData(uint32_t seq_nr, const uint8_t* data, unsigned length);

  virtual void
  OnFileReceived(unsigned status, unsigned length);

  virtual void
  ScheduleNextSendEvent(unsigned int miliseconds=0);

  virtual void
  SendPacket();

  virtual void
  SendManifestPacket();

  virtual void
  SendFilePacket();

  virtual uint32_t
  GetNextSeqNo(); // returns the next sequence number that should be scheduled for download


  virtual bool
  AreAllSeqReceived();


  EventId m_sendEvent; ///< @brief EventId of pending "send packet" event
  Name m_interestName;     ///< \brief NDN Name of the Interest (use Name)
  Time m_interestLifeTime; ///< \brief LifeTime for interest packet

  std::string m_manifestPostfix;


  bool m_hasRequestedManifest;
  bool m_hasReceivedManifest;


  std::string m_outFile;


  long m_fileSize;
  uint32_t m_curSeqNo;
  uint32_t m_maxSeqNo;
  uint32_t m_lastSeqNoReceived;

  uint32_t m_maxPayloadSize;


  std::vector<SequenceStatus> m_sequenceStatus;


protected:
  TracedCallback<Ptr<ns3::ndn::App> /* app */, shared_ptr<const Name> /* interestName */> m_downloadStartedTrace;
  TracedCallback<Ptr<ns3::ndn::App> /* app */, shared_ptr<const Name> /* interestName */, long /*fileSize*/> m_manifestReceivedTrace;
  TracedCallback<Ptr<ns3::ndn::App> /* app */, shared_ptr<const Name> /* interestName */,double /* downloadSpeedInBytesPerSecond */, long /*milliSeconds */> m_downloadFinishedTrace;



private:
  int64_t _start_time;
  int64_t _finished_time;
  shared_ptr<Name> _shared_interestName; // = make_shared<Name>(m_interestName);



};

} // namespace ndn
} // namespace ns3

#endif
