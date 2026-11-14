#ifndef SIMULATOR_H
#define SIMULATOR_H
#include <vector>
#include <map>
#include <memory>
#include "Gate.h"
#include <string>
using namespace std;
class FaultSimulator {
	private:
		vector<unique_ptr<Gate>> gates;
		map<string, Gate*> Gatemap;
		vector<string> allGateNames;

		vector<Gate*> terminalGates;
		int test_pins;
		vector<vector<bool>> normalResults;
		vector<vector<bool>> badResults;
	public:
		struct Pin {
			Gate* gate;
			int index;
		};
		FaultSimulator();
		void loadCircuit(string filename);
		vector<Pin> findEmptypin();
		vector<Gate*> findterminalGate();
		void RunNormalTest();
		void RunFaultTest(string targetGate,int faultType);
		vector<string> getAllGateNames();
		void ContrastResult();
};
#endif

