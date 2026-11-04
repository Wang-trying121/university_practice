#include "Gate.h"
Gate::Gate ():
inputa(false),inputb(false),output(false) ,inputsourceA(nullptr), inputsourceB(nullptr) {}
Gate::~Gate() {}
void Gate::connectSourceA(Gate* source) {
	inputsourceA = source;
}
void Gate::connectSourceB(Gate* source) {
	inputsourceB = source;
}
bool Gate::getRealA() {
	if(inputsourceA == nullptr) {//没有连线就用默认值
		return inputa;
	} else {
		inputsourceA->evaluate();//有连线向上一个器件逼问
		return inputsourceA->getOutput();
	}
}
bool Gate::getRealB() {
	if(inputsourceB == nullptr) {
		return inputb;
	} else {
		inputsourceB->evaluate();
		return inputsourceB->getOutput();
	}
}
void Gate::setInput(bool a,bool b) {
	inputa = a;//设置默认值
	inputb = b;
}
bool Gate::getOutput() {
	return output;
}
void AndGate::evaluate(){
	output = getRealA()&&getRealB();
}
void OrGate::evaluate(){
	output = getRealA()||getRealB();
}
void NotGate::evaluate(){
	output = !getRealA();
}
void XorGate::evaluate(){
	output = !(getRealA() == getRealB());
}
void AorGate::evaluate(){
	output = (getRealA() == getRealB());
}
