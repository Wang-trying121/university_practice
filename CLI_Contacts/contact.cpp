#include<bits/stdc++.h>
using namespace std;
struct Contact {
	string name;
	string number;
	string email;
	string group;
};
//保存函数
void saveContacts(vector<Contact> &contacts) {
	ofstream outFile("联系人.txt");
	if (!outFile) {
		cout<<"文件打开失败"<<endl;
		return;
	}
	for(size_t n = 0; n < contacts.size(); n++) {
		outFile<<contacts[n].name<<endl;
		outFile<<contacts[n].number<<endl;
		outFile<<contacts[n].email<<endl;
		outFile<<contacts[n].group<<endl;
	}
	outFile.close();
	cout<<"保存成功"<<endl;
}
//加载函数
void loadContacts(vector<Contact> &contacts) {
	ifstream inFile("联系人.txt");
	if (!inFile) {
		cout<<"新建通讯录"<<endl;
		return;
	}
	Contact c;
	while(getline(inFile,c.name)) {
		getline(inFile,c.number);
		getline(inFile,c.email);
		getline(inFile,c.group);
		contacts.push_back(c);
	}
	inFile.close();
}
//填写函数
void typeIn(const string &prompt,string &target) {
	while(true) {
		cout<<prompt<<endl;
		getline(cin,target);
		if (target.empty()) {
			cout<<prompt<<"不能为空"<<endl;
		} else {
			break;
		}
	}
}
//添加
void Contactadd(vector<Contact> &contacts) {
	cout<<"添加联系人"<<endl;
	Contact c;
	typeIn("姓名",c.name);
	string check = c.name;
	bool find = 0;
	for(size_t n = 0; n < contacts.size(); n++) {
		if(contacts[n].name == check) {
			cout<<"已有该联系人"<<endl;
			find = 1;
			break;
		}
	}
	if(find == 0) {
		typeIn("电话",c.number);
		typeIn("邮箱",c.email);
		typeIn("分组",c.group);
		contacts.push_back(c);
		cout<<"添加成功"<<endl;



	}
}
bool compareByname(const Contact& a,const Contact& b) {
	return a.name < b.name;
}
void sortContact(vector<Contact> &contacts) {
	sort(contacts.begin(),contacts.end(),compareByname);
}
//展示
void showALLcontact(const vector<Contact> &contacts) {
	if(contacts.empty()) {
		cout<<"当前目录为空"<<endl;
	}
	set<string> groups;
	for(size_t i = 0; i < contacts.size(); i++) {
		groups.insert(contacts[i].group);
	}
	for (const string& g: groups) {
		cout<<"["<<g<<"]"<<endl;
		for (size_t i = 0; i < contacts.size(); i++) {
			if(contacts[i].group == g) {
				cout<<contacts[i].name<<" "<<contacts[i].number<<" "<<contacts[i].email<<endl;
			}
		}
	}
}
void Contactshow(vector<Contact> &contacts) {
	cout<<"显示联系人"<<endl;
	sortContact(contacts);
	if(contacts.size() == 0) {
		cout<<"暂无联系人"<<endl;
	} else {
		for(size_t n = 0; n < contacts.size(); n++) {
			cout<<contacts[n].name<<" "<<contacts[n].number<<endl;
			cout<<contacts[n].email<<" "<<contacts[n].group<<endl<<endl;
		}
	}
}
//查寻
void Contactsask(const vector<Contact> &contacts) {
	cout<<"查询"<<endl;
	cout<<"请输入您要查询的对象名"<<endl;
	string target;
	getline(cin,target);
	bool find = 0;
	if(contacts.size() == 0) {
		cout<<"暂无联系人"<<endl;
	}

	else {
		for(size_t n = 0; n < contacts.size(); n++) {
			if(contacts[n].name == target) {
				cout<<contacts[n].name<<" "<<contacts[n].number<<endl;
				cout<<contacts[n].email<<" "<<contacts[n].group<<endl<<endl;
				find = 1;
				break;
			}
		}
		if (find == false) {
			cout<<"未找到该联系人"<<endl;
		}

	}
}
//删除
void Contactsdelete(vector<Contact> &contacts) {
	cout<<"删除"<<endl;
	cout<<"请输入您要删除的对象名"<<endl;
	string target;
	getline(cin,target);
	bool find = 0;
	if(contacts.size() == 0) {
		cout<<"暂无联系人"<<endl;
	} else {
		for(size_t n = 0; n < contacts.size(); n++) {
			if(contacts[n].name == target) {
				contacts.erase(contacts.begin() + n);
				find = 1;
				cout<<"删除成功"<<endl;
				break;
			}
		}
		if(find == 0) {
			cout<<"未找到该联系人"<<endl;
		}
	}
}
//更新
void updateContact(vector<Contact> &contacts) {
	cout<<"请输入修改人姓名"<<endl;
	string target;
	typeIn("姓名",target);
	bool find = 0;
	for(size_t n = 0; n < contacts.size(); n++) {
		if(target == contacts[n].name) {
			typeIn("姓名",contacts[n].name);
			typeIn("电话",contacts[n].number);
			typeIn("邮箱",contacts[n].email);
			typeIn("分组",contacts[n].group);
			find = 1;
			cout<<"修改成功"<<endl;
			break;
		}
	}
	if (!find) {
		cout<<"未找到"<<endl;
	}
}

int main() {
	vector<Contact> contacts;
	loadContacts(contacts);
	int choice = -1;
	cout<<"hello"<<endl;
	while(choice != 0) {
		cout<<"1 添加 2 显示 3 查询 4 修改 5 删除 0 退出 6 通讯录"<<endl;
		cin>>choice;
		cin.ignore();
		if (cin.fail()) {
			cin.clear();
			cin.ignore(1000,'\n');
			cout<<"请输入数字"<<endl;
			continue;
		}
		switch(choice) {
			case 1:
				Contactadd(contacts);
				break;
			case 2:
				Contactshow(contacts);
				break;
			case 3:
				Contactsask(contacts);
				break;
			case 4:
				updateContact(contacts);
				break;
			case 5:
				Contactsdelete(contacts);
				break;
			case 6:
				showALLcontact(contacts) ;
				break;
			case 0:
				cout<<"退出"<<endl;
				break;
			default:
				cout<<"输入错误"<<endl;
				break;
		}
	}
	saveContacts(contacts);

	return 0;
}


