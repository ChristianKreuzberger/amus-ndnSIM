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
      .SetParent<FileConsumerCbr>()
      .AddConstructor<FileConsumerWdw>()
    ;

  return tid;
}

FileConsumerWdw::FileConsumerWdw() : FileConsumerCbr()
{
  NS_LOG_FUNCTION_NOARGS();
}

FileConsumerWdw::~FileConsumerWdw()
{
}



void
FileConsumerWdw::StartApplication()
{
  NS_LOG_FUNCTION_NOARGS();
  FileConsumerCbr::StartApplication();
  // the constructor of FileConsumerCbr should have calculated the maximum window size, so we are going to set it here
  m_maxWindowSize = m_windowSize;


  m_cwndSSThresh = 1000000;


  ignoreTimeoutsCounter = 0;

  m_cwndPhase = SlowStart;
}



void
FileConsumerWdw::IncrementWindow()
{
  NS_LOG_FUNCTION_NOARGS();
  double incrementRatio = 1000.0 / (EstimatedRTT);

  if (m_windowSize < m_cwndSSThresh)
  {
    m_windowSize+= incrementRatio;
    m_cwndPhase = SlowStart;
  } else
  {
    m_windowSize = m_windowSize + incrementRatio / m_windowSize;
    m_cwndPhase = AdditiveIncrease;
  }

  // make sure that we do not request more than we can request
  if (m_windowSize > m_maxWindowSize)
    m_windowSize = m_maxWindowSize;
}


void
FileConsumerWdw::DecrementWindow()
{
  NS_LOG_FUNCTION_NOARGS();
  m_windowSize = m_windowSize / 2.0;

  m_cwndPhase = MultiplicativeDecrease;

  // make sure that we can not get a "too small" window size
  if (m_windowSize < 4)
    m_windowSize = 4;

  m_cwndSSThresh = m_windowSize;
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

    // we need to back off, rescheduling the next send event
    ScheduleNextSendEvent(1000.0 / m_windowSize);
  }

  if (ignoreTimeoutsCounter > 0)
  {
    ignoreTimeoutsCounter--;
  }



  if (!timeout)
  {
    // when ignoreTimeoutsCounter > 0, then we did get a packet that we didn't expect
    IncrementWindow();

    if (m_nextEventScheduleTime > Simulator::Now().GetMilliSeconds() + 1000.0 / m_windowSize)
    {
      ScheduleNextSendEvent(1000.0 / m_windowSize);
    }
  }

  // if we just received the manifest, let's start sending out packets
  if (manifest)
  {
    SendPacket();
  }
}



} // namespace ndn
} // namespace ns3
