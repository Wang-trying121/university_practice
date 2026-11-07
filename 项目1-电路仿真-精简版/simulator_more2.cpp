// ============================================================================
// 文件：simulator_more2.cpp
// 作用：主程序 + 故障仿真器（FaultSimulator 类），整个工程的最终版
//
// 和旧版 simulator-more.cpp 的区别：
//   旧版把 gates / Gatemap / 结果数组 / 故障开关全写成全局变量，只能靠“跑两遍 +
//   拨全局开关”来对比；本版把它们统统收进类里，同一个对象可以反复注入不同故障，
//   互不干扰，也顺手用析构函数解决了内存释放问题。
//
// 一次完整运行流程：
//   1. FaultSimulator s;                  // 建仿真器，内存里还是空的
//   2. s.loadCircuit("circuit.txt");      // 读网表 → new 出每个门 → 按名字连线
//   3. s.runTest("", 0);                  // 无故障跑 4 组测试向量，结果存进 normalResults
//   4. 遍历所有门名，对每个门分别注入 1（卡0）/ 2（卡1）故障再跑一遍，
//      与 normalResults 逐位对比，算出“这个故障能被几个向量抓到”（命中率）
//
// 电路里“信号怎么流动”：每个门只认自己的输入脚，输入脚要么接别人的输出，
// 要么悬空用自己的默认电平；从终端门 evaluate() 开始会一路递归溯源。
// ============================================================================
#include <iostream>
#include <vector>
#include "Gate.h"
#include <fstream>
#include <map>
using namespace std;

// ============================================================================
// 1. 测试向量类：一个 TestPattern 就是一组输入激励，例如 {0,1} 表示 A=0、B=1。
//    4 组向量穷举了两个输入的所有 0/1 组合，正好是 2 输入电路的最小完备测试集。
// ============================================================================
class TestPattern {
	public:
		vector<bool> inputs; // 存放一组测试信号，例如 {0, 1}

		// 构造函数：在创建对象时，直接把信号数组装进 inputs 里
		TestPattern(const vector<bool> &i) {
			inputs = i;
		}
};

// ============================================================================
// 2. 核心仿真类：把旧版散落在全局的“数据 + 逻辑”收编成成员
// ============================================================================
class FaultSimulator {
	private:
		// ---- 两个容器：一个管内存，一个管查找 ----
		vector<Gate*> gates;//大货架：存放所有 new 出来的门，析构时统一 delete
		map<string, Gate*> Gatemap;//花名册：门名 → 门对象，按名字查门（网表连线就靠它）
		vector<string> allGateNames; // 用来存所有门的名字（按创建顺序，供 main 遍历）
		// ---- 两组结果：同一组测试向量下的“正常输出”与“注入故障后的输出” ----
		vector<bool> normalResults;
		vector<bool> badResults;
		// ---- 测试向量表：穷举 4 种输入组合（顺序为 00/01/10/11）----
		vector<TestPattern> suit = {
			TestPattern({0, 0}),
			TestPattern({0, 1}),
			TestPattern({1, 0}),
			TestPattern({1, 1})
		};
	public:
		// 析构函数：自动清理内存
		~FaultSimulator() {
			for(size_t	 i = 0; i < gates.size(); i++) {
				delete gates[i];
			}
			// 对“大货架”上的每个门调用 delete，避免内存泄漏；
			// Gate 的析构函数是虚函数，所以用基类指针 delete 也能正确析构派生类对象。
		}

