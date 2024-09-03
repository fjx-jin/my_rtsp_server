# my_rtsp_server
```shell
#支持点播的rtsp服务器 实现了OPTIONS DESCRIBE SETUP PLAY方法
#暂时不支持多播
#访问地址默认rtsp://127.0.0.1:8554/test
#项目使用了epoll、线程池等知识
#默认读取程序同目录下test.h264 test.aac文件
#使用方法
mkdir build
cmake ..
make
./rtsp_server
```
