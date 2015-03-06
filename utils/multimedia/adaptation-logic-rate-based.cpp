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

#include "adaptation-logic-rate-based.hpp"
#include "multimedia-player.hpp"


namespace dash
{
namespace player
{

ENSURE_ADAPTATION_LOGIC_INITIALIZED(RateBasedAdaptationLogic)

ISegmentURL*
RateBasedAdaptationLogic::GetNextSegment(unsigned int *requested_segment_number, const dash::mpd::IRepresentation **usedRepresentation)
{
  double last_download_speed = this->m_multimediaPlayer->GetLastDownloadBitRate();

  const IRepresentation* useRep = NULL;

  double highest_bitrate = 0.0;

  for (auto& keyValue : *(this->m_availableRepresentations))
  {
    const IRepresentation* rep = keyValue.second;
    //std::cerr << "Rep=" << keyValue.first << " has bitrate " << rep->GetBandwidth() << std::endl;
    if (rep->GetBandwidth() < last_download_speed)
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

  //IRepresentation* rep = (this->m_availableRepresentations->begin()->second);
  *usedRepresentation = useRep;
  *requested_segment_number = currentSegmentNumber;
  return useRep->GetSegmentList()->GetSegmentURLs().at(currentSegmentNumber++);
}
}

}
