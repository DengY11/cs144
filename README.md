Stanford CS 144 Networking Lab
==============================

These labs are open to the public under the (friendly) request that to
preserve their value as a teaching tool, solutions not be posted
publicly by anybody.

Website: https://cs144.stanford.edu

To set up the build system: `cmake -S . -B build`

To compile: `cmake --build build`

To run tests: `cmake --build build --target test`

To run speed benchmarks: `cmake --build build --target speed`

To run clang-tidy (which suggests improvements): `cmake --build build --target tidy`

To format code: `cmake --build build --target format`



| 缩写     | 全称                            | 含义                                                         |
|--------|--------------------------------|------------------------------------------------------------|
| SYN    | Synchronize                    | 用于建立连接时的同步标志，表示“这是一个连接请求”                            |
| ISN    | Initial Sequence Number        | 初始序号，在三次握手的 SYN 报文里随机选择，作为后续字节序号计算的基准                    |
| SEQ/SEQNO | Sequence Number               | 序号字段，标识该报文段第一个数据字节在整个字节流中的位置（32 位环绕计数）              |
| ACK    | Acknowledgment                 | 确认标志，置 1 时表示这是一个带有确认号的报文                                   |
| ACKNO  | Acknowledgment Number          | 确认号字段，告诉对端“我已成功接收至序号 X−1 的字节，下一个期待的字节编号是 X”         |
| FIN    | Finish                         | 结束标志，表示“发送端的数据发完了，想要关闭连接”                                |
| RST    | Reset                          | 重置标志，表示“出现错误或不想再继续本连接，立即强制中断”                          |
| PSH    | Push                           | 推送标志，提示接收端应立即将缓冲区中的数据提交给应用层                              |
| URG    | Urgent                         | 紧急标志，配合 Urgent Pointer 字段使用，指示报文中带有紧急数据                         |

---

## 1. SYN & ISN

- **SYN（Synchronize）**  
  - 在 TCP 三次握手的第一步和第二步使用。  
  - 客户端发送带 SYN=1 的报文（无数据或少量选项），告诉服务器“我想建立连接”。  
  - 服务器收到后回一个 SYN=1、ACK=1 的报文，作为握手的第二步。

- **ISN（Initial Sequence Number）**  
  - 随机生成的 32 位数，放在 SYN 报文的 Sequence Number 字段里。  
  - 它标志着本侧“字节流”计数的起点：第一个真正的数据字节，序号就是 ISN+1。  
  - 通过随机化 ISN，可以抵御部分网络攻击（如序号预测）。

### 三次握手示例
1. 客户端 → 服务器：`[SYN=1, SEQ=ISN_C]`  
2. 服务器 → 客户端：`[SYN=1, ACK=1, SEQ=ISN_S, ACKNO=ISN_C+1]`  
3. 客户端 → 服务器：`[ACK=1, SEQ=ISN_C+1, ACKNO=ISN_S+1]`  
连接此时建立，接下来就可以开始正式传输数据。

---

## 2. SEQ/SEQNO（Sequence Number）

- **作用**：为字节流中的每个字节分配一个全局唯一的“序号”，用于接收端重组乱序、丢包重传等。  
- **字段值**：32 位环绕表示，即超过 2³²−1 后回到 0。TCP 协议通过“unwrap”算法在本地还原成 64 位绝对序号。  
- **语义**：
  - 报文头里的 `SEQ` 指向本报文第一个字节的序号。  
  - 如果报文不带数据，`SEQ` 仍会被计入下一次确认的基础。

---

## 3. ACK & ACKNO（Acknowledgment & Acknowledgment Number）

- **ACK（确认标志）**  
  - 当报文中的 ACK = 1 时，表示该报文携带确认号（ACK Number）。  
  - 如果 ACK = 0，则忽略确认号字段。

- **ACKNO（确认号）**  
  - 32 位环绕数字，表示“期望收到的下一个字节序号”。  
  - 举例：如果 ACKNO = 500，说明接收端已经成功收到并处理了所有序号 499 及之前的字节，下一个期待字节是 500。  
  - ACKNO 同样会考虑 SYN/FIN 各占用一个序号号位。

---

## 4. FIN（Finish）

- **作用**：用于半关闭连接，表示该方向的数据发送完毕。  
- **语义**：  
  - FIN=1 的报文也要占用一个序号号位。  
  - 收到 FIN 后，接收端会 ACK 确认 FIN，也将该方向标记为“已关闭”，但还可继续向对端发送数据直至双方都 FIN/ACK。

---

## 5. RST（Reset）

- **作用**：立即中断连接，不再进行任何重传或四次挥手的正常关闭流程。  
- **典型场景**：  
  - 收到不期望的报文（如向未监听端口发包）。  
  - 应用层检测到错误，强制关闭连接。

---

### 小结

- **SEQ / SEQNO**：标识“我这段报文中第一个字节在整体流中的位置”。  
- **ACK / ACKNO**：标识“我已收到对端第 ACKNO−1 字节，期待从 ACKNO 开始的下一字节”。  
- **SYN / ISN**：三次握手时的“同步”操作与“初始序号”，用来建立连接。  
- **FIN**：表示发送完毕、欲关闭连接；与 SYN 类似，同样占一个序号位。  
- **RST**：表示异常重置，立刻中断连接。

