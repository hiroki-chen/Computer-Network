# 计算机网络作业1——网络聊天室（多人）

###  姓名：陈豪斌 班级：信息安全单 学号：1911397

## 第一部分 协议设计

我们主要关注的是应用层协议的设计，例如HTTPS、微信自定义的Async消息传输协议等。

我们将消息分割为协议头（Header）和协议体（Body），其中协议体可以存放用户想要的各种信息等，规则如下：

#### 多人聊天协议

```cpp
namespace chat_room {
    typedef enum {
        // 消息为空
        EMPTY = 0,
        // 非法消息（例如magic被篡改）
        INVALID = 1,
        // 通话结束
        END = 2,
        // 客户端请求退出
        DISCONNECT = 3,
        // 正常聊天消息
        NORMAL = 4,
        // 第一次确认通信
        GREETING = 5
    } message_type;
    typedef struct ProtocolHeader {
        // 协议的版本号
     	uint8_t version;
        
        // 协议的校验魔数
        uint8_t magic;
        
        // 协议的发送者ID
        uint16_t sender_id;
        
        // 协议体中的消息长度（in byte）
        uint32_t length;
    } ProtocolHeader;
    
    typedef struct Message {
        // 发送者id
        uint16_t sender_id;
        
        // 接受者的vector（变长、可实现分组）
    	std::vector<uint16_t> receiver_id;
        
        // 消息内容
        std::string content;
        
        // 消息类型
        message_type type;
    } Message;
    
    typedef struct ProtocolMessage {
        // 协议头
        ProtocolHeader header;
        
        // 协议体
        Message message;
    } ProtocolMessage;
}
```

更为直观地来说，聊天室协议头的布局如下图所示。

![Unbenanntes Diagramm (1)](/Users/chenhaobin/Downloads/Unbenanntes Diagramm (1).svg)

其中，为了支持SOCKET的发送和接受的功能，我们需要将消息序列化和反序列成一个C类型的字符型数组，才能通过SOCKET发送出去，并根据事先约定好的规则获取消息的内容，并进行校验。下面的代码是协议头的序列化和反序列化接口。

```c++
void serialize_header(char* const data, const chat_room::Message& message)
{
    // 由于地址是按照字节对齐的，只需要移动指针即可。
    try {
         if (*data != 0 || *(data + 1) != 0 || *((uint16_t*)(data + 2)) != 0 || *(uint32_t*)(data + 4)) {
             throw std::invalid_argument("");
         } else {
             // #define DEFAULT_VERSION 	 0x1L
             *data 	     			  = DEFAULT_VERSION;
             // #define DEFAULT_MAGIC_NUMBER 0X99
             *(data + 1) 			  = DEFAULT_MAGIC_NUMBER  
             *((uint16_t*)(data + 2)) = message.sender_id;
             *((uint32_t*)(data + 4)) = (uin32_t)(sizeof(message));
         }
    } catch (const std::invalid_argument& e) {
        std::cerr << "Segmentation Fault!" << std::endl;
        std::cerr.flush();
    }
}

chat_room::ProtocolHeader deserialize_header(char* const data) {
    try {
        uint8_t version 		= *data;
        uint8_t magic   		= *(data + 1);
        
        if (magic != DEFAULT_MAGIC_VERSION) {
            throw std::invalid_argument("");
        }
        
        chat_room::ProtocolHeader header;
        memcpy(&header, data, 8);
        return header;
    } catch (const std::invalid_argument& e) {
        std::cerr << "Invalid message header!" << std::endl;
        std::cerr.flush();
    }
}
```

我们也要将消息进行序列化，这部分的序列化并非是序列化成一个字符型数组，而是通过字符串的方式进行序列化。例如，消息`chat_room::Message`中的数组`vector`会通过逗号（`,`）拼接。如下所示。

```json
---------------------------------------------
"raw": "NORMAL_4_1,2,3,4,5_Hello What is your name???"
---------------------------------------------
{	"description": "ProtocolBody",
	"type": "NORMAL",
	"sender_id": 4,
	"receiver_id": [1,2,3,4,5],
	"content": "Hello What is your name???"
}
```

此处只需要使用开源的`rapidjson`等C++库即可实现，本程序为了实现方便，直接采用了下划线的序列化方式。该消息的含义为：

*   消息类型为正常聊天内容；
*   发送者的UID是4；
*   TA需要将消息发送给小组内的所有人（1，2，3，4，5）；
*   消息内容是“Hello What is your name???”

## 第二部分 聊天程序设计

### 总体概览

*   SOCKET: 本聊天室程序采用了流式SOCKET设计，传输层协议采用的是TCP/IP协议，并采用IPv4作为基本IP地址版本。因此创建socket可以用以下代码实现：

    ```c++
    SOCKET s = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ```

