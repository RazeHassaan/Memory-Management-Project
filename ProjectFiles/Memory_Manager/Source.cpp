#include<iostream>
#include<unordered_map>
#include<queue>
#include<fstream>
#include<iomanip>
#include <cstdlib>   // rand, srand
#include<sstream>
#include<direct.h>   // for _getcwd
#include"json.hpp"

using json = nlohmann::json;   // alias

using namespace std;


 /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


struct PTableEntry
{
	int validBit;
	int DirtyBit;
	int FrameNum;	
	int pageNum;
	int processNum;
	PTableEntry(int Vbit = 0, int Dbit = 0, int FNum = -1,int pNum=-1,int pgNum=-1) :
		validBit(Vbit), DirtyBit(Dbit), FrameNum(FNum),processNum(pNum),pageNum(pgNum) {};
};


   /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


struct VirtualAddr
{
	int processNum;
	int pageNum;
	int pageOffset;
	VirtualAddr(int pNum=-1,int pgNum=-1):processNum(pNum),pageNum(pgNum){}
	void setPNum(int pNum) { processNum = pNum; }
	void setPgNum(int pgNum) { pageNum = pgNum; }
};
   /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


struct PAGE
{ 
	static const int pageSize=10;
	int data[pageSize];
	int processNum; int pageNum;
	
	PAGE() :processNum(-1), pageNum(-1) {};

	void UpdateValueAtOffset(int newVal, int offset)
	{
		data[offset] = newVal; 
	}

	int AccessValueAtOffset(int offset)
	{
		return data[offset];
	}

	void IndexPage(int pNum,int pgNum)
	{
		processNum = pNum; pageNum = pgNum;
	}

	void PopulatePage()
	{
		for(int i=0;i<pageSize;i++)
		{ data[i] = (rand()%100)+1;}
	}

	void PrintPage()
	{
		for(int i=0;i<pageSize;i++)
		{ cout << data[i] << ",";}
		cout << endl;
	}
};


    ///////////////////////////////////////////////////////////////////////       PROCESS      ////////////////////////////////////////////////////////////////////
   //////////////////////////////////////////////////////////////////////         PROCESS     //////////////////////////////////////////////////////////////////////////

class PROCESS
{
	int totalPages;
	static const int pageSize = 10;
	PAGE* pages;
	int processSize;
	int processNum;
public:
	//Constructor and Pages Builder
	PROCESS(int pSize=0,int index=-1):processSize(pSize),processNum(index)
	{ 
	  totalPages = pSize / pageSize;
	  pages = new PAGE[totalPages];
	  int j = 0;

	  //Initialize its pages with indexes and data as continuous Counting
	  for (int i = 0; i < totalPages; i++)
	  {
		  pages[i].PopulatePage();
		  //cout << j << endl;
		  pages[i].IndexPage(processNum, i);
	  }
	}

	//To get the index of the process
	int getIndex() { return processNum; }

	//to Access a certain Page
	PAGE AccessPage(int pageNum)
	{
		PAGE pg;
		pg = pages[pageNum];
		return pg;
	}

	//Returns total pages in the process
	int totalPgNum() { return totalPages; }


	//To update a page
	void UpdatePage(int pgNum, PAGE& pg)
	{
		pages[pgNum] = pg;
	}


	void PrintPages()
	{
		for(int i=0;i<totalPages;i++)
		{
			pages[i].PrintPage();
		}
	}
};


 ///////////////////////////////////////////////////////////////////////       PAGE TABLE      ////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////       PAGE TABLE      ////////////////////////////////////////////////////////////////////////


class PageTable
{
	PROCESS &p;
	PTableEntry* PTable;
	int entries;
public:
	PageTable(PROCESS &prcs):p(prcs)
	{
		entries = p.totalPgNum();
		PTable = new PTableEntry[entries];
		for(int i=0;i<entries;i++)
		{
			PTable[i].processNum = prcs.getIndex();
			PTable[i].pageNum = i;
		}
	}

