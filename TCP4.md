## 《计算机网络》实验作业3-4——基于UDP的可靠传输协议

#### 姓名：陈豪斌    学号：1911397    专业：信息安全

#### 实验内容

基于给定的实验测试环境，通过改变延迟时间和丢包率，完成下面3组性能对比实验：

（1）停等机制与滑动窗口机制性能对比；

（2）滑动窗口机制中不同窗口大小对性能的影响；

（3）有拥塞控制和无拥塞控制的性能比较。



##### 三者性能对比

![Bildschirmfoto 2021-12-13 um 9.54.24 PM](/Users/chenhaobin/Library/Application Support/typora-user-images/Bildschirmfoto 2021-12-13 um 9.54.24 PM.png)

![Bildschirmfoto 2021-12-13 um 10.02.54 PM](/Users/chenhaobin/Library/Application Support/typora-user-images/Bildschirmfoto 2021-12-13 um 10.02.54 PM.png)

上述两组实验对三种机制都进行了比较，明显可以看出停等机制的效率是最低的。在丢包率一致的情况下，滑动窗口和拥塞窗口的效率不分彼此；而在时延较为冗长的情况下，拥塞窗口的效率明显是优于滑动窗口机制的。



##### 不同窗口大小测试

![Bildschirmfoto 2021-12-13 um 10.42.25 PM](/Users/chenhaobin/Library/Application Support/typora-user-images/Bildschirmfoto 2021-12-13 um 10.42.25 PM.png)

![Bildschirmfoto 2021-12-13 um 10.46.13 PM](/Users/chenhaobin/Desktop/Bildschirmfoto 2021-12-13 um 10.46.13 PM.png)

上述两组实验说明了滑动窗口大小并非越大越好，在丢包率和时延很大的情况下，大窗口的协议反而效率不如窗口容量较小的协议。推测是由于大窗口丢包严重导致重发的ACK过多，滑动窗口并不能快速推进。
