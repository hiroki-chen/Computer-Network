《计算机网络》实验3报告

思路

服务器端开辟四个线程，分别命名为Thread 0，1，2，3，其中线程1，2，3共享同一块缓冲区，并有一个互斥锁lock，锁可以用。

其中线程0是生产者线程，它只负责做两件事情：

从UDP中读取数据，并写到缓冲区内，或者是创建新的连接线程（thread 1）：

```c++
std::atomic_bool reentrant_lock = 0;	// 互斥锁

thread 0:
	while (true) {
        while (reentrant_lock != 0) ; 		// wait until consumers release the lock.
        // Lock the buffer.
        reentrant_lock = 1;
        message_len = recvfrom(socket, buffer, buffer_len, ...);
        buffer += message_len;

        if (received message is SYN) {
            std::thread t(establish_connection());
            t.detach();
        }
		// Unlock the buffer.
        reentrant_lock = 0;
    }
```



线程1是消费者线程，它负责建立客户端和服务器的连接。