	void ValidatePage(int pageNum,int Vbit)
	{
		PTable[pageNum].validBit = Vbit;
	}

	void UpdateFrame(int PageNum,int frameNum)
	{
		PTable[PageNum].FrameNum=frameNum;
	}

	//to access a page to mark it dirty
	void MarkDirty(int pageNum)
	{
		PTable[pageNum].DirtyBit = 1;
	}

	PTableEntry AccessEntry(int PageNum)
	{
		return PTable[PageNum];
	}

	int getSize() { return entries; }
};


// 1. Forward Declarations
class PhysicalMemory;

    //////////////////////////////////////////////////////////////////////////       TLB CACHE       ////////////////////////////////////////////////////////////////////
   //////////////////////////////////////////////////////////////////////////       TLB CACHE      ////////////////////////////////////////////////////////////////////


class TLB
{
	PTableEntry* quickTable;
	int TLBSize;
	int nextIndex = 0;   //Index to the next free entry
	queue<VirtualAddr> VPN_Track;
	VirtualAddr newVPN;

public:
	TLB(int size) :TLBSize(size)
	{
		quickTable = new PTableEntry[TLBSize];
	}


	//Check whether TLB is empty
	bool Is_TLBEmpty() { return nextIndex < TLBSize; }

	int AccessPage(int pNum, int pgNum)
	{
		int FrameNumber = -1;
		for (int i = 0; i < TLBSize; i++)
		{
			if (quickTable[i].processNum == pNum)
			{
				if (quickTable[i].pageNum == pgNum)
				{
					FrameNumber = quickTable[i].FrameNum;
				}
			}
		}
		return FrameNumber;
	}


	//Updates TLB
	//Only called by RAM and given desired parameters when TLB does not have req. page frame numbers
	void UpdateTLB(int pNum, int pgNum, int FrmNum)
	{
		PTableEntry newEntry{ 1,0,FrmNum,pNum,pgNum };
		PTableEntry target;
		if (Is_TLBEmpty())  //Means TLB has some space
		{
			AddEntrytoTLB(newEntry);  //Reuse code Enhancement
		}
		else                //Means TLB has no space
		{
			//---------------->FIFO REPLACEMENT ALGORITHM<---------------------------------
			newVPN = VPN_Track.front();
			VPN_Track.pop();
			//Now we'll find that PTableEntry and replace it with new PTableEntry
			for (int i = 0; i < TLBSize; i++)
			{
				target = quickTable[i];
				if ((target.processNum == newVPN.processNum) && (target.pageNum == newVPN.pageNum))
				{
					quickTable[i] = newEntry;
				}
			}

			// record new mapping  //This was missing
			VirtualAddr v; v.setPNum(pNum); v.setPgNum(pgNum);
			VPN_Track.push(v);
		}
	}

	void AddEntrytoTLB(PTableEntry entry)
	{
		if (nextIndex >= TLBSize) return; // defensive
		quickTable[nextIndex] = entry;
		// Keep VPN_Track in sync:
		VirtualAddr v;
		v.setPNum(entry.processNum);
		v.setPgNum(entry.pageNum);
		VPN_Track.push(v);
		nextIndex++;
	}

};
 ///////////////////////////////////////////////////////////////////////       HARD DISK ROM      ////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////       HARD DISK ROM      ///////////////////////////////////////////////////////////////////////


class DISK
{
	int dirtybit, validityBit;
	int totalReads; int totalWrites;
	int latency;
	vector<int> pIndexes;
	PROCESS p1{40,1};
	PROCESS p2{80,2};
	PROCESS p3{120,3};
	PROCESS p4{100,4};
	PageTable PT1{ p1 };
	PageTable PT2{ p2 };
	PageTable PT3{ p3 };
	PageTable PT4{ p4 };
	vector<PageTable> Tables;
	unordered_map<string,PROCESS> BackingStore;
public:

