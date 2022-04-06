# SocketServerC++

#### 介绍
- 基于C/C++底层socket实现并发网络服务器
- 主要实现了HttpServer，能应付上百并发量(当然了，你要是想应付成千上万当然也行)
- 该服务器尚不稳定，服务器运行时有可能会出现(Segmentation fault)错误，目前并未找到错误原因
- 我猜测是设计漏洞，应该是在调用动态绑定的处理函数时本应能调用的函数出了点函数赋值时的什么未知原因导致赋值失败
- 当然了，以上只是我的猜测，我并没有能DeBug证明之
- 在此开源我写的服务器1.0版本，因为运行性能不稳定，我建议对该项目尽量报以参考学习的目的，而非应用于生产环境
- 后续我会写一个服务器2.0，从设计上避开绑定函数形式存在的的隐性bug，欢迎感兴趣的童鞋参考我写的脏代码
- 参考了（https://gitee.com/ma_yidan/NetServer） 的实现逻辑，在此基础上：
    + 优化拆分了一些非必要的类结构以便于理解和优化程序设计
    + 重构了大量的实现方式以用于拓展服务
    + 添加了详尽的注释
    + 设计了基于高级服务的函数注册绑定的连接复用模式
    + 添加了端口映射服务、文件单例模式类型识别、非网站式高级服务（例如：文件传输服务ResourceService）
