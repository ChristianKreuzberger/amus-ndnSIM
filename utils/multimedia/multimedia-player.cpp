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

#include "multimedia-player.hpp"



namespace dash
{
namespace player
{
MultimediaPlayer::MultimediaPlayer() : MultimediaPlayer("dash::player::AdaptationLogic")
{
  m_lastBitrate = 0.0;
}


MultimediaPlayer::~MultimediaPlayer()
{
#if defined(DEBUG) || defined(NS3_LOG_ENABLE)
  std::cerr << "Deleting MultimediaPlayer;";
#endif
  m_adaptLogic = nullptr;
}

MultimediaPlayer::MultimediaPlayer(std::string AdaptationLogicStr) : m_buffer(0)
{
  std::shared_ptr<AdaptationLogic> aLogic = AdaptationLogicFactory::Create(AdaptationLogicStr, this);

  if (aLogic == nullptr)
  {
    std::cerr << "MultimediaPlayer():\tFailed initializing adaptation logic '" << AdaptationLogicStr << "'" << std::endl;
  }
  else
  {
#if defined(DEBUG) || defined(NS3_LOG_ENABLE)
    std::cerr << "MultimediaPlayer():\tInitialized adaptation logic of type " << aLogic->GetName() << std::endl;
#endif
    m_adaptLogic = aLogic;
  }
}



std::shared_ptr<AdaptationLogic>&
MultimediaPlayer::GetAdaptationLogic()
{
  return m_adaptLogic;
}



void
MultimediaPlayer::SetAvailableRepresentations(std::map<std::string, IRepresentation*>* availableRepresentations)
{
  this->m_availableRepresentations = availableRepresentations;
  m_adaptLogic->SetAvailableRepresentations(availableRepresentations);
}


void
MultimediaPlayer::AddToBuffer(unsigned int seconds)
{
  m_buffer += seconds;
}


unsigned int
MultimediaPlayer::GetBufferLevel()
{
  return m_buffer;
}


bool
MultimediaPlayer::ConsumeFromBuffer(unsigned int seconds)
{
  if (m_buffer >= seconds)
  {
    m_buffer -= seconds;
    return true;
  }
  return false;
}


void
MultimediaPlayer::SetLastDownloadBitRate(double bitrate)
{
  this->m_lastBitrate = bitrate;
}


double
MultimediaPlayer::GetLastDownloadBitRate()
{
  return this->m_lastBitrate;
}



}
}
