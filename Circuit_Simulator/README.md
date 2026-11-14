# C++ 电路仿真系统
[![Language](https://img.shields.io/badge/Language-C%2B%2B14-blue.svg)](https://isocpp.org/)
[![Build](https://img.shields.io/badge/Build-g%2B%2B%20Passing-green.svg)]()
[![Warnings](https://img.shields.io/badge/Warnings-Zero-orange.svg)]()
[![AI](https://img.shields.io/badge/AI_for_EDA-Exploring-purple.svg)]()

这是一个基于**C++**的逻辑电路仿真程序，实现了与门、或门、否门、异或门的连接与故障测试。

## 🔖 特性介绍
- **加载** 能够自主识别电路网表（circuit.txt)并加载电路
- **面向对象**使用C++面向对象实现不同类型门的多态逻辑运算
- **算法分析**设计基于 `std::map` 的图剪枝算法（时间复杂度 O(N log N)），自动识别源头门和终端门
- **深度递归优先求值** 计算信号沿电路逆向传播，同时引入状态机（‘evalState')，进行环路检测，防止栈溢出
- **现代内存管理** 全面使用 **std::unique_ptr** 
- **故障注入** 通过遍历对象，注入“只输出 1”或“只输出 0” 的故障

## ✨ 系统架构
```text
[输入层] circuit.txt (电路网表文本)
       │
       ▼
[解析层] 动态创建逻辑门对象 (new AndGate, new NotGate...) 
       │ 并存入 vector<unique_ptr<Gate>> 和 map<string, Gate*> 中
       ▼
[拓扑层] 连线与拓扑识别 (map 剪枝寻找 Source Gate 和 Terminal Gate)
       │
       ▼
[仿真层] 施加测试激励 (DFS 递归信号求值)
       │
       ▼
[诊断层] 注入 SA0/SA1 故障，比对终端输出，计算故障覆盖率 (输出至控制台)
```
## 📂 文件结构
``` text
电路仿真demo/
|Gate.h              # 逻辑门基类与派生类的声明 (多态接口)
│simulator.h        # 关于各种计算与运行的声明
│simulator-prepare.cpp   # 关于每一个门的具体性质
|simulator-prepare2.cpp       # 准备，测试，与故障注入，结果对比分析
|main.cpp            #主程序入口
|circuit.txt         # 模拟的电路网表文件

```
## 🛠️ 快速开始
**环境要求**：支持 C++14 的编译器（GCC / Clang / MSVC）

**编译命令（严格模式）：**
```
bash

g++ -std=c++14 main.cpp simulator-prepare.cpp simulator-prepare2.cpp -o simulator -Wall -Wextra -Werror
```
**运行测试：**
```
bash

./simulator
```
_(程序将自动读取 `circuit.txt` 中的电路结构，遍历所有测试向量，并输出故障覆盖率报告)_

## 🗺️  未来路线图 (Roadmap)

- 引入启发式算法替代 2<sup>n</sup>次穷举
- 提取网表图特征，自动识别易故障节点
- 搭载前端UI，实现可视化操作
