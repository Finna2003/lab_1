#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
/*#define _CRT_SECURE_NO_DEPRECATE*/

using namespace std;

/*
* П(КC, КМ, ціна, кабінет) - процедура
* К(КC, ім'я, прізвище) - клієнт
* КМ(...) - майстер
*/


typedef struct {
	char name[20];
	char surname[20];
} Client;


typedef struct {
	int km;
	int price;
	int cabinet;
} Procedure;

struct ProcedureNode {
	ProcedureNode(Procedure* procedure, const int& _numberSn, const int& _next) { this->procedure = *procedure, previous = -1; next = _next; numberSn = _numberSn; }
	ProcedureNode() { previous = -1; next = -1; numberSn = -1; }
	Procedure procedure;
	int previous;
	int next;
	int numberSn;
};

struct ClientNode {
	ClientNode(const Client* s, const int &_numberIndex) { firstProcedure = -1; client = *s; numberProcedure = 0; numberIndex = _numberIndex; }
	ClientNode() { firstProcedure = -1; numberProcedure = 0; numberIndex = -1; }
	int firstProcedure;
	Client client;
	int numberProcedure;
	int numberIndex;
};

struct ClientIndexNode {
	ClientIndexNode() { numberFl = -1; KC = -1; }
	ClientIndexNode(const int &_numberFl, const int &_KS) { numberFl = _numberFl; KC = _KS; }
	int numberFl;
	int KC;
};


template <typename T, typename N>
class FileConnector
{
public:
	FileConnector(const string &fileName, const ios_base::openmode &mode ,const int &sizeBlock, const ios_base::openmode &modeAd = NULL)
	{
		this->fileName = fileName;
		this->mode = mode;
		this->sizeBlock = sizeBlock;
		file.open(fileName, mode | ios_base::binary | modeAd);
		if (!file.is_open())
			throw exception("FileConnector");
		if ((GetSize() * sizeBlock) % sizeBlock != 0)
			throw exception("HealDamaged");
	}

	~FileConnector() { file.close(); }

	int GetSize()
	{
		if (mode != ios_base::app)
			file.seekg(0, ios_base::end);
		return ((int)file.tellg() / sizeBlock);
	}

	N ReadLast()
	{
		return this->Read(this->GetSize());
	}

	void WriteEnd(const N *node)
	{
		this->Write(node, (GetSize() + 1));
	}

	void Write(const N* node, const int& number)
	{
		if (number <= 0)
			throw exception("WriteNegative");
		if (typeid(T) == typeid(ifstream))
			throw exception("WriteInIfstream");
		file.seekg((number - 1) * sizeBlock, ios_base::beg);
		file.write((char*)node, sizeBlock);
	}

	N Read(const int& number)
	{
		if (number <= 0)
			throw exception("ReadNegative");
		if (typeid(T) == typeid(ofstream) || mode == ios_base::app)
			throw exception("ReadInOfstream");
		N node;
		file.seekg((number - 1) * sizeBlock, ios_base::beg);
		file.read((char*)&node, sizeBlock);
		return node;
	}

	void DeleteLast()
	{
		if (typeid(T) == typeid(ifstream) || mode == ios_base::app)
			throw exception("ReadInOfstream");
		int size = (GetSize() - 1) * sizeBlock;
		char* buf = new char[size];
		file.seekg(0, ios_base::beg);
		file.read(buf, size);
		file.close();
		ofstream tempFile(fileName, ios_base::out, ios_base::binary);
		if (!tempFile.is_open())
			throw exception("DeleteLast");
		tempFile.write(buf, size);
		tempFile.close();
		delete[] buf;
		file.open(fileName);
		if (!file.is_open())
			throw exception("DeleteLast");
	}

	T& GetFile() { return file; }
private:
	T file;
	ios_base::openmode mode;
	int sizeBlock;
	string fileName;
};

class DbConnector
{
public:
	DbConnector(bool isTrunc = false)
	{
		//create files
		ofstream fileSFl (fileSFlName, (isTrunc ? ios_base::out : ios_base::app) | ios_base::binary);
		ofstream fileSInd(fileSIndName, (isTrunc ? ios_base::out : ios_base::app) | ios_base::binary);
		ofstream fileSpFl(fileSpFlName, (isTrunc ? ios_base::out : ios_base::app) | ios_base::binary);
		if (!fileSFl.is_open() || !fileSInd.is_open() || !fileSpFl.is_open())
			throw exception("ConstructorDbConnector");
		fileSFl.close();
		fileSInd.close();
		fileSpFl.close();
	}

