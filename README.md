# Bebop CPP 2017

Last update 2017.09.13

* Reference

   - Node-bebop project: https://github.com/hybridgroup/node-bebop  
   - Parrot Developer official site:  http://developer.parrot.com/
       you can get official SDK and documents (now SDK3 is latest version): http://developer.parrot.com/docs/SDK3/
   - Video Streaming since Bebop firmware 3.3 : http://cvdrone.de/stream-bebop-video-with-python-opencv.html
       
* Done 
   - minimum message handling (same level as node-bebop project)
     e.g.) TX: Ping, Flight mode control (take-off, land),  pcmd (roll, pitch, yawrate, gaz)
           RX: Pong, FlyingStateChanged,                    GPS,  velocity,  attitude, altitude  (not verified)   
   - message-control thread : RX and TX periodic commands
          - TX socket : shared now by message control and main user thread, so mutex protected
          - RX socket : only used in message control thread 
   - video thread : rtp and video decoding using opencv 
   - main thread  :  send flight mode commands and user input 
   - Log module   :  logging all status infos into a signle file (with date extension), mutex used for mti-thread control
   
* TODO & Plan

    - ITEM 1: velocity control: feedback control of vx, vy, vz 
       
    - ITEM 2:  add vision module and thread
           the interface between video decoding thread to be defined.  e.g. call-back, pipe-queue,  etc.
           
    - ITEM 3: decrease the resolution and store video into into mp4 or avi file.
     
   
* KnownIssues

    - WIFI distance : we purchased long-distance anttena dongle but need install linux drivers for them.
    - Many unhanlded messages (doesnot seems so critical but have to clean up.)
    
<EndOfDocument>    
