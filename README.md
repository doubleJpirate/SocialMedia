# SocialMedia
## 基于单epoll+线程池，Reactor模型的web服务器，旨在实现一个小型社交媒体

目前实现了基本的tcp连接和简单的http报文交换

下一步准备增加社交媒体项目的页面内容

### 编译与运行
```bash
g++ -std=c++11 text.cpp threadpool.cpp tools.cpp webserver.cpp task.cpp -o text -pthread
./text