	int InsertM(Client* s)
	{
		if (s == nullptr)
			throw exception("InsertMNull");
		FileConnector<fstream, ClientNode> fileSFl(fileSFlName, ios_base::in, sizeof(ClientNode), iostream::out);
		FileConnector<fstream, ClientIndexNode> fileSInd (fileSIndName, ios_base::in, sizeof(ClientIndexNode), ios_base::out);

		ClientNode* cn = new ClientNode(s, fileSInd.GetSize() + 1);
		ClientIndexNode* cin = new ClientIndexNode((fileSFl.GetSize() + 1), fileSInd.GetSize() != 0 ? (fileSInd.ReadLast().KC + 1) : 1);

		fileSFl.WriteEnd(cn);
		fileSInd.WriteEnd(cin);

		int KC = cin->KC;
		delete cn;
		delete cin;

		return KC;
	}

	bool InsertS(int KC, Procedure* r)
	{
		if (r == nullptr)
			throw exception("InsertSNull");
		FileConnector<fstream, ClientNode> fileSFl(fileSFlName, ios_base::in, sizeof(ClientNode), iostream::out);
		FileConnector<fstream, ProcedureNode> fileSpFl(fileSpFlName, ios_base::in, sizeof(ProcedureNode), iostream::out);

		int numberSn = GetM(KC);
		if (numberSn < 0)
			return false;

		ClientNode cn = fileSFl.Read(numberSn);
		ProcedureNode* newFirtsPn = new ProcedureNode(r, numberSn, cn.firstProcedure);
		int numberPn = fileSpFl.GetSize() + 1;

		if (cn.firstProcedure != -1)
		{
			ProcedureNode oldFirstPn = fileSpFl.Read(cn.firstProcedure);
			oldFirstPn.previous = numberPn;
			fileSpFl.Write(&oldFirstPn, cn.firstProcedure);
		}

		cn.firstProcedure = numberPn;
		cn.numberProcedure = cn.numberProcedure + 1;
		fileSFl.Write(&cn, numberSn);
		fileSpFl.WriteEnd(newFirtsPn);

		delete newFirtsPn;
		return true;
	}

	int GetS(int KC, int KM)
	{
		FileConnector<ifstream, ClientNode> fileSFl(fileSFlName, ios_base::in, sizeof(ClientNode));
		FileConnector<ifstream, ProcedureNode> fileSpFl(fileSpFlName, ios_base::in, sizeof(ProcedureNode));

		int numberCn = GetM(KC);
		if (numberCn < 0)
			return -1;
		ClientNode cn = fileSFl.Read(numberCn);
		if (cn.firstProcedure < 0)
			return -1;

		int numberPn = -1;
		ProcedureNode pn = fileSpFl.Read(cn.firstProcedure);
		if (pn.procedure.km == KM)
		{
			numberPn = cn.firstProcedure;
		}
		else
		{
			int number = pn.next;
			for (int i = 1; i < cn.numberProcedure; i++)
			{
				pn = fileSpFl.Read(pn.next);
				if (pn.procedure.km == KM)
				{
					numberPn = number;
					break;
				}
				number = pn.next;
				
			}
		}
		return numberPn;
	}

	int GetM(int KC)
	{
		return this->BinarySearchNumberFl(KC);
	}	

	bool UpdateM(int KC, string field, char value[20])
	{
		int numberCn = GetM(KC);
		if (numberCn < 0)
			return false;
		FileConnector<fstream, ClientNode> fileSFl(fileSFlName, ios_base::in, sizeof(ClientNode), iostream::out);
		ClientNode cn = fileSFl.Read(numberCn);

		if (field == "name")
			memcpy((void*)&cn.client.name, (void*)value, 20);
		else if (field == "surname")
			memcpy((void*)&cn.client.surname, (void*)value, 20);
		else
			return false;

		fileSFl.Write(&cn, numberCn);
		return true;
	}

