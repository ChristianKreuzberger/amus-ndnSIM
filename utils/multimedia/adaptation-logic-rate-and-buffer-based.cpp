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

#include "adaptation-logic-rate-and-buffer-based.hpp"
#include "multimedia-player.hpp"


namespace dash
{
namespace player
{

ENSURE_ADAPTATION_LOGIC_INITIALIZED(RateAndBufferBasedAdaptationLogic)

ISegmentURL*
RateAndBufferBasedAdaptationLogic::GetNextSegment(unsigned int *requested_segment_number, const dash::mpd::IRepresentation **usedRepresentation)
{
  const IRepresentation* useRep = NULL;;

  double factor = 1.0;

  if (this->m_multimediaPlayer->GetBufferLevel() < 4)
  {
    factor = 0.33;
  } else if (this->m_multimediaPlayer->GetBufferLevel() >= 4 && this->m_multimediaPlayer->GetBufferLevel() < 8)
  {
    factor = 0.66;
  } else if (this->m_multimediaPlayer->GetBufferLevel() >= 8 && this->m_multimediaPlayer->GetBufferLevel() < 16)
  {
    factor = 1.0;
  } else {
    factor = 1.2;
  }

  double last_download_speed = this->m_multimediaPlayer->GetLastDownloadBitRate();

  double highest_bitrate = 0.0;

  for (auto& keyValue : *(this->m_availableRepresentations))
  {
    const IRepresentation* rep = keyValue.second;
    std::cerr << "Rep=" << keyValue.first << " has bitrate " << rep->GetBandwidth() << std::endl;
    if (rep->GetBandwidth() < last_download_speed*factor)
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
  return useRep->GetSegmentList()->GetSegmentURLs().at(currentSegmentNumber++);
}

}

}