	DISK()
	{
		BackingStore["Process1"] = {p1};
		BackingStore["Process2"] = {p2};
		BackingStore["Process3"] = {p3};
		BackingStore["Process4"] = {p4};
		Tables.push_back(PT1);
		Tables.push_back(PT2);
		Tables.push_back(PT3);
		Tables.push_back(PT4);
		pIndexes.push_back(1);
		pIndexes.push_back(2);
		pIndexes.push_back(3);
		pIndexes.push_back(4);
	}

	PAGE ReadPage(int ProcessNum,int pgNum)
	{
		PAGE pg;
		switch (ProcessNum)
		{
		 case 1:
		 { pg = BackingStore["Process1"].AccessPage(pgNum); break; }
		 case 2:
		 { pg = BackingStore["Process2"].AccessPage(pgNum); break; }
		 case 3:
		 { pg = BackingStore["Process3"].AccessPage(pgNum); break; }
		 case 4:
		 { pg = BackingStore["Process4"].AccessPage(pgNum); break; }
		 default:
		 { cout << "Process not exists; Process Number was "<<ProcessNum<<"\n"; }
	    }
		return pg;
	}

	PROCESS accessProcess(int ProcessNum)
	{
		PROCESS p;
		switch (ProcessNum)
		{
		case 1:
		{ p = BackingStore["Process1"]; break; }
		case 2:
		{ p = BackingStore["Process2"]; break; }
		case 3:
		{ p = BackingStore["Process3"]; break; }
		case 4:
		{ p = BackingStore["Process4"]; break; }
		default:
		{ cout << "Process not exists\n"; break; }
		}
		return p;
	}

	PageTable accessTable(int tableNum)
	{
		
		switch (tableNum)
		{
		case 1:
		{ PageTable pt = Tables[0]; return pt; }
		case 2:
		{ PageTable pt = Tables[1]; return pt; }
		case 3:
		{ PageTable pt = Tables[2]; return pt; }
		case 4:
		{ PageTable pt = Tables[3]; return pt; }
		default:
		{ cout << "Required Table not exists\n"; }
		}
	}

	void WriteBack(int pNum,int pgNum, PAGE& pg)
	{
		//if a page is written back to Disk
		switch (pNum)
		{
		case 1:
		{ BackingStore["Process1"].UpdatePage(pgNum, pg); break; }
		case 2:
		{ BackingStore["Process2"].UpdatePage(pgNum, pg); break; }
		case 3:
		{ BackingStore["Process3"].UpdatePage(pgNum, pg); break; }
		case 4:
		{ BackingStore["Process4s"].UpdatePage(pgNum, pg); break; }
		default:
		{ cout << "Required Process not exists in Disk\n"; break; }
		}
	}

	vector<int> getPIndexes() { return pIndexes; }
};

    ///////////////////////////////////////////////////////////////////////       MEMORY RAM       ////////////////////////////////////////////////////////////////////
   ///////////////////////////////////////////////////////////////////////       MEMORY RAM       ////////////////////////////////////////////////////////////////////

class PhysicalMemory
{
	vector<PageTable> Tables;
	PAGE* frames;
	int size;
	DISK& store;
	int freeFrames;
	int nextFreeFrame;   //Tells the next empty slot in RAM frames
	TLB& fastMem;
	queue<VirtualAddr> FIFO_queue;   //queue to be used in FIFO Replacement algorithm
	VirtualAddr newVPN;
	vector<int> pIndexes;

public:
	//Constructor
	PhysicalMemory(int space,DISK &d, TLB& tlb) :size(space),store(d),nextFreeFrame(0),freeFrames(space),fastMem(tlb)
	{
		frames = new PAGE[size];
		InitializeTables();
		InitializePages();
		pIndexes = store.getPIndexes();
		for (int index : pIndexes)
		{
			InitializeTLB(index);
		}
	}

	void InitializeTables()
	{
		for (int i = 1; i < 5; i++)
		{ Tables.push_back(store.accessTable(i)); }
	}