		// 读文件并建电路：逐行解析网表，造门或连线。
		// 网表格式（一行一条指令，忽略空行）：
		//     <门类型> <门名>                 造门，类型可为 AND / OR / NOT / XOR
		//     CONNECT <源门名> <目标门名>      把源门的输出接到目标门的 A 脚
		// 举例：OR chipX → 造一个叫 chipX 的或门
		void loadCircuit(string filename) {
			ifstream infile(filename);
			if (!infile) {
				cout << "提取失败" << endl;
				return;
			}

			// --- 阶段 B：准备仓库（大货架 和 花名册） ---
			string type, name, name2;

			cout << "文件打开成功！开始读取..." << endl <<endl;

			while(infile >> type >> name) {
				cout << "读到指令: " << type << " 名字: " << name << endl;

				// 分支 1：如果是造芯片的指令（非 CONNECT 指令）
				if (type != "CONNECT") {
					Gate* newgate = nullptr; // 准备一个万能插座（基类指针）

					// 根据从文件读到的类型，在内存中动态创建对应的门对象。
					// 注意这里只判断了 4 种门，AorGate（同或门）还没接进网表解析。
					if(type == "AND") {
						newgate = new AndGate();
					} else if(type == "NOT") {
						newgate = new NotGate();
					} else if(type == "OR") {
						newgate = new OrGate();
					} else if(type == "XOR") {
						newgate = new XorGate();
					}

					// 如果芯片造出来了，就贴标签并入库
					if (newgate != nullptr) {
						newgate->name = name;        // 给芯片贴上名字（如"g1"）
						gates.push_back(newgate);    // 放进大货架，便于最后统一销毁
						Gatemap[name] = newgate;     // 登记进花名册，便于按名字查找
						allGateNames.push_back(newgate->name);  // 另外记一份顺序表，供 main 遍历
						cout << "成功创建门: " << newgate->name << endl<<endl;
					}
					// 若类型拼错（比如写成 BND），newgate 保持 nullptr，这个门被静默跳过。

					// 分支 2：如果是连线的指令（CONNECT）
				} else if(type == "CONNECT") {
					infile >> name2; // CONNECT 后面有两个名字，这里把第二个名字（目标门）读进 name2

					cout << "尝试连线: " << name << " -> " << name2 << endl;

					// 安全检查：防止文件里写了错别字导致查不到门，引起程序崩溃
					if (Gatemap[name] == nullptr) {
						cout << "【错误】找不到源门: " << name << endl;
					} else if (Gatemap[name2] == nullptr) {
						cout << "【错误】找不到目标门: " << name2 << endl;
					} else {
						// 核心连线魔法！
						// 从花名册里找到目标门，把源门的地址传给目标门的输入 A 引脚
						// 相当于把一根物理导线从 name 的输出端，焊到了 name2 的输入端
						Gatemap[name2]->connectSourceA(Gatemap[name]);
						cout << "连线成功！" << endl<<endl;
					}
				}
			}
			infile.close(); // while 循环结束，电路已完全搭建在内存中
			// 收尾说明：连线的目标脚固定是 A 脚，所以这份网表只支持“一个上游 + B 脚留空”
			// 的连接方式；想串两个上游，得再补一条能接 B 脚的指令。
		}

		// 找“源头门”：判断标准就是 ifsourceGate()，即 A 脚悬空的门。
		// 这些门没有上游，测试激励必须直接打在它们身上。
		vector<Gate*> findsourceGate() {
			vector<Gate*> sourceGates;
			for (auto& pair : Gatemap) {
				if (pair.second->ifsourceGate()) {
					sourceGates.push_back(pair.second);
				}
			}
			return sourceGates;
		}
		// 找“终端门”：反过来想——把所有“当过别人上游”的门都从花名册副本里删掉，
		// 剩下没人拿来当上游的，就是电路的最后一级输出（终端）。
		// 说明：这里是按名字 erase，所以前提是门名唯一；若某个门既当别人的上游、
		// 自己的 A 脚又悬空，它会同时被算作源头和上游，这份判断方式只做到“够用”。
		vector<Gate*> findterminalGate() {
			map<string,Gate*> Copygate = Gatemap;
			for (auto& pair :Gatemap) {
				Gate* srcA = pair.second-> getSourceA();
				if (srcA != nullptr) {
					Copygate.erase(srcA->name);
				}
				Gate* srcB = pair.second-> getSourceB();
				if (srcB != nullptr) {
					Copygate.erase(srcB->name);
				}
			}
			vector<Gate*> terminalGates;
			for (auto& pair :Copygate) {
				terminalGates.push_back(pair.second);
			}
			return terminalGates;
		}

