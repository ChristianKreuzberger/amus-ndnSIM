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

#include "ndn-file-consumer-wdw.hpp"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/integer.h"
#include "ns3/double.h"

#include "utils/ndn-ns3-packet-tag.hpp"
#include "model/ndn-app-face.hpp"
#include "utils/ndn-rtt-mean-deviation.hpp"

#include "model/ndn-app-face.hpp"

#include <math.h>


NS_LOG_COMPONENT_DEFINE("ndn.FileConsumerWdw");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(FileConsumerWdw);

TypeId
FileConsumerWdw::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::FileConsumerWdw")
      .SetGroupName("Ndn")
      .SetParent<FileConsumer>()
      .AddConstructor<FileConsumerWdw>()
      .AddAttribute("MaxWindowSize", "The max. amount of interests that are issued per second", UintegerValue(100),
                    MakeUintegerAccessor(&FileConsumerWdw::m_maxWindowSize),
                    MakeUintegerChecker<uint32_t>())
    ;

  return tid;
}

FileConsumerWdw::FileConsumerWdw() : FileConsumer()
{
  NS_LOG_FUNCTION_NOARGS();
}

FileConsumerWdw::~FileConsumerWdw()
{
}



uint32_t
FileConsumerWdw::IncreaseCounter()
{
  m_counter++;
  m_counter = m_counter % LAST_SEQ_WINDOW_SIZE;
}


unsigned int
FileConsumerWdw::CountDuplicateAcks()
{
  uint32_t curSeqNo = m_lastSeqRecvArray[m_counter];

  int duplicateAckCnt = 0;

  for (int i = 1; i < LAST_SEQ_WINDOW_SIZE; i++)
  {
    uint32_t thisSeqNo = m_lastSeqRecvArray[(m_counter -  i + LAST_SEQ_WINDOW_SIZE) % LAST_SEQ_WINDOW_SIZE];

    if (thisSeqNo == curSeqNo)
    {
      duplicateAckCnt++;
    } else
    {
      return duplicateAckCnt;
    }

  }
  return duplicateAckCnt;
}


void
FileConsumerWdw::StartApplication()
{
  FileConsumer::StartApplication();
  m_inFlight = 0;
  m_windowSize = m_lastWindowSize = 4;
  m_cwndSSThresh = 1000000;
  m_counter = 0;


  ignoreTimeoutsCounter = 0;


  // that is what we start with
  m_cwndPhase = SlowStart;

  //averageTimeout = 1000;

  received_packets_during_this_window = 0;
  timeouts_during_this_window = 0;
  lastSeqNoRecv = 0;

  m_hadWrongSeqOrder = m_hadTimeout = false;
}



void
FileConsumerWdw::IncrementWindow()
{
  if (m_windowSize < m_cwndSSThresh)
  {
    m_windowSize++;
    m_cwndPhase = SlowStart;
  } else
  {
    m_windowSize = m_windowSize + 1.0 / m_windowSize;
    m_cwndPhase = AdditiveIncrease;
  }

  if (m_windowSize > 83)
    m_windowSize = 83;
}


void
FileConsumerWdw::DecrementWindow()
{
  m_windowSize = m_windowSize * 3.0/4.0;

  m_cwndPhase = MultiplicativeDecrease;

  // make sure that we can not get a "too small" window size
  if (m_windowSize < 4)
    m_windowSize = 4;

  m_cwndSSThresh = m_windowSize;
}


uint32_t
FileConsumerWdw::GetMaxConSeqNo()
{
  uint32_t seqNo = 0;

  for ( auto &seqStatus : m_sequenceStatus ) {
    // find the first sequence which has not been received
    if (seqStatus != Received )
    {
      return seqNo - 1;
    }
    seqNo++;
  }

  return seqNo;
}



void
FileConsumerWdw::UpdateCwndSSThresh()
{
  m_cwndSSThresh = m_windowSize * 3.0/4.0;
  if (m_cwndSSThresh < 4)
    m_cwndSSThresh = 4;

  NS_LOG_DEBUG("New SS Threshold: " << m_cwndSSThresh);
}



void
FileConsumerWdw::AfterData(bool manifest, bool timeout, uint32_t seq_nr)
{
  if (timeout && m_cwndPhase != MultiplicativeDecrease && ignoreTimeoutsCounter == 0)
  {
    // we got a timeout, make sure that we "ignore" the next window_size timeouts for the next timeout
    ignoreTimeoutsCounter = m_windowSize; // this is how many timeouts we expect, based on the current timeout

    //NS_LOG_UNCOND("Timeout, cwnd was " << m_windowSize);

    DecrementWindow();

    EstimatedRTT = EstimatedRTT * 2;
    if (EstimatedRTT > 500)
      EstimatedRTT = 500;

    // we need to back off, rescheduling the next send event
    ScheduleNextSendEvent(WINDOW_TIMER / m_windowSize);
  }

  if (ignoreTimeoutsCounter > 0)
  {
    ignoreTimeoutsCounter--;
  }


  if (!timeout && AreAllSeqReceived())
  {
    OnFileReceived(0, 0);
  }

  if (!timeout)
  {
    // when ignoreTimeoutsCounter > 0, then we did get a packet that we didn't expect
    IncrementWindow();

    if (m_cwndPhase == SlowStart)
    {
      // Put some pressure on the network - in addition to the scheduled packets
      SendPacket();
    } else if (m_cwndPhase == AdditiveIncrease)
    {
      ScheduleNextSendEvent(WINDOW_TIMER / m_windowSize);
    }
  }
}





bool
FileConsumerWdw::SendPacket()
{
  //NS_LOG_FUNCTION(this);
  //NS_LOG_DEBUG("m_inFlight=" << m_inFlight << ", m_windowSize=" << m_windowSize);
  //NS_LOG_DEBUG("Packets_Sent=" << m_packetsSent << ", packets_Recv=" << m_packetsReceived << ", packets_Timeout=" << m_packetsTimeout);
  bool okay = false;

  okay = FileConsumer::SendPacket();

  if (okay)
  {
    m_inFlight++;
  }

  if (!m_finishedDownloadingFile)
  {
    // schedule next send
    ScheduleNextSendEvent(WINDOW_TIMER / m_windowSize);
  }
  else
  {
    NS_LOG_UNCOND("Finished...");
  }

  return okay;
}



void
FileConsumerWdw::OnTimeout(uint32_t seqNo)
{
  timeouts_during_this_window++;
  m_hadTimeout= true;



  // will trigger AfterData(false, true,seqNo)
  FileConsumer::OnTimeout(seqNo);
}


} // namespace ndn
} // namespace ns3
