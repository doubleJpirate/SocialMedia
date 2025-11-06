# SocialMedia
### 项目简介
基于单epoll+线程池，Reactor模型的web服务器，旨在实现一个小型社交媒体

### 开发情况
（目前版本几乎没有错误处理）  
目前实现了tcp连接以及初始登陆页面的展示  
下一步准备增加登录注册相关接口  

### 技术栈
**网络模型** ：Reactor 模型  
**I/O多路复用** ：epoll  
**并发处理** ：线程池（生产者 - 消费者模型）  
**开发语言** ：C++（使用 C++11 标准）  
**前端页面** ：HTML + Tailwind CSS + JavaScript（登录 / 注册页面）  

### 整体流程
目前项目main函数位于text.cpp  
首先对线程池进行初始化  
然后对套接字和epoll进行初始化，随后进入正式的通信过程  
由epoll监听i/o事件  
检测到事件后将事件丢进线程池  
线程池中完成对接收信息的解析和分发以及发送响应数据  

**task文件** ：封装一个任务多态类，用于统一接口，主要的工作函数都在这里  
**threadpool文件** ：封装线程池，本项目需要多个文件访问同一个线程池，这里使用命名空间封装全局函数和全局变量实现  
**tool文件** ：包含多个类需要使用的方法  
**webserver文件** ：封装了服务器通信函数，主要的通信逻辑都在这里  

### 拉取该项目
``` bash
git clone https://github.com/doubleJpirate/SocialMedia.git
```
### 测试方式(测试时请务必保证网络通畅)
http://[服务器IP地址]:19200

### 运行环境
Linux系统
g++编译器

### 其他配置
默认端口号为19200

### 编译与运行
```bash
g++ -std=c++11 *.cpp -o text -pthread
./text
```

### 参考项目
Reactor模型参考 https://github.com/shangguanyongshi/WebFileServer

### 其他
前端代码由AI生成
