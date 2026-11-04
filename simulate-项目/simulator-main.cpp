#include <iostream>
#include <vector>
#include"Gate.h"
using namespace std;
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

