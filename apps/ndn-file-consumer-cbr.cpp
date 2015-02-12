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

#include "ndn-file-consumer-cbr.hpp"
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


NS_LOG_COMPONENT_DEFINE("ndn.FileConsumerCbr");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(FileConsumerCbr);

TypeId
FileConsumerCbr::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::FileConsumerCbr")
      .SetGroupName("Ndn")
      .SetParent<FileConsumer>()
      .AddConstructor<FileConsumerCbr>()
      .AddAttribute("WindowSize", "The amount of interests that are issued per second", UintegerValue(100),
                    MakeUintegerAccessor(&FileConsumerCbr::m_windowSize),
                    MakeUintegerChecker<uint32_t>())
    ;

  return tid;
}

FileConsumerCbr::FileConsumerCbr() : FileConsumer()
{
  NS_LOG_FUNCTION_NOARGS();
}

FileConsumerCbr::~FileConsumerCbr()
{
}


void
FileConsumerCbr::StartApplication()
{
  FileConsumer::StartApplication();

  m_inFlight = 0;
}



void
FileConsumerCbr::AfterData(bool manifest, bool timeout, uint32_t seq_nr)
{
  NS_LOG_FUNCTION(this << manifest << timeout << seq_nr);
  m_inFlight--;

  if (AreAllSeqReceived())
  {
    OnFileReceived(0, 0);
  }

  if (timeout)
  {
    EstimatedRTT = EstimatedRTT * 2;
    if (EstimatedRTT > 500)
      EstimatedRTT = 500;
  }

  // if we just received the manifest, let's start sending out packets
  if (manifest)
  {
    SendPacket();
  }
}





bool
FileConsumerCbr::SendPacket()
{
  bool okay = FileConsumer::SendPacket();

  if (okay)
  {
    m_inFlight++;
  }


  if (m_hasReceivedManifest && this->m_fileSize > 0)
  {
    if (AreAllSeqReceived())
    {
      NS_LOG_DEBUG("Done, triggering OnFileReceived...");
      OnFileReceived(0, 0);
    } else {
      // schedule next event
      ScheduleNextSendEvent(m_rand.GetValue() + 1000.0 / (double)m_windowSize);
    }
  }

  return okay;
}




} // namespace ndn
} // namespace ns3
