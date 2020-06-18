# Chatroom
c++实现client&amp;&amp;server---聊天室项目
本项目基于kylib共享库https://blog.csdn.net/kyee/article/details/3964903
和tcputills通讯接口https://blog.csdn.net/kyee/article/details/52023381

服务端通过配置文件设置属性，打开时监听一个端口，客户端打开时发起登录请求，服务端建立一个连接对象负责于对应的客户端交互信息，服务端维护客户端连接列表和在线列表，所有消息基于自定义协议。
客户端功能：指定id给目的客户端发送消息，广播消息（即发送给其他每个客户端消息），查询当前在线列表（通过索引）。
服务端功能：查询当前在线列表（通过索引），指定id登出目的客户端，30秒剔除连接且未登录的客户端。

系统主要类图如下：
![image](https://github.com/hongxingao/Chatroom/blob/master/system_class.png)

test文件下为编译好的exe程序，list&&cyc_buffer文件下为本人实现的kylib中的读取ini文件，列表类，缓冲区类。其中列表类基于缓冲内存块https://blog.csdn.net/kyee/article/details/40373093 ，本项目所有代码均经过kylib作者的指点及本人多次修改完善，读者可尽情参考。