*   基本角色：

    *   服务器（Server）：主要是充当连接功能，它会在自身内部创建一个流式SOCKET，并绑定到对应的IP地址和对应的端口进行监听（循环监听线程），一旦有新的连接进入，它就创建一个新的线程来处理对应的连接请求。其中该新线程能够不停地从客户端接受消息，处理后广播给所有的接受者对应的socket。服务器的内部结构如下图所示。

        ![Unbenanntes Diagramm (2)](/Users/chenhaobin/Downloads/Unbenanntes Diagramm (2).svg)

    *   客户端（Client）：客户端主要是做发送信息和接受信息的功能，为了防止读和写将控制端阻塞，我们将其分为两个线程，一个线程监听服务器发送的消息，一个线程监听输入流，并转发给服务器。

    最后，架构图如下所示（下图为多人版本的）。

    ![Unbenanntes Diagramm (5)](/Users/chenhaobin/Downloads/Unbenanntes Diagramm (5).svg)

## 客户端发送消息的基本流程

1.   客户端：创建一个流式socket，并将此socket连接到给定的IP地址和port：

     ```c++
     SOCKET client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
     if (connect(client_socket, (SOCKADDR*)(&address)), sizeof(address) != SOCKET_ERROR) {
         // Step 2
     }
     ```

2.   服务器：监听线程获取到新的socket连接，将其添加到服务器端的客户端列表中：

     ```c++
     // In thread listen: 精简版
     std::thread listen_thread(listen);
     listen_thread.join();
     
     void chat_room::Server::wait_for_new_connections(void)
     {
         while (true) {
             SOCKET client = SOCKET_ERROR;
             while ((client = accept(server_socket, NULL, NULL)) == SOCKET_ERROR);
             
             client_sockets.push_back(client);
             
             // 创建新线程
         }
     }
     ```

3.   服务器：给每个新连接的客户端创建对应的处理线程。

     ```c++
     void chat_room::Server::wait_for_new_connections(void)
     {
     	try {
     		SOCKET client = SOCKET_ERROR;
     		while (client = accept(master_socket, NULL, NULL)) {
     			client_sockets.push_back(client);
     			// Create a new thread.
     			std::thread single_handler([this, client] { this->handle_connection(client); });
     			single_handler.detach();
     
     			std::cout << "Detected a new client! Welcome :)" << std::endl;
     			greet();
     		}
     
     		if (client_sockets.size() > MAXIMUM_ALLOWED_CLIENTS) {
     			throw std::runtime_error("TOO MANY CLIENTS");
     		}
     	} catch (const std::runtime_error& e) {
     		//std::cout << client << std::endl;
     		std::cout << get_error_message(e, WSAGetLastError()) << std::endl;
     		WSACleanup();
     	}
     
     	Sleep(100);
     }
     ```

     4.   服务器：向客户端发送greet消息

     5.   客户端：接收到greet消息后向服务器再次返回greet消息；

     6.   服务器：接收到greet消息后客户端已经确认在线，可以进行聊天了。

     7.   客户端：通过写线程发送任意消息；

     8.   服务器：

          1.   通过监听该客户端的线程得到消息；
          2.   反序列化消息得到协议头和协议体；
          3.   检查消息有效性，若有效则从对应的客户端socket队列中取出聊天室所在group的socket；
          4.   调用广播函数，对所有的对应线程发送消息；

     9.   客户端：接收到消息并打印在控制台上。

          ![Unbenanntes Diagramm (6)](/Users/chenhaobin/Downloads/Unbenanntes Diagramm (6).svg)



## 第三部分 程序使用和测试



#### 多人聊天测试

1.   打开服务器；

     ![Bildschirmfoto 2021-10-11 um 12.48.09 PM](/Users/chenhaobin/Library/Application Support/typora-user-images/Bildschirmfoto 2021-10-11 um 12.48.09 PM.png)

2.   打开客户端并制定小组有哪些人和自己的uid，例如1, 2  0;    0, 2 1;   0, 1 2![Bildschirmfoto 2021-10-11 um 12.48.39 PM](/Users/chenhaobin/Library/Application Support/typora-user-images/Bildschirmfoto 2021-10-11 um 12.48.39 PM.png)

3.   效果如下，还支持自己的文字高亮以及时间显示：

     ![Bildschirmfoto 2021-10-11 um 12.55.42 PM](/Users/chenhaobin/Library/Application Support/typora-user-images/Bildschirmfoto 2021-10-11 um 12.55.42 PM.png)

## 第四部分 总结

1.   我学会了如何使用多线程编程；
2.   我了解了socket 编程和应用层协议的部分技巧；
3.   我知道了计算机网络的基本工作流程。