	//Tells is there any free space in RAM
	bool  Is_RAMEmpty() { return freeFrames!=0; }

	//Initializes RAM with some pages and PageTables
	void InitializePages()
	{ 
		PAGE pg;
		for(int ProcessNum=1;ProcessNum<5;ProcessNum++)
		{
			for (int pgNum = 0; pgNum < 5; pgNum++)
			{
				pg = store.ReadPage(ProcessNum, pgNum);
				frames[nextFreeFrame] = pg; 
				newVPN.setPNum(ProcessNum);
				newVPN.setPgNum(pgNum);
				FIFO_queue.push(newVPN);
				freeFrames--;
				Tables[ProcessNum-1].ValidatePage(pgNum,1);
				Tables[ProcessNum-1].UpdateFrame(pgNum, nextFreeFrame);
				nextFreeFrame++;
			}
		}
	}
	

	//Loads Pages to TLB Initially
	void InitializeTLB(int ProcessNum)
	{
	  int SendCount = 0;
	  switch(ProcessNum)
	  {
	    case 1:
	    {
			PageTable pt = Tables[0];
			for (int i=0;i<pt.getSize();i++)
			{
				if(pt.AccessEntry(i).validBit==1)
				{
					SendCount++;
					fastMem.AddEntrytoTLB(pt.AccessEntry(i));
				}
				if (SendCount == 2) { break; }
			}
	    }
		case 2:
		{
			PageTable pt = Tables[1];
			for (int i = 0; i < pt.getSize(); i++)
			{
				if (pt.AccessEntry(i).validBit == 1)
				{
					SendCount++;
					fastMem.AddEntrytoTLB(pt.AccessEntry(i));
				}
				if (SendCount == 2) { break; }
			}
		}
		case 3:
		{
			PageTable pt = Tables[2];
			for (int i = 0; i < pt.getSize(); i++)
			{
				if (pt.AccessEntry(i).validBit == 1)
				{
					SendCount++;
					fastMem.AddEntrytoTLB(pt.AccessEntry(i));
				}
				if (SendCount == 2) { break; }
			}
		}
		case 4:
		{
			PageTable pt = Tables[3];
			for (int i = 0; i < pt.getSize(); i++)
			{
				if (pt.AccessEntry(i).validBit == 1)
				{
					SendCount++;
					fastMem.AddEntrytoTLB(pt.AccessEntry(i));
				}
				if (SendCount == 2) { break; }
			}
		}
	  }
	}
	
	//If Desired Page is not in TLB for reading, it should be checked here:
	int AccessPTablePG(int pNum, int pgNum)
	{
		int reqFrmNum;
		int requiredFrame;
		PTableEntry ptEntry;
		PAGE requiredPage;     //Page to be inserted in RAM
		PAGE target;           //Page to be removed from RAM by replacement algo
		PAGE previousPage;     //Page previously at that frame before replacement


		//Access the required process's Page Table's required page entry
		ptEntry=Tables[pNum - 1].AccessEntry(pgNum);
		if (ptEntry.validBit==0)  //If page Table does not have frame number => Page is missing in RAM
		{
			//PAGE FAULT
			//Access Disk and bring the Page to RAM
			requiredPage=store.ReadPage(pNum, pgNum);
			if (Is_RAMEmpty() == true)  //If there is free space in RAM
			{
				frames[nextFreeFrame] = requiredPage;
				newVPN.setPNum(pNum);
				newVPN.setPgNum(pgNum);
				FIFO_queue.push(newVPN);
				//Update its value in page table and mark its VALID BIT=1;
				Tables[pNum-1].UpdateFrame(pgNum, nextFreeFrame);
				Tables[pNum-1].ValidatePage(pgNum, 1);
				nextFreeFrame++; freeFrames--;
			}
			else     //Means RAM is full
			{
				//---------------->FIFO REPLACEMENT ALGORITHM<---------------------------------
				newVPN = FIFO_queue.front();
				FIFO_queue.pop();
				//since all RAM is full, so we'll traverse all RAM (i<size) to find target page
				for (int i = 0; i < size; i++)
				{
					target = frames[i];
					if ((target.processNum == newVPN.processNum) && (target.pageNum == newVPN.pageNum))
					{
						previousPage = frames[i];
						frames[i] = requiredPage;
						//update Page Table
						Tables[pNum-1].UpdateFrame(pgNum,i);
						//Write back to DISK the page to be replaced if it is dirty
						if (Tables[pNum-1].AccessEntry(pgNum).DirtyBit == 1)
						{
							//Write back to DISK
							store.WriteBack(newVPN.processNum,newVPN.pageNum,previousPage);
						}
					}
				}
			}
			reqFrmNum = Tables[pNum - 1].AccessEntry(pgNum).FrameNum;
		}
		else              //Page Table has the frame Number of desired Page => Page is in RAM
		{
			//Update TLB 
			fastMem.UpdateTLB(pNum, pgNum, ptEntry.FrameNum);
			//Return the required Frame Number from Page Table
			reqFrmNum = ptEntry.FrameNum;
		}
		return reqFrmNum;
	}

