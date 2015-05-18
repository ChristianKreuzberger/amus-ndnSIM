amus-ndnSIM
===========

This is a custom version of ndnSIM 2.0, supporting Adaptive Multimedia Streaming and the BRITE extension.
Please refer to the original [ndnSIM github repository](http://github.com/named-data/ndnSIM) for documentation about
ndnSIM and ns-3.

Adaptive Multimedia Streaming Simulator for NDN is trying to create a bridge between the multimedia and NDN community.
Therefore it is providing several scenarios and the source code for adaptive streaming with NDN, based on ndnSIM and
libdash.

---------------------------------------------

## Installing Procedure:

* Install Pre-Requesits for ndn-cxx, ns-3, BRITE
* Install Pre-Requesits for libdash
* Clone git repositories
* Download Brite repository
* Build Brite
* Build ndn-cxx
* Build libdash
* Build ns-3 with amus-ndnSIM


```bash
	# install pre-requesits for ndn-cxx, ns-3, etc...
	sudo apt-get install git
	sudo apt-get install python-dev python-pygraphviz python-kiwi
	sudo apt-get install python-pygoocanvas python-gnome2
	sudo apt-get install python-rsvg ipython
	sudo apt-get install build-essential
	sudo apt-get install libsqlite3-dev libcrypto++-dev
	sudo apt-get install libboost-all-dev
	# install pre-requesits for libdash
	sudo apt-get install git-core cmake libxml2-dev libcurl4-openssl-dev
	# install mercurial for BRITE
	sudo apt-get install mercurial

	mkdir ndnSIM
	cd ndnSIM

	# clone git repositories for ndn/ndnSIM
	git clone https://github.com/named-data/ndn-cxx.git ndn-cxx
	git clone https://github.com/cawka/ns-3-dev-ndnSIM.git ns-3
	git clone https://github.com/cawka/pybindgen.git pybindgen
	git clone https://github.com/ChristianKreuzberger/amus-ndnSIM.git ns-3/src/ndnSIM
	git clone https://github.com/bitmovin/libdash.git

	# download and built BRITE
	hg clone http://code.nsnam.org/BRITE
	ls -la
	cd BRITE
	make
	cd ..

	# build ndn-cxx
	cd ndn-cxx
	./waf configure
	./waf
	# install ndn-cxx
	sudo ./waf install
	cd ..

	# build libdash
	cd libdash/libdash
	mkdir build
	cd build
	cmake ../
	make dash # only build dash, no need for network stuff
	cd ../../../

	# build ns-3/ndnSIM with brite and dash enabled
	cd ns-3
	./waf configure -d optimized --with-brite=/home/$USER/ndnSIM/BRITE 
	./waf
	sudo ./waf install


	# run a scenario
	./waf --run ndn-file-cbr
```


## Info about libdash
If you have libdash already installed, make sure to specify the `--with-dash` option,e .g.:

    ./waf configure -d optimized --with-brite=/home/$USER/ndnSIM/BRITE --with-dash=/path/to/libdash
    
    
Make sure that the following files and subfolders are available in `/path/to/libdash`:

* `libdash/include/libdash.h`
* `build/bin/libdash.so`

These are standard-paths as created by the makefile for libdash.


---------------------------------------------

## Scenario Template
In addition, we also provide a scenario-template. All you need to do is download and install amus-ndnSIM once, and then
continue with the [amus-ndnSIM scenario template](http://github.com/ChristianKreuzberger/amus-scenario).

---------------------------------------------

## What is MPEG-DASH?
MPEG-DASH (ISO/IEC 23009-1:2012) is a standard for adaptive multimedia streaming over HTTP connections, which is 
adapted for NDN file-transfers in this project. For more information about MPEG-DASH, please consult the following
links:

* [DASH at Wikipedia](http://en.wikipedia.org/wiki/Dynamic_Adaptive_Streaming_over_HTTP)
* [Slideshare: DASH - From content creation to consumption](http://de.slideshare.net/christian.timmerer/dynamic-adaptive-streaming-over-http-from-content-creation-to-consumption)
* [DASH Industry Forum](http://dashif.org/)



