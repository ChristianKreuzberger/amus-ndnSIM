#include "adaptation-logic-svc-buffer-based.hpp"
namespace dash
{
namespace player
{

ENSURE_ADAPTATION_LOGIC_INITIALIZED(SVCBufferBasedAdaptationLogic)

ISegmentURL*
SVCBufferBasedAdaptationLogic::GetNextSegment(unsigned int* requested_segment_number, const dash::mpd::IRepresentation** usedRepresentation)
{
  //TODO IMPLEMENT

  IRepresentation* rep = GetLowestRepresentation();
  *usedRepresentation = rep;
  *requested_segment_number = currentSegmentNumber;
  return rep->GetSegmentList()->GetSegmentURLs().at(currentSegmentNumber++);
}

}
}
