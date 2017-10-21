Name 
====

kcptun-asio -- A Secure Tunnel Based On KCP with N:M Multiplexing  
kcptun-asio is based on C++11 and Asio, fully compatible with [kcptun(go)](https://github.com/xtaci/kcptun)  

Synopsis
========

```
$ ./kcptun_client -l :6666 -r xx:xx:xx:xx:yy --key password --crypt aes --mtu 1200 --ds 20 --ps 10 --nocomp
$ ./kcptun_server -l :7777 -t xx:xx:xx:xx:yy --key password --crypt aes --mtu 1200 --ds 20 --ps 10 --nocomp
```

Features
========

* reliable data transfering based on kcp protocol  
* support aes*/xor/xtea/none/cast5/blowfish/twofish/3des/salsa20 encryption  
* multiplexing  
* snappy streaming compression and decompression,based on [google/snappy](https://github.com/google/snappy).The data frame format is [frame_format](https://github.com/google/snappy/blob/master/framing_format.txt)  
* forward error correction   

Build
=====

Prerequisites
-------------

1. [asio](https://github.com/chriskohlhoff/asio)
2. [cryptopp](https://github.com/weidai11/cryptopp)
3. [snappy](https://github.com/google/snappy)

Unix-like system
----------------
1. Get the newest code  
You should clone from github directly and update the submodules.
```
$ git clone https://github.com/ccsexyz/kcptun-asio.git  
$ cd kcptun-asio  
$ git submodule update --init --recursive
```
2. Build libcryptopp.a  
```
$ cd cryptopp
$ make clean 
$ make libcryptopp.a  
```
3. Build libsnappy.a
```
$ cd snappy
$ cmake .
$ make 
```
4. Build kcptun_client & kcptun_server  
```
$ cmake .
$ make  
```

odd Windows
-----------

Fuck MSVC.
 
TODO   
====

* performance optimization(memory optimization & CPU optimization)   
* improve smux   
