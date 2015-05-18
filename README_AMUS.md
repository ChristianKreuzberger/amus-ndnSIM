# Adaptive Multimedia Streaming Simulator  - A Tutorial
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

Then, create a .cpp file with the following content in your scenario subfolder (see examples/simple-file-transfer.cpp):
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
  producerHelper.SetAttribute("ContentDirectory", StringValue("/home/ckreuz/somedata/"));
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

You can find the whole sourcecode under examples/ndn-file-simple-example2-tracers.cpp.


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

See documentation about FileConsumer for more details.


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
While there are plenty of programs out there to create MPEG-DASH streams, reproducability is a key problem. Therefore we recommend using existing datasets, and we refer to the following two datasets for MPEG-DASH Videos:
* [DASH/AVC Dataset](http://www-itec.uni-klu.ac.at/dash/?page_id=207) by Lederer et al. [[PDF]](http://www-itec.uni-klu.ac.at/bib/files/p89-lederer.pdf) (we recommend using the 2012 version)
* [DASH/SVC Dataset](http://concert.itec.aau.at/SVCDataset/) by Kreuzberger et al. [[PDF]](http://www-itec.uni-klu.ac.at/bib/files/dash_svc_dataset_v1.05.pdf)

In the following, we will provide an example for multimedia streaming with DASH/AVC and DASH/SVC, based on those two datasets. We selected the Big Buck Bunny movie to achieve compareable results.

## DASH/AVC Streaming
Download http://www-itec.uni-klu.ac.at/ftp/datasets/mmsys12/BigBuckBunny/bunny_2s/BigBuckBunny_2s_isoffmain_DIS_23009_1_v_2_1c2_2011_08_30.mpd
Download http://www-itec.uni-klu.ac.at/ftp/datasets/mmsys12/BigBuckBunny/bunny_2s/
```bash
cd ~
mkdir -p multimediaData/AVC/BBB/
cd multimediaData/AVC/BBB/

wget -r --no-parent --cut-dirs=5 --no-host --reject "index.html" http://www-itec.uni-klu.ac.at/ftp/datasets/mmsys12/BigBuckBunny/bunny_2s/
```


## DASH/SVC Streaming
Download http://concert.itec.aau.at/SVCDataset/dataset/mpd/BBB-III.mpd
Download http://concert.itec.aau.at/SVCDataset/dataset/BBB/III/segs/
```bash
cd ~
mkdir -p multimediaData/SVC/BBB/
cd multimediaData/SVC/BBB/
mkdir III
cd III
wget -r --no-parent --no-host-directories --no-directories http://concert.itec.aau.at/SVCDataset/dataset/BBB/III/segs/1080p/
```

------------------
# Part 3: Building Large Networks and Installing Multimedia Clients
BRITE



