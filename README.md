# SocialMedia
## 基于单epoll+线程池，Reactor模型的web服务器，旨在实现一个小型社交媒体

目前实现了tcp连接以及初始登陆页面的展示

下一步准备增加登录注册相关接口

### 拉取该项目
``` bash
git clone https://github.com/doubleJpirate/SocialMedia.git
```

### 运行环境
Linux系统
g++编译器

### 其他配置
暂无

### 编译与运行
```bash
g++ -std=c++11 *.cpp -o text -pthread
./text
```

### 参考项目
Reator模型参考 https://github.com/shangguanyongshi/WebFileServer