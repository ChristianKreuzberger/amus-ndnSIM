/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2015 Christian Kreuzberger and Daniel Posch, Alpen-Adria-University
 * Klagenfurt
 *
 * This file is part of amus-ndnSIM, based on ndnSIM. See AUTHORS for complete list of
 * authors and contributors.
 *
 * amus-ndnSIM and ndnSIM are free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * amus-ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * amus-ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include "adaptation-logic-dashjs.hpp"
#include "multimedia-player.hpp"


namespace dash
{
namespace player
{

ENSURE_ADAPTATION_LOGIC_INITIALIZED(DASHJSAdaptationLogic)

ISegmentURL*
DASHJSAdaptationLogic::GetNextSegment(unsigned int *requested_segment_number,
                                                  const dash::mpd::IRepresentation **usedRepresentation, bool *hasDownloadedAllSegments)
{
  if(currentSegmentNumber < getTotalSegments ())
    *hasDownloadedAllSegments = false;
  else
  {
    *hasDownloadedAllSegments = true;
    return NULL; // everything downloaded
  }

  const IRepresentation* useRep = NULL;;

  double factor = 1.0;

  if (this->m_multimediaPlayer->GetBufferLevel() < 4)
  { // be passive in the beginning
    factor = 0.33;
  } else if (this->m_multimediaPlayer->GetBufferLevel() >= 4 && this->m_multimediaPlayer->GetBufferLevel() < 8)
  { // still passive when there is 4 - 8 seconds in the buffer
    factor = 0.66;
  } else if (this->m_multimediaPlayer->GetBufferLevel() >= 8 && this->m_multimediaPlayer->GetBufferLevel() < 16)
  { // okay, we have 8 seconds in the buffer, that's quite good against short time fluctuations
    factor = 1.0;
  } else { // more than 16 seconds --> we can risk getting something that takes a bit longer or requires slightly more
    factor = 1.1;
  }

  double last_download_speed = this->m_multimediaPlayer->GetLastDownloadBitRate();

  double weighted_download_speed = (previousDownloadSpeed != 0? (0.7*previousDownloadSpeed + 1.3*last_download_speed)/2.0 :last_download_speed);

  double highest_bitrate = 0.0;

  for (auto& keyValue : *(this->m_availableRepresentations))
  {
    const IRepresentation* rep = keyValue.second;
    // std::cerr << "Rep=" << keyValue.first << " has bitrate " << rep->GetBandwidth() << std::endl;
    if (rep->GetBandwidth() < weighted_download_speed*factor)
    {
      if (rep->GetBandwidth() > highest_bitrate)
      {
        useRep = rep;
        highest_bitrate = rep->GetBandwidth();
      }
    }
  }

  if (useRep == NULL)
    useRep = GetLowestRepresentation();

  //std::cerr << "Representation used: " << useRep->GetId() << std::endl;

  //IRepresentation* rep = (this->m_availableRepresentations->begin()->second);
  *usedRepresentation = useRep;
  *requested_segment_number = currentSegmentNumber;
  *hasDownloadedAllSegments = false;

  // remember previousDownloadSpeed
  previousDownloadSpeed = last_download_speed;

  return useRep->GetSegmentList()->GetSegmentURLs().at(currentSegmentNumber++);
}

}

}
