// ============================================================================
// 文件：main.cpp
// 作用：程序入口。把“读网表 → 正常测试 → 逐门注入故障测试 → 对比结果”串成一条线。
//
// 输出长这样：
//   ---------正在进行正确测试-------------   16 组向量（4 个悬空引脚 → 2^4）
//   测试注入0000 / g3的输出为0 / ...
//   ---------正在进行错误测试-------------   每个门 × 两种故障各一遍
//   共发现4个错误 / 向量命中率25%
// ============================================================================
#include <iostream>
#include "simulator.h"

using namespace std;
int main() {
	// 1. 建仿真器：构造时只把 test_pins 置 0，gates / Gatemap 等容器都是空的
	FaultSimulator simulator1;
	// 2. 读网表 circuit.txt：造门 → 登记名字 → 按 CONNECT 把上游接到目标门的 inA / inB
	simulator1.loadCircuit("circuit.txt");
	// 3. 正常模式：把所有悬空引脚穷举 0/1（共 2^n 组），记录每组下所有终端门的输出，
	//    存进 normalResults —— 它是后面故障对比的基准答案，所以必须先跑正常测试。
	//    每组向量开始前都会 Reset() 全部门，避免复用上一轮的求值缓存。
	//    （circuit.txt 有 4 个悬空引脚，所以这里会跑 16 组向量）

	simulator1.RunNormalTest();
	// 4. 遍历每个门，分别注入 1（卡 0）和 2（卡 1）两种故障，每次跑完立刻对比
	//    先拿到门名列表（按创建顺序：g1, g2, g3 ...）
	vector<string> names = simulator1.getAllGateNames();
	// 外层：把每个门轮流当作“被注入了故障的器件”
	for (size_t i = 0; i < names.size(); i++) {
		// 内层：n = 1 是卡 0 故障，n = 2 是卡 1 故障
		for(int n = 1; n < 3; n++) {
			// 用这个门 + 这种故障，重跑一遍全部测试向量
			simulator1.RunFaultTest(names[i],n);
			// 和正常结果逐组对比，打印“共发现 X 个错误 / 向量命中率 Y%”
			simulator1.ContrastResult();
		}
	}

	// main 结尾没有写 return 0；C++ 标准规定 main 执行到末尾等价于 return 0，所以不影响结果
}
