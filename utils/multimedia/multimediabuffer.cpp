#include "multimediabuffer.hpp"

using namespace dash::player;

MultimediaBuffer::MultimediaBuffer(unsigned int maxBufferedSeconds)
{
  this->maxBufferedSeconds= (double) maxBufferedSeconds;

  toBufferSegmentNumber = 0;
  toConsumeSegmentNumber = 0;
}

bool MultimediaBuffer::addToBuffer(unsigned int segmentNumber, const dash::mpd::IRepresentation* usedRepresentation)
{
  //check if we receive a segment with a too large number
  if(toBufferSegmentNumber < segmentNumber)
    return false;

  //determine segment duration
  double duration = (double) usedRepresentation->GetSegmentList()->GetDuration();
  duration /= (double) usedRepresentation->GetSegmentList()->GetTimescale ();

  //check if segment fits in buffer
  if(isFull(duration))
    return false;

  // Check if segment has depIds
  if(usedRepresentation->GetDependencyId ().size() > 0)
  {
    // if so find the corrent map
    MBuffer::iterator it = buff.find (segmentNumber);
    if(it == buff.end ())
      return false;

    BufferRepresentationEntryMap map = it->second;

    for(std::vector<std::string>::const_iterator it = usedRepresentation->GetDependencyId ().begin ();
        it !=  usedRepresentation->GetDependencyId ().end (); it++)
    {
      //depId not found we can not add this layer
      if(map.find (*it) == map.end ());
         return false;
    }
  }

  // Add segment to buffer

  fprintf(stderr, "Inserted something for Segment %d in Buffer\n", segmentNumber);

  MultimediaBuffer::BufferRepresentationEntry entry;
  entry.repId = usedRepresentation->GetId ();
  entry.segmentDuration = duration;
  entry.segmentNumber = segmentNumber;
  entry.depIds = usedRepresentation->GetDependencyId ();
  entry.bitrate_bit_s = usedRepresentation->GetBandwidth ();

  buff[segmentNumber][usedRepresentation->GetId ()] = entry;
  toBufferSegmentNumber++;
  return true;
}

bool MultimediaBuffer::enoughSpaceInBuffer(unsigned int segmentNumber, const dash::mpd::IRepresentation* usedRepresentation)
{
  //determine segment duration
  double duration = (double) usedRepresentation->GetSegmentList()->GetDuration();
  duration /= (double) usedRepresentation->GetSegmentList()->GetTimescale ();

  if(isFull(duration))
    return false;

  return true;
}

bool MultimediaBuffer::isFull(double additional_seconds)
{
  if(maxBufferedSeconds < additional_seconds+getBufferedSeconds())
    return true;
  return false;
}

bool MultimediaBuffer::isEmpty()
{
  if(getBufferedSeconds() <= 0.0)
    return true;
  return false;
}


double MultimediaBuffer::getBufferedSeconds()
{
  double bufferSize = 0.0;

  for(MBuffer::iterator it = buff.begin (); it != buff.end (); ++it)
  {
   // actually dont need this second check...
   /*for(BufferRepresentationEntryMap::iterator k = it->second.begin(); k != it->second.end(); ++k)
    {
      if(k->second.depIds.size() == 0)
      {
        bufferSize += k->second.segmentDuration;
        break;
      }
    }*/
    bufferSize += it->second.begin()->second.segmentDuration;
  }
  fprintf(stderr, "BufferSize = %f\n", bufferSize);
  return bufferSize;
}

MultimediaBuffer::BufferRepresentationEntry MultimediaBuffer::consumeFromBuffer()
{
  BufferRepresentationEntry entryConsumed;

  if(isEmpty())
    return entryConsumed;

  MBuffer::iterator it = buff.find (toConsumeSegmentNumber);
  if(it == buff.end ())
  {
    fprintf(stderr, "Could not find SegmentNumber. This should never happen\n");
    return entryConsumed;
  }

  entryConsumed = getHighestConsumableRepresentation(toConsumeSegmentNumber);
  buff.erase (it);
  toConsumeSegmentNumber++;
  return entryConsumed;
}

MultimediaBuffer::BufferRepresentationEntry MultimediaBuffer::getHighestConsumableRepresentation(int segmentNumber)
{
  BufferRepresentationEntry consumableEntry;

  // find the correct entry map
  MBuffer::iterator it = buff.find (segmentNumber);
  if(it == buff.end ())
  {
    return consumableEntry;
  }

  BufferRepresentationEntryMap map = it->second;

  //find entry with most depIds.
  unsigned int most_depIds = 0;
  for(BufferRepresentationEntryMap::iterator k = map.begin (); k != map.end (); ++k)
  {
    if(most_depIds <= k->second.depIds.size())
    {
      consumableEntry = k->second;
      most_depIds = k->second.depIds.size();
    }
  }
  return consumableEntry;
}
