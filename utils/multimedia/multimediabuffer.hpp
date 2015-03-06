#ifndef MULTIMEDIABUFFER_HPP
#define MULTIMEDIABUFFER_HPP

#include <map>
#include <vector>
#include "libdash/libdash.h"

namespace dash
{
namespace player
{

class MultimediaBuffer
{
public:

  struct BufferRepresentationEntry
  {
    unsigned int segmentNumber;
    double segmentDuration;
    unsigned int bitrate_bit_s;
    std::vector<std::string>  depIds;
    std::string repId;

    BufferRepresentationEntry()
    {
      segmentNumber = 0;
      segmentDuration = 0.0;
      bitrate_bit_s = 0.0;
      repId = "InvalidSegment";
    }

    BufferRepresentationEntry(const BufferRepresentationEntry& other)
    {
      repId = other.repId;
      segmentNumber = other.segmentNumber;
      segmentDuration = other.segmentDuration;
      bitrate_bit_s = other.bitrate_bit_s;
      depIds = other.depIds;
    }
  };

  MultimediaBuffer(unsigned int maxBufferedSeconds);

  bool addToBuffer(unsigned int segmentNumber, const dash::mpd::IRepresentation* usedRepresentation);
  bool enoughSpaceInBuffer(unsigned int segmentNumber, const dash::mpd::IRepresentation* usedRepresentation);
  BufferRepresentationEntry consumeFromBuffer();
  bool isFull(double additional_seconds = 0.0);
  bool isEmpty();
  double getBufferedSeconds();

protected:
  double maxBufferedSeconds;

  unsigned int toBufferSegmentNumber;
  unsigned int toConsumeSegmentNumber;

  typedef std::map
  <std::string, /*Representation*/
  BufferRepresentationEntry
  >BufferRepresentationEntryMap;

  typedef std::map
  <int, /*SegmentNumber*/
  BufferRepresentationEntryMap
  >MBuffer;

  MBuffer buff;

  BufferRepresentationEntry getHighestConsumableRepresentation(int segmentNumber);

};
}
}

#endif // MULTIMEDIABUFFER_HPP
