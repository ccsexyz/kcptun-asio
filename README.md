# kcptun-asio

一个基于 ASIO/C++11 实现的 kcptun,将与 [kcptun(golang)](https://github.com/xtaci/kcptun) 完全兼容  

# 目前进度  

已实现 kcp 数据收发,aes 加密, smux 多路复用  
可以与关闭 FEC 和　snappy 压缩的使用 aes 加密的服务端配合使用    

# TODO  

* 实现服务端逻辑  
* 实现命令行参数解析和 json 配置文件读取  
* 实现 FEC   
* ~~实现 snappy 数据流压缩~~  
* 实现更多的加密方式  
* 实现 smux keepalive  
* 性能优化  
