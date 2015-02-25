amus-ndnSIM
===========

This is a custom version of ndnSIM 2.0, supporting adaptive multimedia streaming and the BRITE extension.
Please refer to the original [ndnSIM github repository](http://github.com/named-data/ndnSIM) for documentation.

---------------------------------------------
Installing Procedure:

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


---------------------------------------------
ndnSIM 2.0 is a new release of [NS-3 based Named Data Networking (NDN)
simulator](http://ndnsim.net/1.0/) that went through extensive refactoring and rewriting.
The key new features of the new version:

- ndnSIM no longer re-implements basic NDN primitives and directly uses implementation from
  [ndn-cxx library (NDN C++ library with eXperimental eXtensions)](http://named-data.net/doc/ndn-cxx/)

- All NDN forwarding and management is implemented directly using source code of
  [Named Data Networking Forwarding Daemon (NFD)](http://named-data.net/doc/NFD/)

- Packet format changed to [NDN-TLV](http://named-data.net/doc/ndn-tlv/)

[ndnSIM 2.0 documentation](http://ndnsim.net)
---------------------------------------------

For more information, including downloading and compilation instruction, please refer to
http://ndnsim.net or documentation in `docs/` folder.
