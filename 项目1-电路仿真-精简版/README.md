# 项目1 · 逻辑电路仿真器（精简版）

> C++11 实现的数字逻辑电路仿真内核：把与/或/非/异或门抽象成对象，用指针当导线连级，
> 递归求值算出输出，再注入「卡 0 / 卡 1」故障来统计测试向量命中率。

本文件夹是从原工程 `D:\university_practice\simulate-项目` 中整理出来的**最小可运行版本**，
只保留真正参与编译和运行的 3 个源文件，并补全了全部注释。

---

## 1. 目录内容

| 文件 | 行数 | 角色 | 说明 |
|---|---:|---|---|
| `Gate.h` | 74 | 头文件（声明） | 抽象基类 `Gate` + 5 个具体门：`AndGate` / `OrGate` / `NotGate` / `XorGate` / `AorGate`(同或) |
| `simulator-prepare.cpp` | 133 | 源文件（实现） | `Gate` 及所有门的成员函数实现：连线、取输入、递归求值、故障注入 |
| `simulator_more2.cpp` | 275 | 源文件（主程序） | `TestPattern` 测试向量、`FaultSimulator` 仿真器类、`main()` |
| `circuit.txt` | 5 | 数据文件 | 网表（电路描述），程序运行时必须能找到它 |
| `README.md` | — | 文档 | 本文件 |

> 3 个源文件缺一不可：`Gate.h` 只有声明不能单独编译成可执行文件；`simulator_more2.cpp`
> 里才是唯一的 `main()`。原始工程里的 `simulator-main.cpp`、`simulator-more.cpp` 是过程式
> 的过渡版本（`main()` 已被注释掉），`Makefile.win` / `.dev` / `.o` / `.exe` 是构建产物，
> 都没有带过来。

**编码说明**：`.h` / `.cpp` 沿用原工程的 **GBK（ANSI）** 编码，这样 Dev-C++ 5.11 打开时
中文注释不会乱码；`README.md` 是 **UTF-8**，方便 Typora / VSCode / GitHub 阅读。
用 VSCode 看源码时请在右下角把编码切成 `GB2312/GBK`。

---

## 2. 快速开始

### 方式 A：命令行编译（最快）

需要 g++（Windows 上装 MinGW 或直接用 Dev-C++ 自带的那个）。在本文件夹内打开终端：

```bat
g++ -std=c++11 -Wall -Wextra -o circuit_sim.exe simulator-prepare.cpp simulator_more2.cpp
circuit_sim.exe
```

程序用**相对路径**打开 `circuit.txt`，所以必须在含有 `circuit.txt` 的目录里运行。
若用 Dev-C++ 自带的编译器，完整路径是：

```bat
"D:\dev.C++\Dev-Cpp\MinGW64\bin\g++.exe" -std=c++11 -Wall -Wextra -o circuit_sim.exe simulator-prepare.cpp simulator_more2.cpp
```

### 方式 B：Dev-C++ 5.11

1. `文件 → 新建 → 项目 → Console Application（C++）`，保存到本文件夹，例如 `项目1精简版.dev`；
2. 把 `simulator-prepare.cpp`、`simulator_more2.cpp` 加入工程（右键 → 添加），`Gate.h` 也一并加入（头文件不参与编译，只是方便查看）；
3. **工程选项 → 参数** 里加上 `-std=c++11`；
4. 编译运行（F11）。工作目录要设成本文件夹，否则会提示「提取失败」（找不到 `circuit.txt`）。

> 注意：这三个文件里只有 `simulator_more2.cpp` 有 `main()`，所以两个 `.cpp` 必须一起编译，
> 不能只编 `simulator-prepare.cpp`，否则链接会报 `undefined reference to WinMain`。

---

## 3. 电路怎么描述：`circuit.txt` 网表格式

一行一条指令，程序用 `while(infile >> type >> name)` 逐词读，不看行号：

| 指令 | 格式 | 含义 |
|---|---|---|
| 造门 | `<类型> <门名>` | 类型可为 `AND` / `OR` / `NOT` / `XOR`；门名自己起，全文件唯一 |
| 连线 | `CONNECT <源门名> <目标门名>` | 把源门的输出接到目标门的 **A 脚**（目前只能接 A 脚） |

自带的例子：

```
OR chipX
XOR chipY
NOT chipZ
CONNECT chipX chipY
CONNECT chipY chipZ
```

意思是：`chipX`(或门) → `chipY`(异或门) → `chipZ`(非门)，形成一条三级链。
源头门是 `chipX`（没有上游），终端门是 `chipZ`（没有下游）。

逻辑关系：`chipZ = NOT(chipY)`，`chipY = XOR(chipX, B)`。而 `chipY` 的 B 脚没接线，
一直是默认的 `false`，**异或一个 0 等于原样传递**，所以实际效果是：