	//Directly Accessing the RAM frames for READ when you have frame Number/Index
	PAGE DirectAccessFrameForRead(int frmNum)
	{
		if (frmNum < 0 || frmNum >= size)
		{
			cout << "INVALID FRAME DETECTED: " << frmNum<<endl;
			exit(1);
		}
		return frames[frmNum];
	}

	//Directly Accessing the RAM frames for WRITE when you have frame Number/Index
	void DirectAccessFrameForWrite(int frmNum,int pNum,int pgNum, int newVal,int pgOffset)
	{
		//Updates the required part of specified Page 
		frames[frmNum].UpdateValueAtOffset(newVal,pgOffset);
		//Now mark it dirty in page table
		Tables[pNum-1].MarkDirty(pgNum);
	}
};


     ///////////////////////////////////////////////////////////////////////       MEMORY_HANDLER       ////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////       MEMORY_HANDLER       ////////////////////////////////////////////////////////////////////


class MEMORY_HANDLER
{
	int processNum;
	int pageNum;
	TLB& cache;
	PhysicalMemory& RAM;
public:
    //Constructor:
	MEMORY_HANDLER(TLB& tlb,int pgNum,int pNum, PhysicalMemory& memory)
		:cache(tlb),processNum(pNum),pageNum(pgNum),RAM(memory){}

	//We want to access the required page to read its specified part
	//Here ,we are just accessing the required Page
	PAGE ReturnPage()
	{
		 PAGE desiredPage;
		 int frame;

		 //First Access TLB
		 frame=cache.AccessPage(processNum, pageNum);
		 if (frame == -1)    //If TLB does not have desired Page
		 {		
			//Access Page Table then RAM
			//Get the desired Frame Number from Page Table
			frame= RAM.AccessPTablePG(processNum, pageNum);
			//Now access the RAM to get the PAGE for read
			desiredPage = RAM.DirectAccessFrameForRead(frame);
		 }
		 else                //If TLB has the desired Page
		 {
			 //directly access RAM because you have the frame Number of Page
			 desiredPage= RAM.DirectAccessFrameForRead(frame);
		 }
		 return desiredPage;
	}

	//We want to access the requiredPage to write a thing to its specific part
	void WriteOnPage(int newData, int pgOffset)
	{
		int frame;

		//First Access TLB
		frame = cache.AccessPage(processNum, pageNum);
		if (frame == -1)    //If TLB does not have desired Page
		{
			//Access Page Table then RAM
			//Get the desired Frame Number from Page Table
			frame = RAM.AccessPTablePG(processNum, pageNum);
			//Now access the RAM to access the page for WRITE
			RAM.DirectAccessFrameForWrite(frame, processNum,pageNum, newData, pgOffset);  //Updated the page
		}
		else                //If TLB has the desired Page
		{
			//directly access RAM because you have the frame Number of Page
			RAM.DirectAccessFrameForWrite(frame, processNum, pageNum, newData, pgOffset);  //Updated the page
		}
	}
};


    ///////////////////////////////////////////////////////////////////////       CONFIGURATION MANAGER       ////////////////////////////////////////////////////////////////////
   ///////////////////////////////////////////////////////////////////////       CONFIGURATION MANAGER       ////////////////////////////////////////////////////////////////////


