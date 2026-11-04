#ifndef GATES_H
#define GATES_H

class Gate {
protected:
    bool inputa;
    bool inputb;
    bool output;
    Gate* inputsourceA;
    Gate* inputsourceB;

public:
    virtual ~Gate();  // 虚析构，带波浪号
    Gate();
    
    void connectSourceA(Gate* source);
    void connectSourceB(Gate* source);
    
    bool getRealA();
    bool getRealB();
    
    void setInput(bool a, bool b);
    bool getOutput();
    
    virtual void evaluate() = 0;
}; // 这里要有分号

class AndGate : public Gate { // 不要括号
public:
    void evaluate() override;
}; 

class OrGate : public Gate {
public:
    void evaluate() override;
}; // 这里要有分号

class XorGate : public Gate { // 首字母大写
public:
    void evaluate() override;
};

class NotGate : public Gate {
public:
    void evaluate() override;
};

class AorGate : public Gate {
public:
    void evaluate() override;
};

#endif