```
chipZ = NOT(chipX) = NOT(A OR B)
```

---

## 4. 代码结构与关键机制

### 4.1 类设计（`Gate.h`）

```
Gate（抽象基类，有纯虚 evaluate()，不能直接 new）
├── AndGate   与门   output = A && B
├── OrGate    或门   output = A || B
├── NotGate   非门   output = !A
├── XorGate   异或门 output = (A != B)
└── AorGate   同或门 output = (A == B)   ← 名字叫 Aor，实际是 XNOR
```

`Gate` 的 5 个数据成员，全部是 protected：

| 成员 | 含义 |
|---|---|
| `inputa` / `inputb` | 引脚**自带电平**，只有该引脚**没接线（悬空）**时才会用到 |
| `output` | 上一次 `evaluate()` 算出的结果 |
| `inputsourceA` / `inputsourceB` | `Gate*` 指针，指向「接在这个引脚上的上游门」，`nullptr` = 悬空 |

**这就是整个仿真器的核心抽象**：一根导线 = 一个指针；一个门 = 一个对象；
`connectSourceA(p)` 就是把 `p` 的输出「焊」到自己的 A 脚上。

### 4.2 信号传播：深度优先递归求值

`simulator-prepare.cpp` 里最关键的两行（`getRealA()` 的 else 分支）：

```cpp
inputsourceA->evaluate();            // 先让上游门把自己算一遍
return inputsourceA->getOutput();    // 再把上游门的结果取回来
```

一个门要 A 脚的值 → 让上游门先算 → 上游门又要它的上游先算 …… 一直递归到源头门
（引脚全悬空）为止，值再一层层往回传。**好处**：不需要拓扑排序，也不用遍历整张网表，
从终端门 `evaluate()` 一次就能拿到正确结果。**代价**：同一段电路会被重复计算（没有缓存），
门数多时会明显变慢。

### 4.3 故障注入：只改一行 `getOutput()`

`Gate::getOutput()` 里判断 `faultType`：

| `faultType` | 行为 | 模拟的物理故障 |
|---:|---|---|
| 0 | 正常返回 `output` | 无 |
| 1 | 恒返回 `0` | 输出**卡在 0**（stuck-at-0） |
| 2 | 恒返回 `1` | 输出**卡在 1**（stuck-at-1） |
| 其他 | 按正常处理 | 非法值兜底 |

因为下游门**只通过 `getOutput()` 取上游的值**，所以故障会顺着连线自然传播到终端，
不需要修改任何门的内部逻辑，这就是「故障仿真」最朴素的实现方式。

### 4.4 自动识别源头门与终端门（`FaultSimulator`）

- **源头门**：`A 脚悬空` 的门（`ifsourceGate()`）→ 测试激励打给它们；
- **终端门**：先把所有「当过别人上游」的门从名册副本里删掉，剩下的就是终端。
  用「删掉做过上游的」这个反向思路，等价于找「出度为 0」的节点。

### 4.5 一次运行的完整流程（`main()`）

```
FaultSimulator s;                    // 建仿真器（内部空）
s.loadCircuit("circuit.txt");        // 读网表 → new 门 → 按名字连线
s.runTest("", 0);                    // 正常模式：4 组向量 → normalResults
for 每个门名 names[i]:
    for m = 1, 2:                    // 卡 0 / 卡 1
        s.runTest(names[i], m);      // 注入故障 → 重跑 → 比对 → 打印命中率 → 恢复
```

---

## 5. 运行结果与解读

自带网表的实际输出（已实测，退出码 0）：

```
文件打开成功！开始读取...

读到指令: OR 名字: chipX
成功创建门: chipX
读到指令: XOR 名字: chipY
成功创建门: chipY
读到指令: NOT 名字: chipZ
成功创建门: chipZ
读到指令: CONNECT 名字: chipX
尝试连线: chipX -> chipY
连线成功！
读到指令: CONNECT 名字: chipY
尝试连线: chipY -> chipZ
连线成功！

测试向量: 0 0
最终输出: 1
...
```

**正常模式真值表**（`chipZ = NOT(A OR B)`）：

| A | B | 中间 X = A OR B | 中间 Y = XOR(X, 0) | 最终输出 Z = NOT(Y) |
|---:|---:|---:|---:|---:|
| 0 | 0 | 0 | 0 | **1** |
| 0 | 1 | 1 | 1 | **0** |
| 1 | 0 | 1 | 1 | **0** |
| 1 | 1 | 1 | 1 | **0** |

**故障覆盖率**（3 个门 × 2 种故障 = 6 轮，每轮 4 个向量）：

