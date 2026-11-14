// ============================================================================
// 文件：simulator-prepare.cpp
// 作用：Gate 及所有具体逻辑门的成员函数实现（对应 Gate.h 的声明）
//
// 核心机制：深度优先递归求值 + evalState 状态机。
//   终端门调用 evaluate() 时，会通过 inputsourceA/B 指针一路向上游“要值”，
//   所以不需要先对门排序、也不需要遍历整张网表——谁想要值，谁就自己往上游追。
//   每个门用 evalState 记录：0 未算 / 1 算到一半 / 2 算完。好处有两个：
//   ① 遇到环形连线能及时收手，不会栈溢出；② 算过的门直接复用结果，避免重复递归。
// ============================================================================
#include "Gate.h"
#include<iostream>
using namespace std;

// ----------------------------------------------------------------------------
// 构造 / 析构
// ----------------------------------------------------------------------------

Gate::Gate ():
	inputa(false),inputb(false),output(false) ,inputsourceA(nullptr), inputsourceB(nullptr),faultType(0) {}
// 初始化列表从左到右赋值：三个电平默认 false（未通电），两个引脚指针默认 nullptr
// （代表悬空/没接线），faultType 默认 0（正常，不注入故障）。

Gate::~Gate() {}
// 本类不拥有堆内存：引脚只是“借用”别人的地址，不负责 delete 上游门，
// 所以析构函数留空。写成 virtual 是为了通过 Gate* 删除派生门时析构链正确。

// ----------------------------------------------------------------------------
// 连线：把上游门的地址记到自己的输入脚上
// ----------------------------------------------------------------------------

void Gate::connectSourceA(Gate* source) {
	inputsourceA = source;
}
void Gate::connectSourceB(Gate* source) {
	inputsourceB = source;
}
// 注意：这里只记地址，不复制值。所以上游门之后怎么变，这边读到的永远是最新值，
// 相当于真的接了一根导线，而不是快照。

// ----------------------------------------------------------------------------
// 取输入电平：A 脚
// ----------------------------------------------------------------------------

// getRealA()：A 脚取值的唯一入口，也是“信号沿导线自动传播”的关键。
//   1) A 脚没接线（nullptr）→ 直接返回 inputa，也就是 setInput() / setInputA() 打进来的激励；
//   2) A 脚接了线          → 先看上游门的 evalState 再决定怎么取值（四种状态见下面各分支）。
// 递归会一路往上游追，直到源头门为止——这就是整个仿真器的灵魂。
bool Gate::getRealA() {
	if(inputsourceA == nullptr) {//没有连线就用默认值
		return inputa;
	} else {
		int st = inputsourceA->get_evalState();
		// 状态 0：上游还没算过 → 递归让上游算一遍，再取它的输出
		if(st == 0) {
			inputsourceA->evaluate();//有连线向上一个器件逼问
			return inputsourceA->getOutput();
		// 状态 1：上游正在算 → 说明连线成环、绕回了自己，报“死循环”并返回 0 兜底
		}else if(st == 1){
			cout<<"警告：出现死循环"<<endl;
			return 0;
		// 状态 2：上游已经算完 → 直接取缓存值，省掉重复递归（同一门一次求值只算一遍）
		}else if(st == 2){
			return inputsourceA->getOutput();
		// 其他值：理论上不会出现，打印报错兜底
		}else{
			cout<<"程序出错，请检查getReal函数"<<endl;
			return 0;
		}

	}
}

// getRealB()：B 脚取值，逻辑与 getRealA() 完全对称（同样带 evalState 状态机判断）。
bool Gate::getRealB() {
	if(inputsourceB == nullptr) {//没有连线就用默认值
		return inputb;
	} else {
		int st = inputsourceB->get_evalState();
		// 与 getRealA() 相同的状态机：0 未算 → 递归；1 成环 → 兜底；2 已完成 → 取缓存
		if(st == 0) {
			inputsourceB->evaluate();//有连线向上一个器件逼问
			return inputsourceB->getOutput();
		// 状态 1：成环，直接返回 0，避免无限递归导致栈溢出
		}else if(st == 1){
			cout<<"警告：出现死循环"<<endl;
			return 0;
		// 状态 2：直接复用上次算好的输出
		}else if(st == 2){
			return inputsourceB->getOutput();
		// 非预期的状态值，报错兜底
		}else{
			cout<<"程序出错，请检查getReal函数"<<endl;
			return 0;
		}

	}
}

// ----------------------------------------------------------------------------
// 判断是不是“源头门”
// ----------------------------------------------------------------------------

// 用途：找出电路里“没有上游”的门，测试激励必须直接打在它们身上。
// 判断条件是 inputsourceA 和 inputsourceB 都为空：A、B 两个脚都悬空才算源头门。

bool Gate::ifsourceGate() {
	// A、B 两个脚都没接线 → 没有上游，判定为源头门
	if (inputsourceA == nullptr && inputsourceB == nullptr) {
		return 1;
	} else {
		return 0;
	}

}

// ----------------------------------------------------------------------------
// 施加激励 / 读取输出
// ----------------------------------------------------------------------------

// 给门手动打测试激励：只对“引脚悬空的源头门”有意义。
// 对已经接了线的门调用 setInput() 不会报错，但那个值会被 getRealA/B() 里的
// 上游取值覆盖掉，形同无效。
void Gate::setInput(bool a,bool b) {
	inputa = a;//设置默认值
	inputb = b;
}

// 读输出，同时在这一步完成故障注入：
//   faultType == 0 → 正常，返回内部算出的 output
//   faultType == 1 → 卡 0 故障：不管内部算出什么，对外一律表现为 0
//   faultType == 2 → 卡 1 故障：不管内部算出什么，对外一律表现为 1
//   ？？？         → 非法值，按正常处理
// 因为下游门只能通过 getOutput() 取上游的值，所以故障会顺着连线自然传播到终端，
// 这正是后面算“测试向量命中率”的基础。
bool Gate::getOutput() {
	if (faultType == 0) {
		return output;
	} else if(faultType == 1) {
		return 0;
	} else if (faultType == 2) {
		return 1;
	} else {
		return output;
	}
}

// ----------------------------------------------------------------------------
// 各门的求值实现：只有一行——按自己的真值表达式更新 output
// ----------------------------------------------------------------------------

// 所有 evaluate() 都是同一个三步套路：
//   ① evalState = 1  标记“正在算”，这样若有环，绕回来的递归才会发现 st == 1；
//   ② 按本门真值表达式更新 output（内部调用 getRealA/B 递归取输入）；
//   ③ evalState = 2  标记“算完了”，之后同一轮里再被问就直接复用 output。
void AndGate::evaluate() {  // 与门：两个输入都为 1 才输出 1
	evalState = 1;
	output = getRealA()&&getRealB();
	evalState = 2;
}
void OrGate::evaluate() {   // 或门：只要有一个输入为 1 就输出 1
	evalState = 1;
	output = getRealA()||getRealB();
	evalState = 2;
}
void NotGate::evaluate() {  // 非门：取反（B 脚不参与计算）
	evalState = 1;
	output = !getRealA();
	evalState = 2;
}
void XorGate::evaluate() {  // 异或门：两输入不同输出 1，相同输出 0
	evalState = 1;
	output = !(getRealA() == getRealB());
	evalState = 2;
}
void AorGate::evaluate() {  // 同或门 XNOR：两输入相同输出 1（异或取反）
	evalState = 1;
	output = (getRealA() == getRealB());
	evalState = 2;
}
