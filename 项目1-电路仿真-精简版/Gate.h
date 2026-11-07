// ============================================================================
// 文件：Gate.h
// 作用：逻辑电路仿真的“元件库”——抽象基类 Gate 与 5 种具体逻辑门
// 说明：本文件只放声明，所有成员函数的实现都在 simulator-prepare.cpp 里。
//
// 设计思路（用一个对象模拟一个真实器件）：
//   一个“门”就是内存里的一个对象。它的 output 由输入算出来；输入要么来自
//   自己身上的 inputa/inputb（没接线的“悬空脚”），要么来自另一个门——
//   用 Gate* 指针挂在 inputsourceA/inputsourceB 上，指针就相当于现实里的导线。
//   因为兄弟类型都派生自 Gate，所以一根“线”可以接任何门的输出，这就是多态。
// ============================================================================
#ifndef GATES_H
#define GATES_H
#include <string> 

class Gate {
protected:
    bool inputa;            // 输入脚 A 的自带电平：只有 A 脚没接线（悬空）时才会被用到
    bool inputb;            // 输入脚 B 的自带电平：同上
    bool output;            // 本门上一次 evaluate() 算出来的结果
    Gate* inputsourceA;     // A 脚接在哪个门的输出上；nullptr = 没接线
    Gate* inputsourceB;     // B 脚接在哪个门的输出上；nullptr = 没接线

public:
    virtual ~Gate();  // 虚析构，带波浪号：保证用 Gate* delete 派生门时能正确析构
    Gate();           // 默认构造：电平全 false，两个引脚指针 nullptr，faultType 为 0
    
    void connectSourceA(Gate* source);   // 把 source 的输出“焊”到本门的 A 脚
    void connectSourceB(Gate* source);   // 把 source 的输出“焊”到本门的 B 脚
    Gate* getSourceA(){return inputsourceA;}   // 查 A 脚接的是谁（没接返回 nullptr）
    Gate* getSourceB(){return inputsourceB;}   // 查 B 脚接的是谁（没接返回 nullptr）
    
    bool getRealA();     // 取 A 脚的真实输入：接了线就递归问上游，没接就用 inputa
    bool getRealB();     // 取 B 脚的真实输入：同上，逻辑与 A 脚对称
    bool ifsourceGate(); // 判断本门是否“源头门”（只看 A 脚是否悬空，详见 .cpp 注释）
    
    void setInput(bool a, bool b);   // 给源头门手动加测试激励（写 inputa/inputb）
    bool getOutput();                // 读输出，并在这一步完成故障注入（看 faultType）
    std::string name;                // 门名，对应 circuit.txt 里写的名字，如 chipX
    int faultType;                   // 故障类型：0=正常，1=卡在0（恒输出0），2=卡在1（恒输出1）
    virtual void evaluate() = 0;     // 纯虚函数：各门自己实现“怎么算 output”，故 Gate 不能直接实例化
}; // 这里要有分号

// ----------------------------------------------------------------------------
// 具体逻辑门：每个派生类只重写 evaluate()，连线、取输入、故障注入全部继承。
// ----------------------------------------------------------------------------

class AndGate : public Gate { // 与门：output = A && B
public:
    void evaluate() override;
}; 

class OrGate : public Gate { // 或门：output = A || B
public:
    void evaluate() override;
}; // 这里要有分号

class XorGate : public Gate { // 异或门：output = A != B（两输入不同时为 1）
public:
    void evaluate() override;
};

class NotGate : public Gate { // 非门：output = !A（只用 A 脚，B 脚被忽略）
public:
    void evaluate() override;
};

class AorGate : public Gate { // 同或门 XNOR：output = (A == B)；名字沿用 Aor，实为“同或”
public:
    void evaluate() override;
};

#endif
