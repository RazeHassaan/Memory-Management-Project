#include<iostream>
#include<unordered_map>
using namespace std;

 /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct PAGE
{ 
	static const int pageSize=10;
	int data[pageSize];
	int DirtyBit; int ValidBit;
	
	PAGE() :DirtyBit(0),ValidBit(0) {};

	void UpdateValueAtOffset(int newVal, int offset)
	{
		data[offset] = newVal; DirtyBit = 1;
	}

	int PopulatePage(int start)
	{
		int begin = start;
		for(int i=0;i<pageSize;i++,begin++)
		{ data[i] = begin;}
		return begin ;
	}

	void PrintPage()
	{
		for(int i=0;i<pageSize;i++)
		{ cout << data[i] << ",";}
		cout << endl;
	}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class PROCESS
{
	int totalPages;
	static const int pageSize = 10;
	PAGE* pages;
	int processSize;
	int processNum;
public:
	//Constructor and Pages Builder
	PROCESS(int pSize=0,int index):processSize(pSize),processNum(index)
	{ 
	  totalPages = pSize / pageSize;
	  pages = new PAGE[totalPages];
	  int j = 0;

	  //Initialize its pages with continuous Counting
	  for (int i = 0; i < totalPages; i++)
	  {
		  j=pages[i].PopulatePage(j);
		  //cout << j << endl;
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

	int totalPgNum() { return totalPages;}

	void PrintPages()
	{
		for(int i=0;i<totalPages;i++)
		{
			pages[i].PrintPage();
		}
	}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

	PTableEntry AccessEntry(int PageNum)
	{
		return PTable[PageNum];
	}

	int getSize() { return entries; }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class DISK
{
	int dirtybit, validityBit;
	int totalReads; int totalWrites;
	int latency;
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
	}

	PAGE ReadPage(int ProcessNum,int pgNum)
	{
		PAGE pg;
		switch (ProcessNum)
		{
		 case 1:
		 { pg= BackingStore["Process1"].AccessPage(pgNum);}
		 case 2:
		 { pg = BackingStore["Process2"].AccessPage(pgNum); }
		 case 3:
		 { pg = BackingStore["Process3"].AccessPage(pgNum); }
		 case 4:
		 { pg = BackingStore["Process4"].AccessPage(pgNum); }
		 default:
		 { cout << "Process not exists\n"; }
	    }
		return pg;
	}

	PageTable accessTable(int tableNum)
	{
		
		switch (tableNum)
		{
		case 1:
		{ PageTable pt = Tables[0]; }
		case 2:
		{ PageTable pt = Tables[1]; }
		case 3:
		{ PageTable pt = Tables[2]; }
		case 4:
		{ PageTable pt = Tables[3]; }
		default:
		{ cout << "Required Table not exists\n"; }
		}
	}

	void WriteBack()
	{
		//if a page is written back to Disk
	}

	void MakeDirty()
	{
		//To make the desired pg
	}


};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class PhysicalMemory
{
	vector<PageTable> Tables;
	PAGE* frames;
	int size;
	DISK& store;
	int freeFrames;
	int nextFreeFrame;   //Tells the next empty slot in RAM frames
	TLB& fastMem;

public:
	//Constructor
	PhysicalMemory(int space,DISK &d, TLB& tlb) :size(space),store(d),nextFreeFrame(0),freeFrames(space),fastMem(tlb)
	{
		frames = new PAGE[size];
		InitializeTables();
		InitializePages();
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
				frames[nextFreeFrame] = pg; freeFrames--;
				Tables[ProcessNum].ValidatePage(pgNum,1);
				Tables[ProcessNum].UpdateFrame(pgNum, nextFreeFrame);
				nextFreeFrame++;
			}
		}
	}
	

	//Loads Pages to TLB Initially
	vector<PTableEntry> InitializeTLB(int ProcessNum)
	{
	  vector<PTableEntry> toTLB;
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
					toTLB.push_back(pt.AccessEntry(i));
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
					toTLB.push_back(pt.AccessEntry(i));
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
					toTLB.push_back(pt.AccessEntry(i));
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
					toTLB.push_back(pt.AccessEntry(i));
				}
				if (SendCount == 2) { break; }
			}
		}
	  }
	  return toTLB;
	}
	
	//If Desired Page is not in TLB for reading, it should be checked here:
	int AccessPTablePG(int pNum, int pgNum)
	{
		int reqFrmNum;
		int requiredFrame;
		PTableEntry ptEntry;
		PAGE requiredPage;
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
				//Update its value in page table and mark its VALID BIT=1;
				Tables[pNum].UpdateFrame(pgNum, nextFreeFrame);
				Tables[pNum].ValidatePage(pgNum, 1);
				nextFreeFrame++; freeFrames--;
			}
			else     //Means RAM is full
			{
				//---------------->APPLY REPLACEMENT ALGORITHM<---------------------------------
			}
		}
		else              //Page Table has the frame Number of desired Page => Page is in RAM
		{
			//Update TLB 
			fastMem.UpdateTLB(pNum, pgNum, ptEntry.FrameNum);
			//Return the required Frame Number from Page Table
			reqFrmNum = ptEntry.FrameNum;
			return reqFrmNum;
		}
	}

	//Directly Accessing the RAM frames for READ when you have frame Number/Index
	PAGE DirectAccessFrameForRead(int frmNum)
	{
		return frames[frmNum];
	}

	//Directly Accessing the RAM frames for WRITE when you have frame Number/Index
	void DirectAccessFrameForWrite(int frmNum, int newVal,int pgOffset)
	{
		//Updates the required part of specified Page 
		frames[frmNum].UpdateValueAtOffset(newVal,pgOffset);
	}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class TLB
{
	PTableEntry* quickTable;
	int TLBSize;
	PhysicalMemory& RAM;
	int nextIndex=0;   //Index to the next free entry

public:
	TLB(int size, PhysicalMemory& mem) :TLBSize(size),RAM(mem)
	{ quickTable = new PTableEntry[TLBSize];}

	//Initialize TLB with Random Pages
	void Initialize()
	{
		vector<PTableEntry> fromRAM;
		for (int i = 1; i < 5; i++)
		{
			fromRAM=RAM.InitializeTLB(i);
			for(PTableEntry pt:fromRAM)
			{
				quickTable[nextIndex] = pt;
				nextIndex++;
			}
	    }
	}
	
	//Check whether TLB is empty
	bool Is_TLBEmpty() { return nextIndex != TLBSize; }

	int AccessPage(int pNum, int pgNum)
	{
		int FrameNumber=-1;
		for (int i = 0; i < TLBSize; i++)
		{
			if (quickTable[i].processNum == pNum)
			{
				if(quickTable[i].pageNum==pgNum)
				{ FrameNumber = quickTable[i].FrameNum; }
			}
		}
		return FrameNumber;
	}
	
	//Updates TLB
	//Only called by RAM and given desired parameters when TLB does not have req. page frame numbers
	void UpdateTLB(int pNum,int pgNum,int FrmNum)
	{
		PTableEntry newEntry{1,0,FrmNum,pNum,pgNum};
		if (Is_TLBEmpty() == true)   //Means TLB has some empty space
		{
			quickTable[nextIndex] = newEntry;
	    }
		else                        //Means TLB is full
		{
			     //---------------->APPLY REPLACEMENT ALGORITHM<---------------------------------
		}
	}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class MEMORY_HANDLER
{
	int VirtualAddress;
	int processNum;
	int pageNum;
	TLB& cache;
	PhysicalMemory& RAM;
public:
    //Constructor:
	MEMORY_HANDLER(int VAddr,TLB& tlb,int pgNum,int pNum, PhysicalMemory& memory)
		:VirtualAddress(VAddr),cache(tlb),processNum(pNum),pageNum(pgNum),RAM(memory){}

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
			RAM.DirectAccessFrameForWrite(frame, newData, pgOffset);  //Updated the page
		}
		else                //If TLB has the desired Page
		{
			//directly access RAM because you have the frame Number of Page
			RAM.DirectAccessFrameForWrite(frame, newData, pgOffset);  //Updated the page
		}
	}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main()
{
	PROCESS p1(4);
	p1.PrintPages();
	return 0;
}
