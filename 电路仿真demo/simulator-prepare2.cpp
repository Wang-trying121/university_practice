#include "simulator.h"
#include<iostream>
#include <fstream>
using namespace std;
FaultSimulator::FaultSimulator():
	test_pins(0) {}
void FaultSimulator::loadCircuit(string filename) {
	ifstream infile(filename);
	if (!infile) {
		cout << "提取失败" << endl;
		return;
	}
	// --- 阶段 B：准备仓库（大货架 和 花名册） ---
	string type, name, name2,pin;
	cout << "文件打开成功！开始读取..." << endl <<endl;
	while(infile >> type >> name) {
		cout << "读到指令: " << type << " 名字: " << name << endl;
		// 分支 1：如果是造芯片的指令（非 CONNECT 指令）
		if (type != "CONNECT") {
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
				Gatemap[name] = newgate.get();     // 登记进花名册，便于按名字查找
				gates.push_back(std::move(newgate));    // 放进大货架，便于最后统一销毁
				allGateNames.push_back(name);  // 另外记一份顺序表，供 main 遍历
				cout << "成功创建门: " << name << endl<<endl;
			}
			// 若类型拼错（比如写成 BND），newgate 保持 nullptr，这个门被静默跳过。
			// 分支 2：如果是连线的指令（CONNECT）
		} else if(type == "CONNECT") {
			infile >> name2;// CONNECT 后面有两个名字，这里把第二个名字（目标门）读进 name2
			infile >> pin;
			auto itsource = Gatemap.find(name);
			auto ittarget = Gatemap.find(name2);
			cout << "尝试连线: " << name << " -> " << name2 <<"通过"<<pin<< endl;
			if (itsource == Gatemap.end() || ittarget == Gatemap.end()) {
				cout<<"出错了！找不到门"<<endl;
			} else {
				// 核心连线魔法！
				// 从花名册里找到目标门，把源门的地址传给目标门的输入 A 引脚
				// 相当于把一根物理导线从 name 的输出端，焊到了 name2 的输入端
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
	// 收尾说明：连线的目标脚固定是 A 脚，所以这份网表只支持“一个上游 + B 脚留空”
	// 的连接方式；想串两个上游，得再补一条能接 B 脚的指令。
}
vector<FaultSimulator::Pin> FaultSimulator::findEmptypin() {
	vector<Pin> nullpins;
	for(size_t i = 0; i < gates.size(); i++) {
		if(gates[i]->getSourceA() == nullptr) {
			nullpins.push_back({gates[i].get(),0});
		}
		if(gates[i]->getSourceB() == nullptr) {
			nullpins.push_back({gates[i].get(),1});
		}
	}
	return nullpins;
}
vector<Gate*> FaultSimulator::findterminalGate() {
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
void FaultSimulator::RunNormalTest() {
	cout<<"---------正在进行正确测试-------------"<<endl;
	terminalGates = findterminalGate();
	vector<Pin>  nullpins = findEmptypin();
	int length = nullpins.size();
	for(int mask = 0; mask < (1 << length); mask++) {
		vector<bool> CurrentOutputs;
		for(int i = 0; i < length; i++) {
			int bit = (mask >> i)&1 ;
			if(nullpins[i].index == 0) {
				nullpins[i].gate->setInputA(bit);
			} else if(nullpins[i].index == 1) {
				nullpins[i].gate->setInputB(bit);
			}

		}
		for(size_t i = 0; i < gates.size(); i++) {
			gates[i]->Reset();
		}
		cout<<"测试注入";
		for(int i = 0; i < length; i++) {
			int bit = (mask >> i)&1 ;
			cout<<bit;
		}
		cout<<endl;
		for (size_t i = 0; i < terminalGates.size(); i++) {
			terminalGates[i]->evaluate();
			cout<<terminalGates[i]->name<<"的输出为"<< terminalGates[i]->getOutput()<<endl;
			CurrentOutputs.push_back(terminalGates[i]->getOutput());
		}
		normalResults.push_back(CurrentOutputs);
	}

}
void FaultSimulator::RunFaultTest(string targetGate,int faultType) {
	cout<<"---------正在进行错误测试-------------"<<endl;
	Gatemap[targetGate]->faultType = faultType;
	badResults.clear();
	vector<Pin> nullpins = findEmptypin();
	int length = nullpins.size();
	for(int mask = 0; mask < (1 << length); mask++) {
		vector<bool> CurrentOutputs;
		for(int i = 0; i < length; i++) {
			int bit = (mask >> i)&1 ;
			if(nullpins[i].index == 0) {
				nullpins[i].gate->setInputA(bit);
			} else if(nullpins[i].index == 1) {
				nullpins[i].gate->setInputB(bit);
			}
		}

		for (size_t i = 0; i < terminalGates.size(); i++) {
			terminalGates[i]->evaluate();
			CurrentOutputs.push_back(terminalGates[i]->getOutput());
		}
		badResults.push_back(CurrentOutputs);



		for(size_t i = 0; i < gates.size(); i++) {
			gates[i]->Reset();
		}
	}
	Gatemap[targetGate]->faultType = 0;

}
void FaultSimulator::ContrastResult() {
	if (normalResults.size() != badResults.size()) {
		cout<<"数据错误"<<endl;
	} else {
		float faults = 0;
		for (size_t i = 0; i < normalResults.size(); i++) {
			int length = normalResults[0].size();
			for(int n = 0; n < length; n++) {
				if(normalResults[i][n] != badResults[i][n]) {
					faults++;
					break;
				}
			}

		}
		cout<<"共发现"<<faults<<"个错误"<<endl;
		cout<<"向量命中率"<<faults/badResults.size()*100<<"%"<<endl;
	}

}
vector<string> FaultSimulator::getAllGateNames() {
	return allGateNames;
}