		// 跑测试：同一个函数兼顾“正常模式”和“故障模式”。
		//   faultType == 0 → 正常模式：4 组向量下的终端输出追加进 normalResults；
		//   faultType != 0 → 故障模式：先给 targetGate 注入该型故障，跑 4 组向量收集
		//                    badResults，与 normalResults 逐位比较，打印命中率，
		//                    最后把故障恢复成 0（不污染下一轮）。
		// 前提假设：整个电路只有一个源头门、一个终端门，所以下面直接用
		// sourceGates[0] / terminalGates[0]。
		void runTest(string targetGate, int faultType) {
			vector<Gate*> sourceGates = findsourceGate();
			vector<Gate*> terminalGates = findterminalGate();
			if (sourceGates.empty()) {
				cout<<"未找到源头门"<<endl;
			} else if(terminalGates.empty()) {
				cout<<"未找到终端门"<<endl;
			} else {
				// ---- 正常模式 ----
				if (faultType == 0) {
					for (int t = 0; t < 4; t++) {
						cout << "测试向量: " << suit[t].inputs[0] << " " << suit[t].inputs[1] << endl;
						// 1. 把测试向量施加给源头 g1
						sourceGates[0]->setInput(suit[t].inputs[0], suit[t].inputs[1]);
						// 2. 让终端 g2 去计算。g2 会通过指针自动向源头 g1 索要数据（递归求值）
						terminalGates[0]->evaluate();
						// 3. 打印终端 g2 算出的最终输出
						cout << "最终输出: " << terminalGates[0]->getOutput() << endl<<endl;
						normalResults.push_back(terminalGates[0]->getOutput());
					}
				}
				// ---- 故障模式：注入 → 重跑 → 比对 → 恢复 ----
				if (faultType != 0) {
					cout<<"现在进行错误测试"<<endl;
					badResults.clear();
					Gatemap[targetGate]->faultType = faultType;
					for (int t = 0; t < 4; t++) {
						//cout << "测试向量: " << suit[t].inputs[0] << " " << suit[t].inputs[1] << endl;
						// 1. 把测试向量施加给源头 g1
						sourceGates[0]->setInput(suit[t].inputs[0], suit[t].inputs[1]);
						// 2. 让终端 g2 去计算。g2 会通过指针自动向源头 g1 索要数据（递归求值）
						terminalGates[0]->evaluate();
						// 3. 打印终端 g2 算出的最终输出
						//cout << "g2最终输出: " << Gatemap["g2"]->getOutput() << endl<<endl;
						badResults.push_back(terminalGates[0]->getOutput());
					}
					float n = 0; // 记录命中的测试向量数量。用 float 是为了后续除法能保留小数
					for(size_t i = 0; i < badResults.size(); i++) {
						// 对比同一个测试向量下，正常输出和故障输出是否不一致
						// 如果不一致，说明这个向量成功检测到了故障
						if(normalResults[i] != badResults[i]) {
							n++;
						}
					}
					cout << "共发现" << n << "个错误" << endl;
					cout << "测试向量命中率" << n / badResults.size() * 100 << "%" << endl;
					n = 0;
					Gatemap[targetGate]->faultType = 0;
				}
			}
		}

		// 设计说明（只记录，不改代码）：
		// 1) normalResults 在这里只追加、不清空：正常调用一次 runTest("", 0) 没问题，
		//    若重复调用，后面比对的第 i 个元素就对不上了。
		// 2) Gatemap[targetGate] 用的是 map::operator[]，若传入不存在的门名，会先插入
		//    一个 nullptr 条目然后解引用崩溃，所以 targetGate 必须来自 getAllGateNames()。
		// 3) 激励只打给 sourceGates[0] 的 A、B 两个脚；下游门如果 B 脚没接线，会一直用
		//    构造函数里的 false，不会跟着第二个测试输入变化。
		// 4) 只模拟“卡 0 / 卡 1”两类固定型故障，不含桥接、延迟、翻转率等故障模型。


// 提供一个接口，让外部能拿到 allGateNames
		// 对外接口：按创建顺序返回所有门名，供 main 遍历注入故障。
		vector<string> getAllGateNames() {
			return allGateNames;
		}
};

// ============================================================================
// 3. 主函数：把“读网表 → 正常测试 → 逐门注入故障测试”串起来
// ============================================================================
// 3. 主函数
int main() {
	// 1. 实例化 FaultSimulator：此时内部只有空容器，还没有任何门。
	FaultSimulator simulator1;
	// 2. 调用 loadCircuit：解析 circuit.txt，new 出门并按名字把它们连起来。
	simulator1.loadCircuit("circuit.txt");
	// 3. 跑正常测试 runTest("", 0)：空名字 + 0 号故障 = 只跑正常模式，
	//    4 组向量的输出存进 normalResults，作为后面比对的基准。
	simulator1.runTest("",0);
	vector<string> names = simulator1.getAllGateNames();
	for (size_t i = 0; i < names.size(); i++) {
		for (int m = 1; m < 3; m++) {
			simulator1.runTest(names[i],m);
		}
	}
	// 4. 用 for 循环遍历 getAllGateNames()，分别注入 1 和 2 故障。
	//    内层 m = 1、2 分别对应“卡 0”和“卡 1”，每轮打印该门的故障命中率。
	//    names[i] 里存的是真实门名，所以一定能被 runTest 里的 Gatemap 查找到。

	return 0;
}