	bool UpdateS(int KC, int KM, string field, char value[4])
	{
		int numberPn = GetS(KC, KM);
		if (numberPn < 0)
			return false;
		FileConnector<fstream, ProcedureNode> fileSpFl(fileSpFlName, ios_base::in, sizeof(ProcedureNode), iostream::out);
		ProcedureNode pn = fileSpFl.Read(numberPn);

		if (field == "price")
			memcpy((void*)&pn.procedure.price, (void*)value, sizeof(int));
		else if (field == "cabinet")
			memcpy((void*)&pn.procedure.cabinet, (void*)value, sizeof(int));
		else
			return false;

		fileSpFl.Write(&pn, numberPn);
		return true;
	}

	bool DelS(int numberPn)
	{
		FileConnector<fstream, ClientNode> fileSFl(fileSFlName, ios_base::in, sizeof(ClientNode), ios_base::out);
		FileConnector<fstream, ProcedureNode> fileSpFl(fileSpFlName, ios_base::in, sizeof(ProcedureNode), ios_base::out);

		ProcedureNode pn = fileSpFl.Read(numberPn);
		ClientNode cn = fileSFl.Read(pn.numberSn);
		if (pn.previous > 0)
		{
			ProcedureNode rnPrevious = fileSpFl.Read(pn.previous);
			if (pn.next != -1)
				rnPrevious.next = pn.next;
			else
				rnPrevious.next = -1;
			fileSpFl.Write(&rnPrevious, pn.previous);
		}
		if (pn.next > 0)
		{
			ProcedureNode pnNext = fileSpFl.Read(pn.next);
			if (pn.previous != -1)
				pnNext.previous = pn.previous;
			else
			{
				pnNext.previous = -1;
				cn.firstProcedure = pn.next;
			}
			fileSpFl.Write(&pnNext, pn.next);
		}
		if (pn.next == -1 && pn.previous == -1)
			cn.firstProcedure = -1;
		cn.numberProcedure = cn.numberProcedure - 1;
		fileSFl.Write(&cn, pn.numberSn);

		ProcedureNode pnLast = fileSpFl.ReadLast();
		fileSpFl.Write(&pnLast, numberPn);
		if (fileSpFl.GetSize() != numberPn)
		{
			if (pnLast.previous == -1)
			{
				ClientNode cn = fileSFl.Read(pnLast.numberSn);
				cn.firstProcedure = numberPn;
				fileSFl.Write(&cn, pnLast.numberSn);
			}
			else
			{
				ProcedureNode pnPrevious = fileSpFl.Read(pnLast.previous);
				pnPrevious.next = numberPn;
				fileSpFl.Write(&pnPrevious, pnLast.previous);
			}
			if (pnLast.next > 0)
			{
				ProcedureNode pnNext = fileSpFl.Read(pnLast.next);
				pnNext.previous = numberPn;
				fileSpFl.Write(&pnNext, pnLast.next);
			}
		}
		fileSpFl.DeleteLast();
		return true;
	}

	bool DelS(int KC, int KM)
	{

		int numberPn = GetS(KC, KM);
		if (numberPn < 0)
			return false;
		if (DelS(numberPn))
			return true;
		return false;
	}

	bool DelM(int KC)
	{
		FileConnector<fstream, ClientNode> fileSFl(fileSFlName, ios_base::in, sizeof(ClientNode), ios_base::out);
		FileConnector<fstream, ClientIndexNode> fileSInd(fileSIndName, ios_base::in, sizeof(ClientIndexNode), ios_base::out);
		FileConnector<fstream, ProcedureNode> fileSpFl(fileSpFlName, ios_base::in, sizeof(ProcedureNode), ios_base::out);

		int numberCn = GetM(KC);
		if (numberCn < 0)
			return false;
		ClientNode sn = fileSFl.Read(numberCn);

		int cur = sn.firstProcedure;
		for (int i = 0; i < sn.numberProcedure; i++)
		{
			int pnNext = fileSpFl.Read(cur).next;
			fileSpFl.GetFile().close();
			if (!DelS(cur))
				return false;
			fileSpFl.GetFile().open(fileSpFlName);
			cur = pnNext;
		}

		ClientNode cnLast = fileSFl.ReadLast();
		fileSFl.Write(&cnLast, numberCn);
		fileSFl.DeleteLast();
		if (fileSFl.GetSize() > 0)
		{
			cur = cnLast.firstProcedure;
			for (int i = 0; i < cnLast.numberProcedure; i++)
			{
				ProcedureNode pn = fileSpFl.Read(cur);
				pn.numberSn = numberCn;
				cur = pn.next;
			}
		}

		ClientIndexNode cinSnLast = fileSInd.Read(cnLast.numberIndex);
		cinSnLast.numberFl = numberCn;
		fileSInd.Write(&cinSnLast, cnLast.numberIndex);

		cur = sn.numberIndex;
		for (int i = 0; i < fileSInd.GetSize() - sn.numberIndex; i++)
		{
			ClientIndexNode curNext = fileSInd.Read(cur + 1);
			ClientNode tempCn = fileSFl.Read(curNext.numberFl);
			tempCn.numberIndex = tempCn.numberIndex - 1;
			fileSFl.Write(&tempCn, curNext.numberFl);
			fileSInd.Write(&curNext, cur);
			cur = cur + 1;
		}
		fileSInd.DeleteLast();

		return true;
	}

