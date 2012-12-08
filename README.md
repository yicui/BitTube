This work was created beteeen 2007 and 2008, when we were searching for a P2P solution with minimum intrusion to the 
user experience. At the time, the mainstream P2P streaming solution were desktop rich applications such as PPStreaming,
PPLive, etc. But users had already spending increasing amount of time on YouTube among other web portals. So we thought
there should be some light-weight approaches that deal with transportation of the videos as plain data objects, and 
let those websites and their Flash players worry about the rest.

![BitTube architecture](https://github.com/downloads/yicui/BitTube/architecture.jpg)

Hence we created the architecture as above, the downloading stub sits on user's desktop machine and intercepts all
HTTP GET requests destined to the port(s) it listens on. It then turns to the P2P world to download the requested data
and wraps it in a HTTP response back to the requester, that's it. Also given the fact that BitTorrent was already the
de facto standard for P2P content sharing, we decided to make our software interoperable with other programs of the
[BiTorrent family](http://en.wikipedia.org/wiki/Comparison_of_BitTorrent_software), hence the name BitTube. 

Our first version was adapted from the Python source code of the original BitTorrent client, then we quickly realized
that this doesn't fit the video **streaming** scenario, where you must sequentially download the video and return 
any downloaded piece to your player ASAP. With this regard, the sophisticated "rarest first" and "tit-for-tat" policies
of BitTorrent have little relevance in our context, so we replaced it with our own C++ implementation.

![BitTube federation](https://github.com/downloads/yicui/BitTube/federation.jpg)

In today's standard, this design is still quite intrusive, First, you need to download and install our program, which 
required a great deal of trust and effort from users. We considered Firefox plugin but still couldn't get around the 
downloading part. Second, we required the content publisher to change URLs of their videos as depicted above. 
The *localhost* prefix is crucially important, otherwise our software is unable to catch the requests. The actual
URL of the video and BitTorrent tracker follow this prefix. Again, this was the best we can achieve back then. 
We used to argue that this is the ONLY thing you have to do (in exchange for A LOT of bandwidth saving).
Plus, if you don't like our tracker, feel free to use anyone's or host your own. We published a 
[paper](https://www.researchgate.net/publication/221558644_BitTube_Case_Study_of_a_Web-Based_Peer-Assisted_Video-on-Demand_System)
in 2008 with a detailed account of this project.

Then in 2009, Adobe rolled out their RTMFP protocol which changed everything. This leads to the reincarnation of BitTube,
which can be found [here](https://github.com/yicui/BitTube-live).