class ConfigurationManager
{
	json j;
	int RAM_Size;
	int Page_Size;
	int TLB_Size;
	int Latency_TLB;
	int Latency_RAM;
	int Latency_Disk;

public:

	//CONSTRUCTOR
	//Constructs the Json Map according to Json file given
	ConfigurationManager(ifstream& Jsonfile)
	{
		if (!Jsonfile.is_open())
		{
			cout << "ERROR: Could'nt open your Json File\n";
		}
		try {
			Jsonfile >> j;   //Parse the Json file and builds a Hash Map according to Json file
		}
		catch (const json::parse_error& ex)
		{
			cout << ex.what();
		}
		AssignValues();
	}

	//Assigns and sets the parameters according to Json Map
	void AssignValues()
	{
		RAM_Size = j["RAM_Size"]["value"].get<int>();
		Page_Size = j["Page_Size"]["value"].get<int>();
		TLB_Size = j["TLB_Size"]["value"].get<int>();
		Latency_TLB = j["Latency"]["TLB"]["value"].get<int>();
		Latency_RAM = j["Latency"]["RAM"]["value"].get<int>();
		Latency_Disk = j["Latency"]["Disk"]["value"].get<int>();
	}

	//Display the assigned values
	void PrintConfig() const {
		std::cout << "RAM Size: " << RAM_Size << " KB\n";
		std::cout << "Page Size: " << Page_Size << " KB\n";
		std::cout << "TLB Size: " << TLB_Size << "\n";
		std::cout << "Latency (TLB): " << Latency_TLB << " ns\n";
		std::cout << "Latency (RAM): " << Latency_RAM << " ns\n";
		std::cout << "Latency (Disk): " << Latency_Disk << " ns\n";
	}

	//Getters
	int getRAM_Size() { return RAM_Size; }
	int getTLB_Size() { return TLB_Size; }
	
};


    ///////////////////////////////////////////////////////////////////////       ADDRESS GENERATOR        ////////////////////////////////////////////////////////////////////
   ////////////////////////////////////////////////////////////////////////       ADDRESS GENERATOR       ////////////////////////////////////////////////////////////////////


class AddressGenerator {
public:
	void createAddressFile(int count) {
		ofstream outFile("trace.txt");

		for (int i = 0; i < count; i++) {
			// Generating a random address between 0 and 500
			unsigned int addr = rand() % 500;
			// Randomly pick 'R' (Read) or 'W' (Write)
			char op = (rand() % 2 == 0) ? 'R' : 'W';

			// Write to file in Hex format: e.g., 0x000000FF R
			outFile << "0x" << std::hex << std::setw(8) << std::setfill('0')
				<< addr << " " << op << std::endl;
		}
		outFile.close();
		cout << "File 'trace.txt' created with " << count << " addresses." << std::endl;
	}
};


    ///////////////////////////////////////////////////////////////////////       ADDRESS PARSER        ////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////       ADDRESS PARSER        ////////////////////////////////////////////////////////////////////