	Client* ReadClient(int numberFl)
	{
		if (numberFl < 0)
			return nullptr;
		FileConnector<ifstream, ClientNode> fileSFl(fileSFlName, ios_base::in, sizeof(ClientNode));
		return new Client{ fileSFl.Read(numberFl).client };
	}

	Procedure* ReadProcedure(int numberSpFl)
	{
		if (numberSpFl < 0)
			return nullptr;
		FileConnector<ifstream, ProcedureNode> fileSpFl(fileSpFlName, ios_base::in, sizeof(ProcedureNode));
		return new Procedure{ fileSpFl.Read(numberSpFl).procedure };
	}

	void PrintSFl()
	{
		FileConnector<ifstream, ClientNode> fileSFl(fileSFlName, ios_base::in, sizeof(ClientNode));
		for (int i = 0; i < fileSFl.GetSize(); i++)
		{
			ClientNode sn = fileSFl.Read((i + 1));
			cout << sn.firstProcedure << " " << sn.numberProcedure << " " << sn.numberIndex << " " << sn.client.surname << " " << sn.client.name << endl;
		}
	}

	void PrintSInd()
	{
		FileConnector<ifstream, ClientIndexNode> fileSFl(fileSIndName, ios_base::in, sizeof(ClientIndexNode));
		for (int i = 0; i < fileSFl.GetSize(); i++)
		{
			ClientIndexNode sn = fileSFl.Read((i + 1));
			cout << sn.KC << " " << sn.numberFl << endl;
		}
	}

	void PrintSpFl()
	{
		FileConnector<ifstream, ProcedureNode> fileSFl(fileSpFlName, ios_base::in, sizeof(ProcedureNode));
		for (int i = 0; i < fileSFl.GetSize(); i++)
		{
			ProcedureNode sn = fileSFl.Read((i + 1));
			cout << sn.previous << " " << sn.next << " " << sn.numberSn << " " << sn.procedure.km << " " << sn.procedure.price << " " << sn.procedure.cabinet << endl;
		}
	}

	void Print()
	{
		FileConnector<ifstream, ProcedureNode> fileSpFl(fileSpFlName, ios_base::in, sizeof(ProcedureNode));
		FileConnector<ifstream, ClientIndexNode> fileSInd(fileSIndName, ios_base::in, sizeof(ClientIndexNode));
		FileConnector<ifstream, ClientNode> fileSFl(fileSFlName, ios_base::in, sizeof(ClientNode));
		for (int i = 0; i < fileSInd.GetSize(); i++)
		{
			ClientIndexNode cin = fileSInd.Read(i + 1);
			ClientNode cn = fileSFl.Read(cin.numberFl);
			cout << "KC: " << cin.KC << " " << "surname: " << cn.client.surname << " " << "name: " << cn.client.name << endl;
			int cur = cn.firstProcedure;
			for (int i = 0; i < cn.numberProcedure; i++)
			{
				ProcedureNode pn = fileSpFl.Read(cur);
				cout << "   ";
				cout << "KC: " << cin.KC << " " << "KM: " << pn.procedure.km << " " << "price: " << pn.procedure.price << " " << "cabinet: " << pn.procedure.cabinet << endl;
				cur = pn.next;
			}
		}
	}

private:
	string fileSFlName = "S.fl";
	string fileSIndName = "S.ind";
	string fileSpFlName = "Sp.fl";

