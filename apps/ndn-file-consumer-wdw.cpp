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


void
FileConsumerWdw::StartApplication()
{
  FileConsumer::StartApplication();
  m_inFlight = 0;
  m_windowSize = m_lastWindowSize = 1;
  m_windowThreshold = 1000000;

  //averageTimeout = 1000;

  received_packets_during_this_window = 0;
  timeouts_during_this_window = 0;
  lastSeqNoRecv = 0;

  m_hadWrongSeqOrder = m_hadTimeout = false;
}



void
FileConsumerWdw::IncrementWindow()
{
  if (m_windowSize < m_windowThreshold)
  {
    m_windowSize = m_windowSize * 2;

    if (m_windowSize > m_windowThreshold)
    {
      m_windowSize = m_windowThreshold;
    }

    m_cwndPhase = SlowStart;
  } else
  {
    m_windowSize = m_windowSize + 1;
    m_cwndPhase = AdditiveIncrease;
  }
}


void
FileConsumerWdw::DecrementWindow()
{
}


uint32_t
FileConsumerWdw::GetMaxConSeqNo()
{
  uint32_t seqNo = 0;

  for ( auto &seqStatus : m_sequenceStatus ) {
    if (seqStatus != Received )
    {
      return seqNo;
    }
    seqNo++;
  }

  return seqNo;
}



void
FileConsumerWdw::AfterData(bool manifest, uint32_t seq_nr)
{
  NS_LOG_FUNCTION(this << manifest << seq_nr);

  m_inFlight--;
  received_packets_during_this_window++;

  if (m_fileSize > 0 && AreAllSeqReceived())
  {
    OnFileReceived(0,0);
    return;
  }

  uint32_t tmp1 = GetMaxConSeqNo();

  if (manifest == false && seq_nr != 0 && lastSeqNoRecv != 0)
  {
    // check for lost packets
    if (tmp1 == lastSeqNoRecv)
    {
      if (tmp1 != preLastSeqNoRecv)
      {
        m_hadWrongSeqOrder = true;
        NS_LOG_DEBUG("Found wrong seq order --> switching to MD mode");
        // either we lost a packet or this comes from a retransmission
        // ask for retransmission of tmp1
        m_sequenceStatus[tmp1+1] = TimedOut;

        // found wrong sequence order, immediately move to MultiplicativeDecrease Phase
        m_cwndPhase = MultiplicativeDecrease;
      } else
      {
        // transfer is still working, go back to additive increase
        m_cwndPhase = AdditiveIncrease;
      }
    } else // tmp1 != lastSeqNoRecv
    {
      // tmp1 should be lastSeqNoRecv+1
      // if not, we fixed a packet, which is good too
      if (tmp1 != lastSeqNoRecv+1)
      {
        if (m_cwndPhase == MultiplicativeDecrease)
          m_cwndPhase = AdditiveIncrease;
      }
    }
  }

  // is this window done?
  if ((received_packets_during_this_window+timeouts_during_this_window) >= m_lastWindowSize)
  {
    NS_LOG_DEBUG("RTT done - lastWindowSize = " << m_lastWindowSize);
    NS_LOG_DEBUG("Packets_Sent=" << m_packetsSent << ", packets_Recv=" << m_packetsReceived << ", packets_Timeout=" << m_packetsTimeout);
    // we got all packets from this window
    if (!m_hadTimeout && !m_hadWrongSeqOrder)
    {
      // and we did not have a timeout or a wrong sequence order --> good
      // means we can grow the congestion window - but by how much?
      IncrementWindow();
      NS_LOG_DEBUG("Incremented Window to " << m_windowSize);
    } else
    {
      // had a timeout or a wrong seq order --> set threshold new
      m_cwndPhase = MultiplicativeDecrease;
      m_windowThreshold = m_windowSize / 2;
      m_windowSize = m_windowSize / 2;
      NS_LOG_DEBUG("Decremented Window to " << m_windowSize);
    }

    if (m_windowSize < 1)
      m_windowSize = 1;

    m_lastWindowSize = m_windowSize;

    received_packets_during_this_window = 0;
    timeouts_during_this_window = 0;
    m_hadTimeout = false;
    m_hadWrongSeqOrder = false;
  }


  if (manifest == false)
  {
    preLastSeqNoRecv = lastSeqNoRecv;
    lastSeqNoRecv = tmp1;
  }






  if (AreAllSeqReceived())
  {
    OnFileReceived(0, 0);
    return;
  }





  if (m_cwndPhase == SlowStart)
  {
    NS_LOG_DEBUG("Slow_start SendPacket()x2, cwnd= " << m_windowSize);

    // send two packets
    if (SendPacket())
      SendPacket();
  } else if (m_cwndPhase == AdditiveIncrease)
  {
    // send one packet, or send two packets at the end of cwnd
    if (received_packets_during_this_window+timeouts_during_this_window >= m_lastWindowSize-1)
    {
      NS_LOG_DEBUG("add_incre SendPacket(): 1+1, cwnd= " << m_windowSize);

      SendPacket();
    } else
    {
      NS_LOG_DEBUG("add_incre SendPacket(): 1  , cwnd= " << m_windowSize);
    }
    SendPacket();
  } else if (m_cwndPhase == MultiplicativeDecrease)
  {
    NS_LOG_DEBUG("Mult-Decr, total packets == " << (received_packets_during_this_window+timeouts_during_this_window));
    // send a packet only for two packets that arrive
    if ((received_packets_during_this_window+timeouts_during_this_window) % 2 == 0)
    {
      NS_LOG_DEBUG("mult_decr SendPacket(): 1/2, cwnd= " << m_windowSize);
      SendPacket();
    } else
    {
      NS_LOG_DEBUG("mult_decr SendPacket(): NONE, cwnd= " << m_windowSize);
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
  m_inFlight++;

  NS_LOG_DEBUG("After SendPacket - return was " << okay);

  if (!okay)
  {
    // schedule next event anyway
    ScheduleNextSendEvent((1000.0 / (double)(m_windowSize)));
  }


  return okay;
}



void
FileConsumerWdw::OnTimeout(uint32_t seqNo)
{
  NS_LOG_DEBUG("Timeout for seq " << seqNo);
  timeouts_during_this_window++;
  m_hadTimeout= true;
  m_inFlight--;

  m_cwndPhase = MultiplicativeDecrease;

  //m_cwndPhase = MultiplicativeDecrease;
  received_packets_during_this_window--;
  AfterData(true, 0);
}


} // namespace ndn
} // namespace ns3