class AddressParser {
private:
	vector<VirtualAddr> addressList; // Vector to store the results

public:
	// This function reads the file and fills the vector
	vector<VirtualAddr> parseHexFile(std::string filename) {
		addressList.clear(); // Clear previous data

		ifstream inFile(filename);
		if (!inFile.is_open()) {
			cerr << "Error: Could not open file " << filename << std::endl;
			return addressList;
		}

		std::string hexStr;
		char op; // Operation type (R/W) read from file

		while (inFile >> hexStr >> op) {
			// 1. Convert hex string to integer
			unsigned int addrValue;
			stringstream ss;
			ss << std::hex << hexStr;
			ss >> addrValue;

			// 2. Extract VPN and Offset using Page Size = 10
			int vpn = addrValue / 10;
			int offset = addrValue % 10;

			// 3. Create a VirtualAddr object and push it to the vector
			// (Note: Since the trace file doesn't store processNum, 
			// you might set it to a default or handle it separately)
			VirtualAddr tempAddr;
			int random = (rand() % 5);
			if(random==0 || random>4)
			{
				tempAddr.setPNum(1);
			}
			else
			{
				// Defaulting to process 0 for now
				tempAddr.setPNum(random);
			}     
			cout<<"Process Num Parsed is::"<<tempAddr.processNum<<tempAddr.pageNum<<endl;
			tempAddr.setPgNum(vpn);    // Setting pageNum from VPN
			tempAddr.pageOffset = offset;

			addressList.push_back(tempAddr);
		}

		inFile.close();
		return addressList; // Return the completed vector
	}
};


    ///////////////////////////////////////////////////////////////////////       MAIN SIMULATOR        ////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////       MAIN SIMULATOR        ////////////////////////////////////////////////////////////////////


int main()
{
	//VECTOR OF ADDRESSES INITIALIZED

	vector<VirtualAddr> myAddresses;
	int totalAddr = myAddresses.size();
	AddressGenerator gen;
	AddressParser parser;
	// 1. Generate 14 addresses
	gen.createAddressFile(14);
	cout << "-----------------------------------" << std::endl;
	// 2. Read and Parse them
	myAddresses = parser.parseHexFile("addresses.txt");

	cout << endl;
	


	//SIZES OF TLB,RAM INITIALIZED FROM FILE

	char buffer[256];
	if (_getcwd(buffer, sizeof(buffer)) != nullptr) {
		cout << "Working directory: " << buffer << endl;
	}
	else {
		cout << "Error getting working directory" << endl;
	}
	ifstream JsonFile("config.JSON");
	if (JsonFile.peek() == EOF) {
		std::cout << "File is empty or not found!\n";
	}
	ConfigurationManager Configurator(JsonFile);
	int RAM_Size = Configurator.getRAM_Size();
	int TLB_Size = Configurator.getTLB_Size();



	//INITIAL DECLARATIONS

	DISK ROM;
	TLB cache(TLB_Size);
	PhysicalMemory RAM(RAM_Size,ROM,cache);
	MEMORY_HANDLER* MMU_Unit=NULL;


	// DYNAMIC WORKING

	int pNum, pgNum, pgOffset, newVal;
	PAGE wanted;
	srand(42);   //Fixed seed for random numbers
	for (VirtualAddr Addr : myAddresses)
	{
		newVal = rand() % 100;
		pNum = Addr.processNum;
		pgNum = Addr.pageNum;
		pgOffset = Addr.pageOffset;
		MMU_Unit = new MEMORY_HANDLER(cache, pgNum, pNum, RAM);
		//First access the page for read
		wanted = MMU_Unit->ReturnPage();
		cout<<"Process Number:"<<pNum<<" , Page Number:"<<pgNum<<" , Page Offset:"<<pgOffset<<" , Value Before::"
			<<wanted.AccessValueAtOffset(pgOffset) << endl;
		//Now write on the page
		wanted.UpdateValueAtOffset(newVal, pgOffset);
		//Display the changed result
		cout << "Process Number:" << pNum << " , Page Number:" << pgNum << " , Page Offset:" << pgOffset << " , Value After::"
			<< wanted.AccessValueAtOffset(pgOffset) << endl;
		 delete MMU_Unit; 
		 MMU_Unit = NULL;
		 cout << endl;
	}


	FINAL RESULT SHOWING
	for (int i = 1; i < 5; i++)
	{
		ROM.accessProcess(i).PrintPages();
	}
	
	return 0;
}
