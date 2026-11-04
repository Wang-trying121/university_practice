#include<bits/stdc++.h>
using namespace std;
class Gate {
	protected://只能再本身和子辈中用 
		bool inputa;
		bool inputb;
		bool output;//bool类型 
		Gate* inputsourceA = nullptr;//Gate类型 
		Gate* inputsourceB = nullptr;
	public://全局可用 
		virtual ~Gate () {}//？ 
		Gate() :inputa(false),inputb(false),output(false) {}//初始化 
		void connectSourceA(Gate* source) {//传入source指针，链接上一个结果 
			inputsourceA = source;
		}
		void connectSourceB(Gate* source) {
			inputsourceB = source;
		}
		bool getRealA() {
			if(inputsourceA == nullptr) {//没有连线就用默认值 
				return inputa;
			} else {
				inputsourceA->evaluate();//有连线向上一个器件逼问 
				return inputsourceA->getOutput();
			}
		}
		bool getRealB() {
			if(inputsourceB == nullptr) {
				return inputb;
			} else {
				inputsourceB->evaluate();
				return inputsourceB->getOutput();
			}
		}
		void setInput(bool a,bool b) {
			inputa = a;//设置默认值 
			inputb = b;
		}
		bool getOutput() {
			return output;
		}
		virtual void evaluate() = 0;//虚拟函数，交给子辈的任务 
};
class AndGate:public Gate {//class是类，这是Gate子辈的类 
	public:
		void evaluate() override {//重写evaluate 
			output = getRealA()&&getRealB();
		}
};
class OrGate:public Gate {
	public:
		void evaluate() override {
			output = getRealA()||getRealB();
		}
};
class XorGate:public Gate {
	public:
		void evaluate() override {
			output = (getRealA() != getRealB());
		}
};
class NotGate : public Gate {
	public:
		void evaluate() override {
			output = !getRealA();
		}
};
class AorGate: public Gate {
	public:
		void evaluate() override {
			output = (getRealA() == getRealB());
		}
};
void PrintTruthTable(Gate* g) {//打印函数 
	for(int i = 0; i<2; i++) {
		for(int j = 0; j<2; j++) {
			g->setInput(i,j);
			g->evaluate();
			cout<<"A="<<i<<" B="<<j<<" 输出="<< g->getOutput()<<endl;
		}
	}
}
class Circuit { 
	private:
		vector<Gate*> gates;
	public:
		void addGate(Gate* g){
			gates.push_back(g);
		}
		void evaluateAll(){
			for (size_t i = 0;i<gates.size();i++){
				gates[i]->evaluate();
			}
		
		}
		~Circuit(){
			cout<<"系统提示，正在清理。。。"<<endl;
			for (size_t i=0;i<gates.size();i++){
				delete gates[i]; 
			}
		} 
		
	
};
int main() {
	/*vector<Gate*> myBox;
	myBox.push_back(new AndGate());
	myBox.push_back(new OrGate());
	myBox.push_back(new NotGate());
	for(size_t i = 0;i < myBox.size();i++){
		myBox[i]->setInput(1,1);
		myBox[i]->evaluate();
		cout<<"这是第"<<i+1<<"个门，输出"<<myBox[i]->getOutput()<<endl;
	}
	for(size_t i = 0;i < myBox.size();i++){
		delete myBox[i];
	}*/

	/*cout<<"--------与门--------"<<endl;
	AndGate and1;
	PrintTruthTable(&and1);
	cout<<"--------或门--------"<<endl;
	OrGate or1;
	PrintTruthTable(&or1);
	cout<<"--------异或门--------"<<endl;
	XorGate xor1;
	PrintTruthTable(&xor1);
	return 0;*/
	{
		Circuit myChip;
		myChip.addGate(new AndGate());
		myChip.addGate(new OrGate());
		myChip.addGate(new NotGate());
		myChip.evaluateAll();
	}
	return 0;
}