| 被注入的门 | 卡 0（faultType=1） | 卡 1（faultType=2） |
|---|---:|---:|
| `chipX` | 75%（3/4 被抓） | 25%（1/4 被抓） |
| `chipY` | 75% | 25% |
| `chipZ` | 25% | 75% |

命中率的算法：把故障模式下的 4 个输出和正常模式的 4 个输出逐位比较，
不一致的个数 ÷ 4 就是命中率（`runTest()` 里的 `n / badResults.size() * 100`）。
卡 0 型让输出恒为 1，与正常值 `1,0,0,0` 有 3 位不同 → 75%；卡 1 型让输出恒为 0，
只与第 1 位不同 → 25%。**同一个故障能被越多的测试向量抓到，这个测试集的质量就越高**，
这就是可测性设计（DFT）里「故障覆盖率」概念的雏形。

---

## 6. 已知限制（只记录，未改代码）

按「只加注释、不改代码」的要求，以下问题**保留原样**，仅在此说明，方便写报告时引用：

1. **只支持单源头 / 单终端**：`runTest()` 里直接写死 `sourceGates[0]`、`terminalGates[0]`，
   电路有两个独立源头门时，第二个门收不到激励。
2. **激励只打在源头门的两个脚上**：下游门如果 B 脚没接线，会一直用构造函数里的 `false`，
   不随测试向量的第二位变化。所以自带网表里 `chipY` 的 B 脚其实始终是 0。
3. **连线只能接 A 脚**：`loadCircuit()` 固定调用 `connectSourceA()`，网表语法没有指定脚位的写法，
   `connectSourceB()` 目前没有用武之地。
4. **`ifsourceGate()` 只检查 A 脚**：A 脚接线、B 脚悬空的中间门会被误判成源头门。
5. **`normalResults` 只追加不清空**：正常模式只应调用一次，重复调用会让命中率算错。
6. **`Gatemap[targetGate]` 用 `operator[]`**：传入不存在的门名会插入 `nullptr` 条目然后崩溃；
   传空字符串也是同理，只是正常模式下走不到这个分支。
7. **求值无缓存**：同一个门在一次 `evaluate()` 里可能被重复计算，门数多时性能会掉。
8. **没有环检测**：连线成环会栈溢出（递归求值无法终止）。
9. **网表里的 `AorGate` 无法创建**：`loadCircuit()` 只解析 `AND/OR/NOT/XOR` 四种类型。

---

## 7. 与原工程的关系

| 内容 | 处理方式 | 原因 |
|---|---|---|
| `Gate.h` | **保留**（补注释） | 完整类声明，是全部逻辑的基础 |
| `simulator-prepare.cpp` | **保留**（补注释） | 所有成员函数实现，必须参与编译 |
| `simulator_more2.cpp` | **保留**（补注释） | 面向对象最终版，唯一有效的 `main()` |
| `simulator-main.cpp` | 未带过来 | 过程式过渡版，`main()` 已被注释，另有 `Circuit` 类未使用 |
| `simulator-more.cpp` | 未带过来 | 全局变量版，被 `simulator_more2.cpp` 完全取代 |
| `circuit.txt` | 带过来了 | 运行必需的数据文件 |
| `.dev` / `Makefile.win` | 未带过来 | 构建配置，可在新文件夹里重新生成 |
| `.o` / `.exe` | 未带过来 | 构建产物，不该进版本库 |

### 本次「只改注释」的验证方式

改动严格限制在注释里，代码本身零改动，用两道验证卡住：

1. **token 级比对**：把原文件（GBK）和新文件都去掉注释、空白后切成 token 序列，
   三者 token 数完全一致（`Gate.h` 212 / `simulator-prepare.cpp` 368 / `simulator_more2.cpp` 1191）；
2. **行为级比对**：用和原工程相同的编译选项（`-std=c++11 -Wall -Wextra -Werror`）编译新文件，
   零警告零错误通过；运行输出与原 `.exe` 的输出**逐字节相同**（SHA-256 一致）。

行数变化：`Gate.h` 59 → 74，`simulator-prepare.cpp` 68 → 133，`simulator_more2.cpp` 219 → 275，
多出来的全是注释和空行。

---

## 8. 可以接着练的方向

- 给 `Gate` 增加通用的「多输入脚」容器（`vector<Gate*> inputs`），摆脱 A/B 两个脚的限制；
- 用 `setInput` 之外的方式做激励：把外部输入抽象成一种 `InputGate`（永远被当源头门处理），
  这样「电路有几个输入脚」就能自动推导出来；
- 求值时加一层缓存标记（`evaluated` 标志 + 拓扑顺序），避免重复递归；
- 把网表解析改成「先造门、后连线」两遍扫描，支持接 B 脚、支持 `AorGate`；
- 输出改成打印「故障覆盖率报告表」或导出 CSV。
