// ============================================================================
// 文件：simulator-prepare2.cpp
// 作用：FaultSimulator 的所有成员函数实现（对应 simulator.h 的声明）
//
// 一次完整流程（main.cpp 里也是这个顺序）：
//   loadCircuit()    读网表：造门、登记名字、按 CONNECT 把上游接到目标门的 inA / inB
//   findEmptypin()   找出所有“悬空引脚”= 电路的外部输入，个数 n 决定 2^n 组测试向量
//   RunNormalTest()  穷举 2^n 组激励，记录每组下所有终端门的输出 → normalResults（基准）
//   RunFaultTest()   对某个门注入卡 0 / 卡 1 故障，用同一套激励重跑 → badResults
//   ContrastResult() 逐组对比两轮结果，算“这组向量抓到了几个故障”
// ============================================================================
#include "simulator.h"
#include<iostream>
#include <fstream>
#include <memory>
using namespace std;
// 构造：唯一需要初始化的是 test_pins，其余容器默认构造即为空
FaultSimulator::FaultSimulator():
	test_pins(0) {}
// 读网表并建电路。每行一条指令，空白分隔：
//     <门类型> <门名>                        造门：AND / OR / NOT / XOR
//     CONNECT <源门名> <目标门名> <脚名>       连线：脚名 inA / inB
// 例：CONNECT g1 g3 inA  →  把 g1 的输出接到 g3 的 A 脚
// 注意：门必须先被创建、后出现在 CONNECT 里，否则会报“找不到门”
void FaultSimulator::loadCircuit(string filename) {
	// --- 阶段 A：打开网表文件 ---
	ifstream infile(filename);
	if (!infile) {
		cout << "提取失败" << endl;
		return;
	}
	// --- 阶段 B：准备仓库（大货架 和 花名册） ---
	// 一行里至少有“指令类型 + 门名”两个词；CONNECT 还要再读两个词（目标门名、脚名）
	string type, name, name2,pin;
	cout << "文件打开成功！开始读取..." << endl <<endl;
	// 逐行解析：每次先读两个词，再按 type 分派到“造门”或“连线”
	while(infile >> type >> name) {
		cout << "读到指令: " << type << " 名字: " << name << endl;
		// 分支 1：如果是造芯片的指令（非 CONNECT 指令）
		if (type != "CONNECT") {
			// unique_ptr 接管 new 出来的门：就算中途 return 或抛异常，也会自动释放
			unique_ptr<Gate> newgate;// 准备一个万能插座（基类指针）
			// 根据从文件读到的类型，在内存中动态创建对应的门对象。
			// 注意这里只判断了 4 种门，AorGate（同或门）还没接进网表解析。
			if(type == "AND") {
				newgate = make_unique<AndGate>();
			} else if(type == "NOT") {
				newgate = make_unique<NotGate>();
			} else if(type == "OR") {
				newgate = make_unique<OrGate>();
			} else if(type == "XOR") {
				newgate = make_unique<XorGate>();
			}
			// 如果芯片造出来了，就贴标签并入库
			if (newgate != nullptr) {
				newgate->name = name;        // 给芯片贴上名字（如"g1"）
				// 顺序很关键：先把裸指针登记进 map，再把所有权 move 进 gates；
				// 对象本体由 gates 的 unique_ptr 持有，map 里的指针只借用地址，不会二次释放
				Gatemap[name] = newgate.get();     // 登记进花名册，便于按名字查找
				gates.push_back(std::move(newgate));    // 放进大货架，便于最后统一销毁
				allGateNames.push_back(name);  // 另外记一份顺序表，供 main 遍历
				cout << "成功创建门: " << name << endl<<endl;
			}
			// 若类型拼错（比如写成 BND），newgate 保持 nullptr，这个门被静默跳过。
		// 分支 2：连线指令 CONNECT <源门名> <目标门名> <脚名>
		} else if(type == "CONNECT") {
			infile >> name2;// CONNECT 后面有两个名字，这里把第二个名字（目标门）读进 name2
			infile >> pin;
			// 用 find 而不是 Gatemap[name]：operator[] 在查不到时会往 map 里插入空指针，污染数据
			auto itsource = Gatemap.find(name);
			auto ittarget = Gatemap.find(name2);
			cout << "尝试连线: " << name << " -> " << name2 <<"通过"<<pin<< endl;
			// 源门或目标门任一查不到就报错，不再往下连线
			if (itsource == Gatemap.end() || ittarget == Gatemap.end()) {
				cout<<"出错了！找不到门"<<endl;
			} else {
				// 核心连线魔法！
				// 从花名册里找到目标门，把源门的地址传给它对应的输入脚（下面按 pin 选 A 还是 B）
				// 相当于把一根物理导线从 name 的输出端，焊到了 name2 的输入端
				// 脚名决定焊到 A 脚还是 B 脚；inA / inB 之外的值视为错误
				if (pin == "inA") {
					(ittarget->second)->connectSourceA(itsource->second);
					cout << "连线成功！" << endl<<endl;
				} else if(pin == "inB") {
					(ittarget->second)->connectSourceB(itsource->second);
					cout << "连线成功！" << endl<<endl;
				} else {
					cout<<"连线失败，请检查连线区"<<endl;
				}
			}
		}
	}
	infile.close(); // while 循环结束，电路已完全搭建在内存中
	// 收尾：网表读完，电路已在内存里搭好。
	// 现在 CONNECT 支持 inA / inB 两个脚，所以一个门可以让两个上游分别接 A、B（见 circuit.txt）。
}
// 找“悬空引脚”：扫描所有门，A 或 B 脚只要没接线就记成一个 Pin。
// 这些引脚就是电路的“外部输入”，测试激励必须从它们打进去；
// 返回的个数 n 决定了测试向量共有 2^n 组（RunNormalTest 里用 mask 穷举）。
vector<FaultSimulator::Pin> FaultSimulator::findEmptypin() {
	vector<Pin> nullpins;
	for(size_t i = 0; i < gates.size(); i++) {
		// A 脚悬空 → 记为 {门, 0}
		if(gates[i]->getSourceA() == nullptr) {
			nullpins.push_back({gates[i].get(),0});
		}
		// B 脚悬空 → 记为 {门, 1}
		if(gates[i]->getSourceB() == nullptr) {
			nullpins.push_back({gates[i].get(),1});
		}
	}
	return nullpins;
}
// 找“终端门”：把所有“当过别人上游”的门从花名册副本里删掉，剩下的就是没有下游的门。
// 终端门是最后要观测输出的对象；用 map 删除做剪枝，复杂度 O(N log N)。
vector<Gate*> FaultSimulator::findterminalGate() {
	// 复制一份花名册，专门用来“删掉那些当过上游的门”，不动原来的 Gatemap
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
// 正常模式：穷举全部测试向量，记录每组的终端输出。
//   向量个数 = 2^(悬空引脚数)；mask 的每一位对应一个悬空引脚，1 表示打高电平。
//   每换一组向量前都要把全部门 Reset()，否则上一轮残留的 evalState=2 会让门直接复用旧结果。
void FaultSimulator::RunNormalTest() {
	cout<<"---------正在进行正确测试-------------"<<endl;
	// 终端门和悬空引脚各算一次就够了，后面每组向量都复用这两份列表
	terminalGates = findterminalGate();
	vector<Pin>  nullpins = findEmptypin();
	int length = nullpins.size();
	// mask 从 0 数到 2^n - 1：每个 bit 对应 findEmptypin() 里的一个悬空引脚
	for(int mask = 0; mask < (1 << length); mask++) {
		vector<bool> CurrentOutputs;
		// ① 把本组激励按位打到所有悬空引脚上
		for(int i = 0; i < length; i++) {
			int bit = (mask >> i)&1 ;
			if(nullpins[i].index == 0) {
				nullpins[i].gate->setInputA(bit);
			} else if(nullpins[i].index == 1) {
				nullpins[i].gate->setInputB(bit);
			}

		}
		// ② 清空所有门的求值状态（关键：否则会被当成“已算完”而直接复用上一组的结果）
		for(size_t i = 0; i < gates.size(); i++) {
			gates[i]->Reset();
		}
		// ③ 打印本组激励机制（bit0 在最左边，和输出顺序一致）
		cout<<"测试注入";
		for(int i = 0; i < length; i++) {
			int bit = (mask >> i)&1 ;
			cout<<bit;
		}
		cout<<endl;
		// ④ 让每个终端门重新求值，并把输出记录下来
		for (size_t i = 0; i < terminalGates.size(); i++) {
			terminalGates[i]->evaluate();
			cout<<terminalGates[i]->name<<"的输出为"<< terminalGates[i]->getOutput()<<endl;
			CurrentOutputs.push_back(terminalGates[i]->getOutput());
		}
		// 一组向量的所有终端输出打包存起来，故障模式下要按同样的下标对齐比较
		normalResults.push_back(CurrentOutputs);
	}

}
// 故障模式：给 targetGate 注入 faultType（1=卡 0，2=卡 1）后，用同一套向量再跑一遍。
//   结果存进 badResults —— 必须与 normalResults 一一对应，ContrastResult 才能逐组比较。
//   跑完把 faultType 复位成 0，避免污染下一次测试。
// 每组向量开始前都会 Reset() 全部门（与 RunNormalTest 的做法一致），
// 所以不会读到上一轮遗留的 evalState=2 缓存值；否则第一组向量会复用旧结果，把命中率算歪。
void FaultSimulator::RunFaultTest(string targetGate,int faultType) {
	cout<<"---------正在进行错误测试-------------"<<endl;
	// 注入故障：真正生效的地方在 Gate::getOutput()（卡 0 恒返回 0，卡 1 恒返回 1）
	Gatemap[targetGate]->faultType = faultType;
	// 清空上一轮的故障结果
	badResults.clear();
	// 重新收集悬空引脚：保证与正常模式用的是同一套激励、同样的顺序
	vector<Pin> nullpins = findEmptypin();
	int length = nullpins.size();
	// 与 RunNormalTest 完全相同的 mask 穷举
	for(int mask = 0; mask < (1 << length); mask++) {
		vector<bool> CurrentOutputs;
		// 每组向量开始前先清空所有门的求值状态（关键：否则会被当成“已算完”而直接复用旧结果）
		for(size_t i = 0; i < gates.size(); i++) {
			gates[i]->Reset();
		}
		// 逐位设置激励（顺序与正常模式一致，才能和对面的基准对齐）
		for(int i = 0; i < length; i++) {
			int bit = (mask >> i)&1 ;
			if(nullpins[i].index == 0) {
				nullpins[i].gate->setInputA(bit);
			} else if(nullpins[i].index == 1) {
				nullpins[i].gate->setInputB(bit);
			}
		}

		// 终端门重新求值：递归取输入时会读到被注入故障的那个门，故障就这样传播到输出
		for (size_t i = 0; i < terminalGates.size(); i++) {
			terminalGates[i]->evaluate();
			CurrentOutputs.push_back(terminalGates[i]->getOutput());
		}
		// 本组向量的故障输出入列（下标与 normalResults 一一对应）
		badResults.push_back(CurrentOutputs);



	}
	// 测试结束，把故障恢复成 0（否则下一次 RunFaultTest 会带着旧故障跑）
	Gatemap[targetGate]->faultType = 0;

}
// 对比正常 / 故障两轮结果，算“这组测试向量抓到了几个故障”。
//   判定标准：同一组向量下，只要任意一个终端输出不同，就算抓到了（faults++ 后 break）。
//   命中率 = 抓到的向量数 / 向量总数 × 100%。
void FaultSimulator::ContrastResult() {
	// 两轮向量数不一致 → 数据对不上，直接报错
	if (normalResults.size() != badResults.size()) {
		cout<<"数据错误"<<endl;
	} else {
		// faults 记“抓到故障的向量个数”（用 float 是为了后面除法保留小数）
		float faults = 0;
		for (size_t i = 0; i < normalResults.size(); i++) {
			// 终端个数取第一组即可（每组向量的输出集合长度都相同）
			int length = normalResults[0].size();
			// 逐个终端比较：发现一个不同就判定本组向量命中了，跳出内层循环
			for(int n = 0; n < length; n++) {
				if(normalResults[i][n] != badResults[i][n]) {
					faults++;
					break;
				}
			}

		}
		// 打印统计结果：命中率 = 抓到的向量数 / 向量总数 × 100%
		cout<<"共发现"<<faults<<"个错误"<<endl;
		cout<<"向量命中率"<<faults/badResults.size()*100<<"%"<<endl;
	}

}
// 对外接口：按创建顺序返回门名，供 main 遍历注入故障
vector<string> FaultSimulator::getAllGateNames() {
	return allGateNames;
}