	int BinarySearchNumberFl(int KS)
	{
		FileConnector<ifstream, ClientIndexNode> fileSInd(fileSIndName, ios_base::in, sizeof(ClientIndexNode));
		ClientIndexNode cin;

		int left = 1, right = fileSInd.GetSize() + 1;
		while (left <= right)
		{
			int middle = (left + right) / 2;
			cin = fileSInd.Read(middle);
			if (cin.KC > KS)
				right = middle - 1;
			else if (cin.KC < KS)
				left = middle + 1;
			else
				return cin.numberFl;
		}
		return -1;
	}
};

void PrintInfo()
{
	cout << "Commands:" << endl;
	cout << "insert-m [surname] [name]" << endl;
	cout << "insert-s [KC] [KM] [price] [cabinet]" << endl;
	cout << "get-m [KC]" << endl;
	cout << "get-s [KC] [KM]" << endl;
	cout << "update-m [KC] [field] [value]" << endl;
	cout << "update-s [KC] [KM] [field] [value]" << endl;
	cout << "del-m [KC]" << endl;
	cout << "del-s [KC] [KM]" << endl;
	cout << "print" << endl ;
	cout << "exit" << endl << endl;
}

int main() {
	setlocale(LC_ALL, "Russian");
	DbConnector st(true);
	string command;
	try {
		PrintInfo();
		while (true)
		{
			cin >> command;
			if (command == "insert-m")
			{
				Client s = {};
				cin >> s.surname >> s.name;
				cout << "Added Successfully KC: " << st.InsertM(&s) << endl;
			}
			else if (command == "get-m")
			{
				int KC;
				cin >> KC;
				Client* s = st.ReadClient(st.GetM(KC));
				if (s == nullptr)
					cout << "Student don't found" << endl;
				else
					cout << s->surname << " " << s->name << endl;
				delete s;
			}
			else if (command == "insert-s")
			{
				Procedure r = {};
				int KC;
				cin >> KC >> r.km >> r.price >> r.cabinet;
				if (st.InsertS(KC, &r))
					cout << "Added" << endl;
				else
					cout << "Not Added" << endl;

			}
			else if (command == "get-s")
			{
				int KC, KM;
				cin >> KC >> KM;
				Procedure* r = st.ReadProcedure(st.GetS(KC, KM));
				if (r == nullptr)
					cout << "Rent don't found" << endl;
				else
					cout << r->km << " " << r->price << " " << r->cabinet << endl;
				delete r;
			}
			else if (command == "update-m")
			{
				int KS;
				string field;
				char value[20];
				cin >> KS >> field >> value;
				if (st.UpdateM(KS, field, value))
					cout << "Successfully updated" << endl;
				else
					cout << "Failure" << endl;
			}
			else if (command == "update-s")
			{
				int KC, KM;
				string field;
				char value[4];
				cin >> KC >> KM >> field;
				int number;
				cin >> number;
				memcpy((void*)value, (void*)&number, sizeof(int));
				if (st.UpdateS(KC, KM, field, value))
					cout << "Successfully updated" << endl;
				else
					cout << "Failure" << endl;
			}
			else if (command == "del-m")
			{
				int KC;
				cin >> KC;
				if (st.DelM(KC))
					cout << "Successfully deleted" << endl;
				else
					cout << "Failure" << endl;
			}
			else if (command == "del-s")
			{
				int KC, KM;
				cin >> KC >> KM;
				if (st.DelS(KC, KM))
					cout << "Successfully deleted" << endl;
				else
					cout << "Failure" << endl;
			}
			else if (command == "printSInd")
			{
				st.PrintSInd();
			}
			else if (command == "printSFl")
			{
				st.PrintSFl();
			}
			else if (command == "printSpFl")
			{
				st.PrintSpFl();
			}
			else if (command == "print")
			{
				st.Print();
			}
			else if (command == "exit")
			{
				break;
			}
			else
			{
				cout << "Wrong Command" << endl;
			}
		}
	}
	catch (exception& ex)
	{
		cout << ex.what();
	}

	return 0;
}