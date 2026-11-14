// ============================================================================
// 文件：simulator.h
// 作用：故障仿真器 FaultSimulator 的类声明（电路怎么组织、怎么批量测试），
//       具体实现在 simulator-prepare2.cpp 里。
//
// 它要干的事：把 circuit.txt 里的门和线装进内存 → 找出所有“悬空引脚”并穷举 0/1 激励
//             → 算出终端门输出 → 再对每个门分别注入“卡 0 / 卡 1”故障重跑一遍
//             → 对比两次结果，算这组测试向量能抓到多少故障（故障覆盖率）。
//
// 和 Gate.h 的分工：
//   Gate.h      —— 一个门怎么算（真值表达式、引脚接线、故障表现）
//   simulator.h —— 一堆门怎么组成电路、怎么批量测试、怎么对比结果
// ============================================================================
#ifndef SIMULATOR_H
#define SIMULATOR_H
#include <vector>
#include <map>
#include <memory>
#include "Gate.h"
#include <string>
using namespace std;
// ----------------------------------------------------------------------------
// 故障仿真器：持有整个电路，负责“加载 → 识别引脚 → 跑测试 → 对比结果”这条流水线
// ----------------------------------------------------------------------------
class FaultSimulator {
	private:
		// ---- 两个容器：一个管内存，一个管查找 ----
		vector<unique_ptr<Gate>> gates;
		// 门名 → 门对象（裸指针，只“借用”地址，不拥有所有权），连线时按名字查门
		map<string, Gate*> Gatemap;
		// 门名按创建顺序另存一份，供 main 遍历注入故障（map 是按名字排序的，不是创建顺序）
		vector<string> allGateNames;

		// ---- 测试相关的缓存 ----
		// 终端门（没有下游的门）：求值时只看它们的输出，识别一次即可复用
		vector<Gate*> terminalGates;
		// 预留的引脚计数变量（当前实现里实际用的是 findEmptypin() 返回的列表长度）
		int test_pins;
		// 正常模式结果：外层下标 = 第几组测试向量，内层 = 该组下所有终端门的输出
		vector<vector<bool>> normalResults;
		// 故障模式结果：结构与 normalResults 完全一致，下标一一对应才能逐组比较
		vector<vector<bool>> badResults;
	public:
		// 一个“悬空引脚”的坐标：哪个门（gate）的第几个脚（index）
		// index = 0 表示 A 脚，index = 1 表示 B 脚；测试激励就是按这个坐标逐个打进去的
		struct Pin {
			Gate* gate;
			int index;
		};
		// 构造：只把 test_pins 置 0，其余容器走各自的默认构造（都是空的）
		FaultSimulator();
		// 读网表建电路。指令格式：<门类型> <门名> / CONNECT <源门名> <目标门名> <脚名>
		// 造门时会同时写进 gates（拥有所有权）和 Gatemap（按名字查找），并记进 allGateNames。
		void loadCircuit(string filename);
		// 扫描所有门，收集“没有接线”的引脚（A 脚 / B 脚），这些就是电路的“外部输入”
		// 返回个数 n 决定测试向量有 2^n 组（穷举所有 0/1 组合）
		vector<Pin> findEmptypin();
		// 找出终端门：把所有“当过别人上游”的门从花名册副本里删掉，剩下的就是终端
		// 用 map 的删除实现剪枝，复杂度 O(N log N)
		vector<Gate*> findterminalGate();
		// 正常模式：按 2^n 组向量逐组施加激励、记录终端输出，存进 normalResults
		// 这是后面所有故障对比的“基准答案”
		void RunNormalTest();
		// 故障模式：给 targetGate 注入 faultType（1=卡 0，2=卡 1）后重跑同一套向量
		// 结果存进 badResults；跑完会把 faultType 复位成 0
		// 前提：targetGate 必须是 getAllGateNames() 里的名字（内部用 map 下标访问）
		void RunFaultTest(string targetGate,int faultType);
		// 对外接口：按创建顺序返回所有门名，供 main 遍历注入故障
		vector<string> getAllGateNames();
		// 对比 normalResults 与 badResults：同一向量下只要有任意终端输出不同，就算“抓到”
		// 最后打印“共发现 X 个错误”和“向量命中率 Y%”
		void ContrastResult();
};
#endif

