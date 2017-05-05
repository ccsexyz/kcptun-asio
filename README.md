# kcptun-asio

一个基于 ASIO/C++11 实现的 kcptun,将与 [kcptun(golang)](https://github.com/xtaci/kcptun) 完全兼容  

# 目前进度  

* kcp 协议数据收发  
* kcptun 支持的所有加密方式(aes*/xor/xtea/none/cast5/blowfish/twofish/3des/salsa20)  
* smux 多路复用  
* snappy 流数据压缩与解压缩  

已实现客户端,能够兼容不使用 FEC 的 kcptun 服务端  

# TODO  

* 实现服务端逻辑  
* 实现命令行参数解析和 json 配置文件读取  
* 实现 FEC   
* ~~实现 snappy 数据流压缩~~  
* ~~实现更多的加密方式~~  
* ~~实现 smux keepalive~~  
* 性能优化  
