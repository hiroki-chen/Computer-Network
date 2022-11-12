# 计算机网络作业2——网络聊天室（双人）

###  姓名：陈豪斌 班级：信息安全单 学号：1911397

# 说明：除了聊天协议不太一样，其他部分均和多人聊天设计的内容是一样的！！！！！！！！！！！！！

#### 双人聊天协议

```c++
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
        
        // 接受者的id
    	uint16_t receiver_id;		// <- 区别于多人的版本
        
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

#### 双人聊天测试

![Bildschirmfoto 2021-10-11 um 1.09.52 PM](/Users/chenhaobin/Library/Application Support/typora-user-images/Bildschirmfoto 2021-10-11 um 1.09.52 PM.png)

