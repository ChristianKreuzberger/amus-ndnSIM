# Adaptive Multimedia Streaming Framework for ndnSIM  - A Tutorial
**Note:** This tutorial requires you to have a working installation of [amus-ndnSIM](http://github.com/ChristianKreuzberger/amus-ndnSIM). 

Within this tutorial, we will try simple examples like file-transfers, up to building a large example with several multimedia streaming clients.

We recommend using the [amus scenario template](http://github.com/ChristianKreuzberger/amus-scenario) for this tutorial, but you can also place the examples provided here in the scratch subdirectoryof the ns-3 directory.
We further recommend strictly following all steps of this tutorial to generate a stable and soild adaptive multimedia streaming scenario. 

The content of this tutorial is organized as follows: Part 1 explains how File Transfers are organized and implemented in amus-ndnSIM. Part 2 explains how adaptive multimedia streaming can be used in amus-ndnSIM, and what datasets have been tested. Part 3 shows several neat simulations, which are based on ns-3 simulations (like WiFi, mobility models, ...). Part 4 finally shows how to generate a large network by using BRITE.

------------------
# Part 1: File Transfers
Unfortunately, ndnSIM did not provide any option for transfering files. The only implemented application that come close are the consumer and producer app (resp. ndn-app-consumer-cbr and ndn-app-producer). We implemented a FileServer (resp. ndn-app-file-producer) and a FileConsumer for basic file transfers. 
Contrary to some existing approaches, you do not need to provide content in any specific form. It is enough to just tell the FileServer that you want to host a certain directory (e.g., ~/data). The FileServer will automatically handle chunks and segmentation. Segmentation is based on the Maximum Transmission Unit (MTU), the FileServer aims to always fully utilize the MTU by automatically maximizing the payload size. This immediately leads us to a limitation: The MTU needs to be the same for the whole network.
Furthermore, the file transfer is using sequence numbers for Interests. The basic procedures works as follows: If you request ``/prefix/app/File1.txt``, then the consumer will first issue an Interest for ``/prefix/app/File1.txt/Manifest``, which contains basic information about the file. Second, the consumer will start to sequentially issue Interests for the chunks: ``/prefix/app/File1.txt/1``, ``/prefix/app/File1.txt/2``, ..., though the sequence numbers are binary encoded.
The FileServer will respond with the respective payload, and the file consumer will stop requesting once it has received the whole file. These are two important properties that the basic consumers of ndnSIM do not have.


## Hosting Content
As already mentioned, hosting content does not require any special attention. You can host content on any ndnSIM node by adding the following sourcecode:
```cplusplus
  // Producer
  ndn::AppHelper producerHelper("ns3::ndn::FileServer");

  // Producer will reply to all requests starting with /prefix
  producerHelper.SetPrefix("/myprefix");
  producerHelper.SetAttribute("ContentDirectory", StringValue("/home/username/somedata/"));
  producerHelper.Install(nodes.Get(0)); // install to some node from nodelist
```
This will make the directory ``/home/username/somedata/`` fully available under the prefix ``/myprefix``. 
You can install the producer on as many nodes you want. Just make sure to also provide routing information, the same was as you would for a ndn-producer.

```cplusplus
  ndnGlobalRoutingHelper.AddOrigins("/myprefix", nodes.Get(0));
```

## Requesting Content
Similar to the example above, the client, a FileConsumer, needs to be installed:
```cplusplus
  // Consumer
  ndn::AppHelper consumerHelper("ns3::ndn::FileConsumer");
  consumerHelper.SetAttribute("FileToRequest", StringValue("/myprefix/file1.img"));
  
   consumerHelper.Install(nodes.Get(2)); /install to some node from nodelist
```
This simple piece of code will automatically request file1.txt, similar to what ``ns3::ndn::Consumer`` would do.

## Full Example: File Transfer
First, create a data directory in your home directory, and create a file of 10 Megabyte size:
```bash
mkdir ~/somedata
fallocate -l 10M ~/somedata/file1.img
```

Then, create a .cpp file with the following content in your scenario subfolder (see [examples/ndn-file-simple-example1.cpp](examples/ndn-file-simple-example1.cpp)):
```cplusplus
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/ndnSIM/apps/ndn-app.hpp"

namespace ns3 {

int
main(int argc, char* argv[])
{
  // setting default parameters for PointToPoint links and channels
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("10Mbps"));
  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("10ms"));
  Config::SetDefault("ns3::DropTailQueue::MaxPackets", StringValue("20"));

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.Parse(argc, argv);

  // Creating nodes
  NodeContainer nodes;
  nodes.Create(3); // 3 nodes, connected: 0 <---> 1 <---> 2

  // Connecting nodes using two links
  PointToPointHelper p2p;
  p2p.Install(nodes.Get(0), nodes.Get(1));
  p2p.Install(nodes.Get(1), nodes.Get(2));

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.setCsSize(0);
  ndnHelper.SetOldContentStore("ns3::ndn::cs::Lru", "MaxSize", "100");
  ndnHelper.InstallAll();

  // Choosing forwarding strategy
  ndn::StrategyChoiceHelper::InstallAll("/myprefix", "/localhost/nfd/strategy/best-route");

  // Consumer
  ndn::AppHelper consumerHelper("ns3::ndn::FileConsumer");
  consumerHelper.SetAttribute("FileToRequest", StringValue("/myprefix/file1.img"));

  consumerHelper.Install(nodes.Get(2)); // install to some node from nodelist

  // Producer
  ndn::AppHelper producerHelper("ns3::ndn::FileServer");

  // Producer will reply to all requests starting with /prefix
  producerHelper.SetPrefix("/myprefix");
  producerHelper.SetAttribute("ContentDirectory", StringValue("/home/someuser/somedata/"));
  producerHelper.Install(nodes.Get(0)); // install to some node from nodelist

  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();

  ndnGlobalRoutingHelper.AddOrigins("/myprefix", nodes.Get(0));
  ndn::GlobalRoutingHelper::CalculateRoutes();

  Simulator::Stop(Seconds(60.0));

  Simulator::Run();
  Simulator::Destroy();

  NS_LOG_UNCOND("Simulation Finished.");

  return 0;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}


``` 

You can now compile and start the scenario using
```bash
./waf --run ndn-file-simple-example1
```
Though you will not see any output. We will talk about generating some output from file transfers in the next section. For now, if you want to verify that your file transfer is working, use the visualizer:
```bash
./waf --run ndn-file-simple-example1 --vis
```

## Tracing File Transfers
For more information about the file transfer, especially the download speed, we have implemented file transfer tracers, similar to existing tracers in ndnSIM. At first, we need to create 3 functions that will act as our output:
```cplusplus
void
FileDownloadedTrace(Ptr<ns3::ndn::App> app, shared_ptr<const ndn::Name> interestName, double downloadSpeed, long milliSeconds)
{
  std::cout << "Trace: File finished downloading: " << Simulator::Now().GetMilliSeconds () << " "<< *interestName <<
     " Download Speed: " << downloadSpeed/1000.0 << " Kilobit/s in " << milliSeconds << " ms" << std::endl;
}

void
FileDownloadedManifestTrace(Ptr<ns3::ndn::App> app, shared_ptr<const ndn::Name> interestName, long fileSize)
{
  std::cout << "Trace: Manifest received: " << Simulator::Now().GetMilliSeconds () <<" "<< *interestName << " File Size: " << fileSize << std::endl;
}

void
FileDownloadStartedTrace(Ptr<ns3::ndn::App> app, shared_ptr<const ndn::Name> interestName)
{
  std::cout << "Trace: File started downloading: " << Simulator::Now().GetMilliSeconds () <<" "<< *interestName << std::endl;
}
```

Next, we need to make sure to connect them as trace sources:
```cplusplus
  // Connect Tracers
  Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/FileDownloadFinished",
                               MakeCallback(&FileDownloadedTrace));
  Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/ManifestReceived",
                               MakeCallback(&FileDownloadedManifestTrace));
  Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/FileDownloadStarted",
                               MakeCallback(&FileDownloadStartedTrace));
```

You can find the whole sourcecode under [examples/ndn-file-simple-example2-tracers.cpp](examples/ndn-file-simple-example2-tracers.cpp).


## Enhanced File Consumer
While the basic FileConsumer works for basic testing, we recommend using the enhanced version, ```FileConsumerCbr```, which issues Interests at a much higher rate, to fully utilize the link capacity the consumer has (read: ```DownloadSpeedInBytes/MTU```).
In addition, we do not want to waste any bandwidth, therefore we also recommend issueing Interests for the file, although the manifest has not been received yet.
This can be achieved by the following code:

```cplusplus
  // Consumer
  ndn::AppHelper consumerHelper("ns3::ndn::FileConsumerCbr");
  consumerHelper.SetAttribute("FileToRequest", StringValue("/myprefix/file1.img"));
  consumerHelper.SetAttribute("StartWindowSize", StringValue("60"));
```
Full Example: [examples/ndn-file-simple-example3-enhanced.cpp](examples/ndn-file-simple-example3-enhanced.cpp).

See documentation about FileConsumer for more details.


## Comparison
We have now shown you simple examples for file transfers. It is now the time to compare the log output of those examples, by running ```ndn-file-simple-example2-tracers``` and ```ndn-file-simple-example3-enhanced```.
Furthermore, feel free to play around with ```StartWindowSize```, from 0 to 100, and see how this impacts the performance.

Here is the output from our examples:
```bash
~/ndnSIM/test-scenario$ ./waf --run ndn-file-simple-example2-tracers
Waf: Entering directory `ndnSIM/test-scenario/build'
Waf: Leaving directory `ndnSIM/test-scenario/build'
'build' finished successfully (0.057s)
Trace: File started downloading: 0 /myprefix/file1.img
Trace: Manifest received: 40 /myprefix/file1.img File Size: 10485760
Trace: File finished downloading: 307222 /myprefix/file1.img Download Speed: 273.047 Kilobit/s in 307222 ms
Simulation Finished.
~/ndnSIM/test-scenario$ ./waf --run ndn-file-simple-example3-enhanced
Waf: Entering directory `ndnSIM/test-scenario/build'
Waf: Leaving directory `ndnSIM/test-scenario/build'
'build' finished successfully (0.057s)
Trace: File started downloading: 0 /myprefix/file1.img
Trace: Manifest received: 40 /myprefix/file1.img File Size: 10485760
Trace: File finished downloading: 8723 /myprefix/file1.img Download Speed: 9616.65 Kilobit/s in 8723 ms
Simulation Finished.
```

As you can see, FileConsumerCbr performs much better, and it is the recommended Consumer to use.


## Testing File Transfers with Real Data
With FileConsumer and FileProducer we are transfering real data packets over the simulated network. For some scenarios it might be necessary to use the file transfered via the simulator. This improves the accuracy of the results. We added a method for writeing the received file to the storage with the following command:
```cplusplus
  consumerHelper.SetAttribute("WriteOutfile", StringValue("/home/username/somefile.img"));
```
Feel free to also compare the outfile with the original file by using ```m5sum``` or ```sha1sum```.


## Tracing Several File Consumers 
When considering a larger example, it might be required to log the trace output to a file for all clients. This can be easily achieved by using the ``FileConsumerLogTracer`` with the following line of code:
```
  ndn::FileConsumerLogTracer::InstallAll("file-consumer-log-trace.txt");
```


## Summary
In this chapter, we have presented the basics of file transfers in amus-ndnSIM. We can now use this for over-the-top adaptive multimedia streaming.

------------------
# Part 2: Multimedia Streaming
For multimedia streaming, we use the concept of Dynamic Adaptive Streaming over HTTP, adapted for the file transfer logic from Part 1. We make use of [libdash](http://github.com/bitmovin/libdash), an open source library for MPEG-DASH. 

## Hosting Content
Fortunately, we already have a FileServer, and this is all we need for hosting content (except for the content, obviously). We can basically use any MPEG-DASH compatible video stream, as long as the Media Presentation Description (MPD) file can be parsed by libdash (Note: we only support BASIC MPD structures, with only one period, and no wildcards).

## Getting DASH Content for Testing
While there are plenty of programs out there to create MPEG-DASH streams, reproducability of demos and simulations is a key problem. Therefore we recommend using existing datasets, and we refer to the following two datasets for MPEG-DASH Videos:

* [DASH/AVC Dataset](http://www-itec.uni-klu.ac.at/dash/?page_id=207) by Lederer et al. [[PDF]](http://www-itec.uni-klu.ac.at/bib/files/p89-lederer.pdf) [[Bib]](http://www-itec.uni-klu.ac.at/bib/index.php?key=Mueller2012&bib=itec.bib) [[Website]](http://www-itec.uni-klu.ac.at/dash/?page_id=207) (we recommend using the older 2012 version for testing purpose)
* [DASH/SVC Dataset](http://concert.itec.aau.at/SVCDataset/) by Kreuzberger et al. [[PDF]](http://www-itec.uni-klu.ac.at/bib/files/dash_svc_dataset_v1.05.pdf) [[Bib]](http://www-itec.uni-klu.ac.at/bib/index.php?key=Kreuzberger2015a&bib=itec.bib) [[Website]](http://concert.itec.aau.at/SVCDataset/)

In the following, we will provide an example for multimedia streaming with DASH/AVC and DASH/SVC, based on those two datasets. We selected the Big Buck Bunny movie to achieve compareable results.

### DASH/AVC Streaming
**Note:** This will require roughly 4 Gigabyte of diskspace
We are going to download the BigBuckBunny movie from the DASH/AVC Dataset, segment length 2 seconds.
First of all, download the video files from [here](http://www-itec.uni-klu.ac.at/ftp/datasets/mmsys12/BigBuckBunny/bunny_2s/):
```bash
cd ~
mkdir -p multimediaData/AVC/BBB/
cd multimediaData/AVC/BBB/

wget -r --no-parent --cut-dirs=5 --no-host --reject "index.html" http://www-itec.uni-klu.ac.at/ftp/datasets/mmsys12/BigBuckBunny/bunny_2s/
```
this folder should also contain the MPD file ``BigBuckBunny_2s_isoffmain_DIS_23009_1_v_2_1c2_2011_08_30.mpd``, if not, you can download the
 [MPD file](http://www-itec.uni-klu.ac.at/ftp/datasets/mmsys12/BigBuckBunny/bunny_2s/BigBuckBunny_2s_isoffmain_DIS_23009_1_v_2_1c2_2011_08_30.mpd) manually.
```bash
wget http://www-itec.uni-klu.ac.at/ftp/datasets/mmsys12/BigBuckBunny/bunny_2s/BigBuckBunny_2s_isoffmain_DIS_23009_1_v_2_1c2_2011_08_30.mpd
```
Open the MPD in an editor of your choice, and locate the ``<BaseURL>`` tag. Change
```xml
<BaseURL>http://www-itec.uni-klu.ac.at/ftp/datasets/mmsys12/BigBuckBunny/bunny_2s/</BaseURL>
```
to
```xml
<BaseURL>/myprefix/AVC/BBB/</BaseURL>
```
Furthermore, we recommend renaming the file to a name of your choice, e.g., BBB-2s.mpd.
```bash
mv BigBuckBunny_2s_isoffmain_DIS_23009_1_v_2_1c2_2011_08_30.mpd BBB-2s.mpd
```

### DASH/SVC Streaming
**Note:** This will require roughly 1 Gigabyte of diskspace
We are going to download the BigBuckBunny movie from the DASH/SVC Dataset, with a segment length of 2 seconds, and no temporal scalability. First of all, download the video files from [here](http://concert.itec.aau.at/SVCDataset/dataset/BBB/III/segs/).
```bash
cd ~
mkdir -p multimediaData/SVC/BBB/
cd multimediaData/SVC/BBB/
mkdir III
cd III
wget -r --no-parent --no-host-directories --no-directories http://concert.itec.aau.at/SVCDataset/dataset/BBB/III/segs/1080p/
```
Second, download the [MPD file](http://concert.itec.aau.at/SVCDataset/dataset/mpd/BBB-III.mpd).
```bash
wget http://concert.itec.aau.at/SVCDataset/dataset/mpd/BBB-III.mpd
```
Open the MPD in an editor of your choice, and locate the ``<BaseURL>`` tag. Change
```xml
<BaseURL>http://concert.itec.aau.at/SVCDataset/dataset/BBB/III/segs/1080p/</BaseURL>
```
to
```xml
<BaseURL>/myprefix/SVC/BBB/III/</BaseURL>
```

### Creating a Server
This is really simple:
```cplusplus
  // Producer
  ndn::AppHelper producerHelper("ns3::ndn::FileServer");

  // Producer will reply to all requests starting with /myprefix
  producerHelper.SetPrefix("/myprefix");
  producerHelper.SetAttribute("ContentDirectory", StringValue("/home/username/multimediaData/"));
  producerHelper.Install(nodes.Get(0)); // install to some node from nodelist

```
Done!

## Basics: Using the Multimedia Consumer
### AVC Content
Our multimedia consumers are built on top of the FileConsumers, therefore it is necessary to specify which FileConsumer you want. We recommend using the ``FileConsumerCbr`` class, hence you should use the following code for requesting AVC content:
```cplusplus
  ns3::ndn::AppHelper consumerHelper("ns3::ndn::FileConsumerCbr::MultimediaConsumer");
  consumerHelper.SetAttribute("AllowUpscale", BooleanValue(true));
  consumerHelper.SetAttribute("AllowDownscale", BooleanValue(false));
  consumerHelper.SetAttribute("ScreenWidth", UintegerValue(1920));
  consumerHelper.SetAttribute("ScreenHeight", UintegerValue(1080));
  consumerHelper.SetAttribute("StartRepresentationId", StringValue("auto"));
  consumerHelper.SetAttribute("MaxBufferedSeconds", UintegerValue(30));
  consumerHelper.SetAttribute("StartUpDelay", StringValue("0.1"));

  consumerHelper.SetAttribute("AdaptationLogic", StringValue("dash::player::RateAndBufferBasedAdaptationLogic"));
  consumerHelper.SetAttribute("MpdFileToRequest", StringValue(std::string("/myprefix/AVC/BBB/BBB-2s.mpd" )));

  ApplicationContainer app1 = consumerHelper.Install (nodes.Get(2));
```

See [examples/ndn-multimedia-simple-avc-example1.cpp](examples/ndn-multimedia-simple-avc-example1.cpp) for the full example.

### SVC Content
This is very similar to the AVC case, you just need to specify a different adaptation logic (and the correct MPD file):

```cplusplus
  ns3::ndn::AppHelper consumerHelper("ns3::ndn::FileConsumerCbr::MultimediaConsumer");
  consumerHelper.SetAttribute("AllowUpscale", BooleanValue(true));
  consumerHelper.SetAttribute("AllowDownscale", BooleanValue(false));
  consumerHelper.SetAttribute("ScreenWidth", UintegerValue(1920));
  consumerHelper.SetAttribute("ScreenHeight", UintegerValue(1080));
  consumerHelper.SetAttribute("StartRepresentationId", StringValue("auto"));
  consumerHelper.SetAttribute("MaxBufferedSeconds", UintegerValue(30));
  consumerHelper.SetAttribute("StartUpDelay", StringValue("0.1"));

  consumerHelper.SetAttribute("AdaptationLogic", StringValue("dash::player::SVCBufferBasedAdaptationLogic"));
  consumerHelper.SetAttribute("MpdFileToRequest", StringValue(std::string("/myprefix/SVC/BBB/BBB-III.mpd" )));

  ApplicationContainer app1 = consumerHelper.Install (nodes.Get(2));
```


See [examples/ndn-multimedia-simple-svc-example1.cpp](examples/ndn-multimedia-simple-svc-example1.cpp) for the full example.


## Multimedia Consumers Options
Our Multimedia Consumers have plenty of options to be configured. First of all, the two most important ones are 

 * ``AdaptationLogic`` 
 *  ``MpdFileToRequest``

``MpdFileToRequest`` is, like the name suggests, the MPD file of the video to play. For the purpose of having a quicker start-up, it is possible to use gzip'ed MPD files here, by specifying a file like this: ``mpdfilename.mpd.gz``(the file obviously needs to be available in the directory).
``AdaptationLogic``  depends heavily on the MPD file. If the MPD file contains SVC content, the following adaptation logics are available: 

 * ``SVCBufferBasedAdaptationLogic``
 * ``SVCRateBasedAdaptationLogic``
 * ``SVCNoAdaptationLogic`` - requests all SVC representations, starting from the base layer, until the segment needs to be consumed
 * ``AlwaysLowestAdaptationLogic`` (default value) - use always the lowest representation available (warning: this might not work in all cases for SVC content)

For AVC:

 * ``AlwaysLowestAdaptationLogic`` (default value) - use always the lowest representation available
 * ``RateBasedAdaptationLogic``- the estimated throughput is the main deciding factor for the representation used
 * ``RateAndBufferBasedAdaptationLogic`` - the clients local buffer and the estimated throughput will be used for determining the representation



Then we have several options that the clients can use for their "simulated screens":

 * ``ScreenWidth`` and ``ScreenHeight``
 * ``AllowUpscale`` and ``AllowDownscale``

Those 4 options are used to determine which representations from the MPD file are unusable for the client. For example, if you have a mobile phone with a 1280x720 screen, you should not request a representation with 1920x1080 and downscale the video.
Instead, you can request the 640x320 representation and upscale it to 1280x720 or request the 1280x720 representation and use it without scaling. For some scenarios it might even be necessary to disable upscaling too.


 * ``MaxBufferedSeconds`` (default: 30)
 * ``StartUpDelay``(default: 2.0)

Those options are used to determine the maximum amount of buffered seconds. While on most computers, this number could be rather large, especially with mobile phones and tablets this number is restricted by the available memory. Common practice for those is around 30 seconds.
The ``StartUpDelay`` is used for scenarios, where you want the client to buffer first, and start playing after a certain amount of time. This could beneficial for scenarios where you have a slow Internet connection, as it is better to buffer first, and then play. If set to 0, the player will start playing the video as soon as the first segment has been successfully downloaded.

 * ``StartRepresentationId`` (default: "auto")

Can take the following values: "lowest", "auto" (means: use adaptation logic) or a certain representation id. This attribute is for testing purpose, and we recommend using "auto".

## Multimedia Consumers and Tracers
For evaluation purpose, we have a special tracer for multimedia consumers available. This tracer logs the following events:

 * Segment consumed at which time with representation id and representation bitrate
 * Video playback stall (freeze)
 * Start-up delay

Example:
```cplusplus
// include the header file
#include "ns3/ndnSIM/utils/tracers/ndn-dashplayer-tracer.hpp"

// ...
int
main(int argc, char* argv[])
{
  // ...
  // install the tracer
  ndn::DASHPlayerTracer::InstallAll("dash-output.txt");

  Simulator::Run();
  Simulator::Destroy();
  // ...
}
```

The output will be a CSV file called dash-output.txt, and look like this (for SVC content)
```
Time  Node  SegmentNumber   SegmentDuration(sec)  SegmentRepID SegmentBitrate(bit/s)  StallingTime(msec) SegmentDepIds
0.42821 2       0                 2                        0               624758                  0
2.42821 2       1                 2                        0               624758                  0
...
18.4282 2       9                 2                        0               624758                  0
20.4282 2       10                2                        1               2122081                 0       0
...
26.4282 2       13                2                        1               2122081                 0       0
28.4282 2       14                2                        2               5108358                 0       0,1
30.4282 2       15                2                        2               5108358                 0       0,1
32.4282 2       16                2                        2               5108358                 0       0,1
34.4282 2       17                2                        2               5108358                 0       0,1
36.4282 2       18                2                        3               9885231                 0       0,1,2
38.4282 2       19                2                        3               9885231                 0       0,1,2
...
```
AVC content will look similar, but will not have the SegmentDepIds filled at all.

See examples/ndn-multimedia-simple-avc-example2-tracer.cpp and See examples/ndn-multimedia-simple-svc-example2-tracer.cpp for the full sourcecode.

### Other Tracers
For evaluation purposes most other tracers should also work, for instance, Content Store Tracer. Please note, that due to some limitations with ns-3/ndnSIM, the AppId of the MultimediaConsumer will change over time, therefore you need to filter the Node (```NodeId```), instead of ```AppId``` in all traces.

See [examples/ndn-multimedia-simple-avc-example2-tracers.cpp](examples/ndn-multimedia-simple-avc-example2-tracers.cpp) and [examples/ndn-multimedia-simple-svc-example2-tracers.cpp](examples/ndn-multimedia-simple-svc-example2-tracers.cpp) for the full example.

------------------

# Part 3: Building Large Networks with BRITE and Installing Multimedia Clients
## Basics
The [BRITE Network Generator](http://www.cs.bu.edu/brite/) is an open source project, which can be used by ndnSIM/ns-3. We have set up a wrapper class (see [helper/ndn-brite-topology-helper.cpp](helper/ndn-brite-topology-helper.cpp) and  [helper/ndn-brite-topology-helper.hpp](helper/ndn-brite-topology-helper.hpp)). All you need is a brite config file.



```cplusplus
// include the header file
#include "ns3/ndnSIM/helper/ndn-brite-topology-helper.hpp"
// ...

  // Create Brite Topology Helper
  ndn::NDNBriteTopologyHelper bth ("brite.conf");
  bth.AssignStreams (3);
  // tell the topology helper to build the topology
  bth.BuildBriteTopology ();

// ...

```

You can find the full example later. 

## Random Network
Use the ``--RngRun=`` option of [ns-3 Random Variables](https://www.nsnam.org/docs/manual/html/random-variables.html#id1) to generate different instances of the random network.

Example:
```bash
./waf --run "ndn-multimedia-brite-example1 --RngRun=0" --vis
./waf --run "ndn-multimedia-brite-example1 --RngRun=1" --vis
./waf --run "ndn-multimedia-brite-example1 --RngRun=2" --vis
```


## Installing the NDN Stack, Multimedia Clients, Routing, ...
First, we include the ndn brite helper and configure the application to have a parameter for the brite config file:

```cplusplus
#include "ns3/ndnSIM/helper/ndn-brite-topology-helper.hpp"
```

```cplusplus
int
main(int argc, char* argv[])
{
  std::string confFile = "brite.conf";

  CommandLine cmd;
  cmd.AddValue ("briteConfFile", "BRITE configuration file", confFile);
  cmd.Parse(argc, argv);
```

Next, we create an instance of the NDN stack and build the topology:

```cplusplus
  // Create NDN Stack
  ndn::StackHelper ndnHelper;

  // Create Brite Topology Helper
  ndn::NDNBriteTopologyHelper bth (confFile);
  bth.AssignStreams (3);
  // tell the topology helper to build the topology 
  bth.BuildBriteTopology ();
```

Now we go through that topology and create a ```NodeContainer``` for servers, clients and routers, so we can distingiush the configuration of those nodes.

```cplusplus
  // Separate clients, servers and routers
  NodeContainer client;
  NodeContainer server;
  NodeContainer router;
```

Iterate over all AS (Autonomous System) and get all non leaf nodes
```cplusplus
  for (uint32_t i = 0; i < bth.GetNAs(); i++)
  {
    std::cout << "Number of nodes for AS: " << bth.GetNNodesForAs(i) << ", non leaf nodes: " << bth.GetNNonLeafNodesForAs(i) << std::endl;
    for(int node=0; node < bth.GetNNonLeafNodesForAs(i); node++)
    {
      std::cout << "Node " << node << " has " << bth.GetNonLeafNodeForAs(i,node)->GetNDevices() << " devices " << std::endl;
      //container.Add (briteHelper->GetNodeForAs (ASnumber,node));
      router.Add(bth.GetNonLeafNodeForAs(i,node));
    }
  }
```

Iterate over all AS and get all leaf nodes
```cplusplus
  uint32_t sumLeafNodes = 0;
  for (uint32_t i = 0; i < bth.GetNAs(); i++)
  {
    uint32_t numLeafNodes = bth.GetNLeafNodesForAs(i);

    std::cout << "AS " << i << " has " << numLeafNodes << "leaf nodes! " << std::endl;

    for (uint32_t j= 0; j < numLeafNodes; j++)
    {
      if (decideIfServer(i,j)) // some decision, can be random
      {
        server.Add(bth.GetLeafNodeForAs(i,j));
      }
      else
      {
        client.Add(bth.GetLeafNodeForAs(i,j));
      }
    }

    sumLeafNodes+= numLeafNodes;
  }

  std::cout << "Total Number of leaf nodes: " << sumLeafNodes << std::endl;

```
Where ``decideIfServer(asNumber, leafNumber)`` needs to have some logic to decide whether this node should be a client or a server. Generally, a random strategy works well, you just need to make sure you know how many servers and clients you want to have (in relation).


Now let's isntall the ndn stack and content stores:
```cplusplus
  // clients do not really need a large content store, but it could be beneficial to give them some
  ndnHelper.SetOldContentStore ("ns3::ndn::cs::Stats::Lru","MaxSize", "100");
  ndnHelper.Install (client);

  // servers do not need a content store at all, they have an app to do that
  ndnHelper.SetOldContentStore ("ns3::ndn::cs::Stats::Lru","MaxSize", "1");
  ndnHelper.Install (server);

  // what really needs a content store is the routers, which we don't have many
  ndnHelper.setCsSize(10000);
  ndnHelper.Install(router);
```
and choose a forwarding strategy
```cplusplus
  // Choosing forwarding strategy
  ndn::StrategyChoiceHelper::InstallAll("/myprefix", "/localhost/nfd/strategy/best-route");
```

Install the GlobalRoutingHelper
```cplusplus
  // Installing global routing interface on all nodes
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();
```

Install the Multimedia Consumer (in this case an SVC consumer)
```cplusplus
  // Installing multimedia consumer
  ns3::ndn::AppHelper consumerHelper("ns3::ndn::FileConsumerCbr::MultimediaConsumer");
  consumerHelper.SetAttribute("AllowUpscale", BooleanValue(true));
  consumerHelper.SetAttribute("AllowDownscale", BooleanValue(false));
  consumerHelper.SetAttribute("ScreenWidth", UintegerValue(1920));
  consumerHelper.SetAttribute("ScreenHeight", UintegerValue(1080));
  consumerHelper.SetAttribute("StartRepresentationId", StringValue("auto"));
  consumerHelper.SetAttribute("MaxBufferedSeconds", UintegerValue(30));
  consumerHelper.SetAttribute("StartUpDelay", StringValue("0.1"));

  consumerHelper.SetAttribute("AdaptationLogic", StringValue("dash::player::SVCBufferBasedAdaptationLogic"));
  consumerHelper.SetAttribute("MpdFileToRequest", StringValue(std::string("/myprefix/SVC/BBB/BBB-III.mpd" )));
```

Start clients / install logic
```cplusplus
  // Randomize Client File Selection
  Ptr<UniformRandomVariable> r = CreateObject<UniformRandomVariable>();
  for(int i=0; i<client.size (); i++)
  {
    // TODO: Make some logic to decide which file to request
    consumerHelper.SetAttribute("MpdFileToRequest", StringValue(std::string("/myprefix/SVC/BBB/BBB-III.mpd" )));
    ApplicationContainer consumer = consumerHelper.Install (client[i]);

    std::cout << "Client " << i << " is Node " << client[i]->GetId() << std::endl;

    // Start and stop the consumer
    consumer.Start (Seconds(1.0)); // TODO: Start at randomized time
    consumer.Stop (Seconds(600.0));
  }
```

Install the servers
```cplusplus
   // Producer
  ndn::AppHelper producerHelper("ns3::ndn::FileServer");

  // Producer will reply to all requests starting with /myprefix
  producerHelper.SetPrefix("/myprefix");
  producerHelper.SetAttribute("ContentDirectory", StringValue("/home/someuser/multimediaData/"));
  producerHelper.Install(server); // install to servers

  ndnGlobalRoutingHelper.AddOrigins("/myprefix", server);
```

Install Routing:
```cplusplus
  // Calculate and install FIBs
  ndn::GlobalRoutingHelper::CalculateAllPossibleRoutes();
```

Run the Simulation
```cplusplus
  Simulator::Stop(Seconds(1000.0));
  Simulator::Run();
  Simulator::Destroy();

  std::cout << "Simulation ended" << std::endl;

  return 0;
}
```


Full example here: [examples/ndn-multimedia-brite-example1.cpp](examples/ndn-multimedia-brite-example1.cpp).
Brite config File: TODO

------------------

# Part 4: Use Cases/Scenarios and Examples
Not ready yet...



