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

#ifndef DASH_ALWAYS_LOWEST_ADAPTATION_LOGIC
#define DASH_ALWAYS_LOWEST_ADAPTATION_LOGIC

#include "adaptation-logic.hpp"

namespace dash
{
namespace player
{
class AlwaysLowestAdaptationLogic : public AdaptationLogic
{
public:
  AlwaysLowestAdaptationLogic(MultimediaPlayer* mPlayer) : AdaptationLogic (mPlayer)
  {
  }

  virtual std::string GetName() const
  {
    return "dash::player::AlwaysLowestAdaptationLogic";
  }

  static std::shared_ptr<AdaptationLogic> Create(MultimediaPlayer* mPlayer)
  {
    return std::make_shared<AlwaysLowestAdaptationLogic>(mPlayer);
  }

  virtual ISegmentURL*
  GetNextSegment(unsigned int current_segment_number);


protected:
  static AlwaysLowestAdaptationLogic _staticLogic;

  AlwaysLowestAdaptationLogic()
  {
    ENSURE_ADAPTATION_LOGIC_REGISTERED(AlwaysLowestAdaptationLogic);
  }


};
}
}


#endif // DASH_ALWAYS_LOWEST_ADAPTATION_LOGIC
