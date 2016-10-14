#include <iostream>
#include <vector>
#include <set>
#include <tuple>
#include <string>
#include <ctime>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <exception>
#include "any.h"
#include "CSVReader.h"
#include "CSVWriter.h"
#include "hash.h"
#include "KMeans.h"

using namespace any_type;
using namespace std;


string itoa(int num)
{
	string str = "";
	while (num)
	{
		str = str + static_cast<char>(num % 10 + '0');
		num /= 10;
	}
	return str;
}

template <typename out, typename in>
out atob(const in& I)
{
	static stringstream ss;
	out O;
	ss.str("");
	ss.clear();
	ss << I;
	ss >> O;
	return O;
}

enum Label_CSV{
	NO,
	DELTA_TIME,
	ABS_TIME,
	EPOCH_TIME,
	HWSRC,
	HWDEST,
	PROTO,
	RSSI,
	SSID,
	SUBTYPE,
	SRCIP,
	DESTIP,
	AGENT
};

struct Content{
	string device;
	string comeTime;
	string goTime;
	set<string> sProbeRqSSID; //SSID of probe request
	vector<double> vProbeRqFlag;
	set<string> sSelfIP;
	set<string> sOtherIP;
	set<string> sAgent;
	vector<int> vRSSI;
	vector<string> vRSSITime;
	int group;
	int rssiCounter;
	int tcpCounter;
	int udpCounter;
	int httpCounter;
	double sumRSSI;
	int maxRSSI;
	int minRSSI;
};




class distFunction{
public:
	distFunction(){}
	double operator()(Content *a, Content *b){
		double sum = 0;
		for (int i = 0; i < a->vProbeRqFlag.size(); ++i)
		{
			sum += abs(a->vProbeRqFlag[i] - b->vProbeRqFlag[i]);
		}
		return sum;
	}
	/*double operator()(Content *a, Content *b){
		double sumA = 0, sumB = 0;
		for (int i = 0; i < a->vProbeRqFlag.size(); ++i)
		{
			sumA += a->vProbeRqFlag[i];
			sumB += b->vProbeRqFlag[i];
		}
		return abs(sumA-sumB);
	}*/
};

class distFuncTS{
public:
	distFuncTS(){}
	double operator()(Content *a, Content *b){
		double sumA = 0, sumB = 0;
		for (int i = 0; i < a->vProbeRqFlag.size(); ++i)
		{
			sumA += a->vProbeRqFlag[i];
			sumB += b->vProbeRqFlag[i];
		}
		sumA = abs(sumA - sumB) / a->vProbeRqFlag.size(); //normalize
		sumA += abs(atoi(a->comeTime.c_str()) - atoi(b->comeTime.c_str())) / 2 / 60 / 60; //comeTime: 2 hours a unit 
		sumA += abs(atoi(a->goTime.c_str()) - atoi(a->comeTime.c_str()) - atoi(b->goTime.c_str()) + atoi(b->comeTime.c_str())) / 1 / 60 / 60 / 2; //leaveTime: 1 hours a unit, *0.5

		return sumA;
	}
};

class findCentroidFunction{
public:
	findCentroidFunction(){}
	void operator()(Content *point, int len, Content *centroid, int n_cluster)
	{
		/* group element for centroids are used as counters */
		for (int i = 0; i < n_cluster; ++i){
			centroid[i].vProbeRqFlag.clear();
			centroid[i].vProbeRqFlag.insert(centroid[i].vProbeRqFlag.begin(), point[0].vProbeRqFlag.size(), 0.0);
			centroid[i].group = 0;
		}
		for (int i = 0; i < len; ++i)
		{
			for (int j = 0; j < point[i].vProbeRqFlag.size(); ++j)
			{
				centroid[point[i].group].vProbeRqFlag[j] += point[i].vProbeRqFlag[j];
			}
			++centroid[point[i].group].group;
		}
		for (int i = 0; i < n_cluster; ++i)
		{
			//cout << "centroid["<< i << "].group" << centroid[i].group << endl;
			for (int j = 0; j < centroid[i].vProbeRqFlag.size(); ++j)
			{
				centroid[i].vProbeRqFlag[j] /= centroid[i].group;
			}
		}
	}
};

class findCentFuncTS{
public:
	findCentFuncTS(){}
	void operator()(Content *point, int len, Content *centroid, int n_cluster)
	{
		/*init array of comeTime goTime*/
		double *comeTimeArr = new double[n_cluster], *goTimeArr = new double[n_cluster];
		for (int i = 0; i < n_cluster; ++i)
		{
			comeTimeArr[i] = 0;
			goTimeArr[i] = 0;
		}
		/* group element for centroids are used as counters */
		for (int i = 0; i < n_cluster; ++i){
			centroid[i].vProbeRqFlag.clear();
			centroid[i].vProbeRqFlag.insert(centroid[i].vProbeRqFlag.begin(), point[0].vProbeRqFlag.size(), 0.0);
			centroid[i].group = 0;
		}
		for (int i = 0; i < len; ++i)
		{
			for (int j = 0; j < point[i].vProbeRqFlag.size(); ++j)
			{
				centroid[point[i].group].vProbeRqFlag[j] += point[i].vProbeRqFlag[j];
			}
			++centroid[point[i].group].group;
		}
		for (int i = 0; i < len; ++i)
		{
			comeTimeArr[point[i].group] += atoi(point[i].comeTime.c_str());
			goTimeArr[point[i].group] += atoi(point[i].goTime.c_str());
		}
		for (int i = 0; i < n_cluster; ++i)
		{
			for (int j = 0; j < centroid[i].vProbeRqFlag.size(); ++j)
			{
				centroid[i].vProbeRqFlag[j] /= centroid[i].group;
			}
			centroid[i].comeTime = itoa(comeTimeArr[i] / centroid[i].group);
			centroid[i].goTime = itoa(goTimeArr[i] / centroid[i].group);
		}
	}
};

unsigned long long avalible_memory()
{
	char *mem = NULL;
	unsigned long long size, offset;
	//init
	size = 1;

	//increase size of allocation
	while (mem == NULL)
	{
		mem = (char*)malloc(size);
		if (mem)
		{
			free(mem);
			mem = NULL;
			size <<= 1;
		}
		else mem = (char*)1;
	}

	//decrease offset of size
	offset = size >> 2;
	size -= offset;
	while (offset > 1024 * 1024 * 16)
	{
		offset >>= 1;
		mem = (char*)malloc(size);
		if (mem)
		{
			free(mem);
			mem = NULL;
			size += offset;
		}
		else
		{
			size -= offset;
		}
	}
	return size - offset;  //return a safe value
}

enum FLAG
{
	FILENAME = 0x1,
	LINE_BY_LINE = 0x2,
};

int main(int argc, char** argv){
	clock_t START1;
	int flag = 0;
	string flagChecker;
	string inFileName;
	
	for (int i = 1; i < argc; ++i)
	{
		if (argv[i][0] == '-')
		{
			flagChecker = argv[i];
			if (flagChecker == "-l" || flagChecker == "--line-by-line") flag |= LINE_BY_LINE;
			else
			{
				cout << "Syntax error." << endl;
				system("pause");
				return 1;
			}
		}
		else
		{
			if (flag & FILENAME)
			{
				cout << "Syntax error." << endl;
				system("pause");
				return 1;
			}
			else
			{
				inFileName = argv[i];
				flag |= FILENAME;
			}
		}
	}
	if ((flag & FILENAME) == 0)
	{
		cout << "No input file." << endl;
		system("pause");
		return 1;
	}
	

	//open input csv
	fstream inputCSV;
	inputCSV.open(inFileName, std::ios::in);
	if (!inputCSV) return 1;  //can't open input file



	Hash<string, Content> myHash(126, new StringSum());
	Hash<string, Content>::iterator hIter;
	set<string> sAllProbe;

	Content content; //Device, come, go

	//check whether there is enough memory 
	unsigned long long inputCSVSize, avaliMem;
	bool memoryEnough;

	inputCSV.seekg(0, inputCSV.end);
	inputCSVSize = static_cast<unsigned long long>(inputCSV.tellg())*1.5 + CSVREADER_BUF_SIZE;
	avaliMem = avalible_memory();
	cout << "require memory: " << inputCSVSize / 1024 / 1024 << " MB" << endl;
	cout << "avalible memory: " << avaliMem / 1024 / 1024 << " MB" << endl;
	
	if (inputCSVSize >= avaliMem || flag & LINE_BY_LINE) memoryEnough = false;
	else memoryEnough = true;

	int tmpRSSI;

	if (memoryEnough)  // enough memory 
	{
		cout << "memory is enough" << endl;
		vector<vector<string>> data;
		inputCSV.seekg(0, inputCSV.beg);
	
		try{
			START1 = clock();
			readCSV(inputCSV, data);
			for (int i = 1; i < data[0].size(); ++i){
				data[HWSRC][i] = data[HWSRC][i].substr(0, data[HWSRC][i].find_first_of(" "));
				data[HWDEST][i] = data[HWDEST][i].substr(0, data[HWDEST][i].find_first_of(" "));
			}
			START1 = clock() - START1;
			cout << "Read complited, use: " << START1 << " msec" << endl;

			/*use hash to sort data*/
			START1 = clock();
			for (int i = 1; i < data[0].size(); ++i){
				if (i % 1000000 == 0) cout << "do " << i << " data" << endl;
				if (myHash.find(data[HWSRC][i], hIter)){
					(*hIter).content.goTime = data[EPOCH_TIME][i];
				}
				else{
					myHash.insert(hIter, data[HWSRC][i], Content{ data[HWSRC][i], data[EPOCH_TIME][i], data[EPOCH_TIME][i] });
					(*hIter).content.maxRSSI = 0x80000000; //INT_MIN
					(*hIter).content.minRSSI = 0x7FFFFFFF; //INT_MAX
				}
				if (data[RSSI][i] != "" && data[HWSRC][i] != ""){
					tmpRSSI = atob<int>(data[RSSI][i]);
					(*hIter).content.vRSSI.push_back(tmpRSSI);
					(*hIter).content.vRSSITime.push_back(data[EPOCH_TIME][i]);
					if (tmpRSSI > (*hIter).content.maxRSSI) (*hIter).content.maxRSSI = tmpRSSI;
					if (tmpRSSI < (*hIter).content.minRSSI) (*hIter).content.minRSSI = tmpRSSI;
					(*hIter).content.sumRSSI += static_cast<double>(tmpRSSI);
					(*hIter).content.rssiCounter++;
				}
				if (data[SUBTYPE][i] == "Probe Request" && data[SSID][i] != ""){
					(*hIter).content.sProbeRqSSID.insert(data[SSID][i]);
					sAllProbe.insert(data[SSID][i]);
				}
				if (data[SRCIP][i] != ""){
					(*hIter).content.sSelfIP.insert(data[SRCIP][i]);
				}
				if (data[PROTO][i] == "TCP"){
					(*hIter).content.tcpCounter++;
				}
				if (data[PROTO][i] == "UDP"){
					(*hIter).content.udpCounter++;
				}
				if (data[PROTO][i] == "HTTP" || data[PROTO][i] == "HTTP/XML"){
					(*hIter).content.httpCounter++;
					if (data[AGENT][i] != ""){
						(*hIter).content.sAgent.insert(data[AGENT][i]);
					}
				}
				

				if (myHash.find(data[HWDEST][i], hIter)){
					(*hIter).content.goTime = data[EPOCH_TIME][i];
				}
				else
				{
					myHash.insert(hIter, data[HWDEST][i], Content{ data[HWDEST][i], data[EPOCH_TIME][i], data[EPOCH_TIME][i] });
					(*hIter).content.maxRSSI = 0x80000000; //INT_MIN
					(*hIter).content.minRSSI = 0x7FFFFFFF; //INT_MAX
				}
				if (data[DESTIP][i] != ""){
					(*hIter).content.sOtherIP.insert(data[DESTIP][i]);
				}
				if (data[PROTO][i] == "TCP"){
					(*hIter).content.tcpCounter++;
				}
				if (data[PROTO][i] == "UDP"){
					(*hIter).content.udpCounter++;
				}
				if (data[PROTO][i] == "HTTP" || data[PROTO][i] == "HTTP/XML"){
					(*hIter).content.httpCounter++;
				}
			}
			START1 = clock() - START1;
			cout << "Hash: " << START1 << " msec" << endl;
		}
		catch (exception e){
			cout << e.what() << endl;
			data.clear();
			memoryEnough = false;
		}
	}
	if (!memoryEnough) // not enough memory 
	{
		cout << "memory is not enough" << endl;
		vector<string> dataLine;
		inputCSV.seekg(0, inputCSV.beg);
		string line, match;
		int countLine = 0;

		START1 = clock();
		/*read csv line by line*/
		while (getline(inputCSV, line))
		{
			/*count lines*/
			++countLine;
			if (countLine % 1000000 == 0) cout << "do " << countLine << " data" << endl;

			/*parse line from csv*/
			while (line.length() > 1)
			{
				parse_csv(match, line);
				dataLine.push_back(match);
			}

			if (countLine > 1)
			{
				dataLine[HWSRC] = dataLine[HWSRC].substr(0, dataLine[HWSRC].find_first_of(" "));
				dataLine[HWDEST] = dataLine[HWDEST].substr(0, dataLine[HWDEST].find_first_of(" "));
				/*use hash to sort data*/
				if (myHash.find(dataLine[HWSRC], hIter)){
					(*hIter).content.goTime = dataLine[EPOCH_TIME];
				}
				else{
					myHash.insert(hIter, dataLine[HWSRC], Content{ dataLine[HWSRC], dataLine[EPOCH_TIME], dataLine[EPOCH_TIME] });
					(*hIter).content.maxRSSI = 0x80000000; //INT_MIN
					(*hIter).content.minRSSI = 0x7FFFFFFF; //INT_MAX
				}
				if (dataLine[RSSI] != "" && dataLine[HWSRC] != ""){
					tmpRSSI = atob<int>(dataLine[RSSI]);
					(*hIter).content.vRSSI.push_back(tmpRSSI);
					(*hIter).content.vRSSITime.push_back(dataLine[EPOCH_TIME]);
					if (tmpRSSI > (*hIter).content.maxRSSI) (*hIter).content.maxRSSI = tmpRSSI;
					if (tmpRSSI < (*hIter).content.minRSSI) (*hIter).content.minRSSI = tmpRSSI;
					(*hIter).content.sumRSSI += static_cast<double>(tmpRSSI);
					(*hIter).content.rssiCounter++;
				}
				if (dataLine[SUBTYPE] == "Probe Request" && dataLine[SSID] != ""){
					(*hIter).content.sProbeRqSSID.insert(dataLine[SSID]);
					sAllProbe.insert(dataLine[SSID]);
				}
				if (dataLine[SRCIP] != ""){
					(*hIter).content.sSelfIP.insert(dataLine[SRCIP]);
				}
				if (dataLine[PROTO] == "TCP"){
					(*hIter).content.tcpCounter++;
				}
				if (dataLine[PROTO] == "UDP"){
					(*hIter).content.udpCounter++;
				}
				if (dataLine[PROTO] == "HTTP" || dataLine[PROTO] == "HTTP/XML"){
					(*hIter).content.httpCounter++;
					if (dataLine[AGENT] != ""){
						(*hIter).content.sAgent.insert(dataLine[AGENT]);
					}
				}


				if (myHash.find(dataLine[HWDEST], hIter)){
					(*hIter).content.goTime = dataLine[EPOCH_TIME];
				}
				else
				{
					myHash.insert(hIter, dataLine[HWDEST], Content{ dataLine[HWDEST], dataLine[EPOCH_TIME], dataLine[EPOCH_TIME] });
					(*hIter).content.maxRSSI = 0x80000000; //INT_MIN
					(*hIter).content.minRSSI = 0x7FFFFFFF; //INT_MAX
				}
				if (dataLine[DESTIP] != ""){
					(*hIter).content.sOtherIP.insert(dataLine[DESTIP]);
				}
				if (dataLine[PROTO] == "TCP"){
					(*hIter).content.tcpCounter++;
				}
				if (dataLine[PROTO] == "UDP"){
					(*hIter).content.udpCounter++;
				}
				if (dataLine[PROTO] == "HTTP" || dataLine[PROTO] == "HTTP/XML"){
					(*hIter).content.httpCounter++;
				}
			}
			dataLine.clear();
		}
		START1 = clock() - START1;
		cout << "Hash: " << START1 << " msec" << endl;
	}


	/*output result*/
	fstream fs1, fs2, fs3, fs4, fs5, fs6, fs7;

	//deal with output files' names
	string fs1FileName, fs2FileName, fs3FileName, fs4FileName, fs5FileName, fs6FileName, fs7FileName;
	fs1FileName = argv[1];
	fs1FileName = fs1FileName.substr(0, fs1FileName.find_last_of('.'));
	fs2FileName = fs1FileName;
	fs3FileName = fs1FileName;
	fs4FileName = fs1FileName;
	fs5FileName = fs1FileName;
	fs6FileName = fs1FileName;
	fs7FileName = fs1FileName;
	fs1FileName += "_probe_request.csv";
	fs2FileName += "_probe_kmeans.csv";
	fs3FileName += "_customer.csv";
	fs4FileName += "_self_IP.csv";
	fs5FileName += "_go_out_IP.csv";
	fs6FileName += "_agent.csv";
	fs7FileName += "_rssi.csv";
	fs1.open(fs1FileName, ios::out | ios::trunc);
	//fs2.open(fs2FileName, ios::out | ios::trunc);
	fs3.open(fs3FileName, ios::out | ios::trunc);
	fs4.open(fs4FileName, ios::out | ios::trunc);
	fs5.open(fs5FileName, ios::out | ios::trunc);
	fs6.open(fs6FileName, ios::out | ios::trunc);
	fs7.open(fs7FileName, ios::out | ios::trunc);


	if (fs1)
	{
		fs1 << "\"HWAddress\",\"Probe Request SSID\"" << endl;
		for_each(myHash.begin(), myHash.end(), bind(
			[](fstream& fs1, decltype(*myHash.begin()) hashItem)
		{
			if (hashItem.content.sProbeRqSSID.size())
			{
				fs1 << "\"" << hashItem.content.device << "\",\"" << hashItem.content.sProbeRqSSID.size() << "\"";
				for_each(hashItem.content.sProbeRqSSID.begin(), hashItem.content.sProbeRqSSID.end(), bind(
					[](fstream& fs1, string SSID)
				{
					fs1 << ",\"" << SSID << "\"";
				},
					ref(fs1),
					placeholders::_1
					));
				fs1 << endl;
			}
		},
		ref(fs1),
		placeholders::_1
		));
	}

	if (fs4)
	{
		fs4 << "\"HWAddress\",\"Self IP\"" << endl;
		for_each(myHash.begin(), myHash.end(), bind(
			[](fstream& fs4, decltype(*myHash.begin()) hashItem)
		{
			if (hashItem.content.sSelfIP.size())
			{
				fs4 << "\"" << hashItem.content.device << "\",\"" << hashItem.content.sSelfIP.size() << "\"";
				for_each(hashItem.content.sSelfIP.begin(), hashItem.content.sSelfIP.end(), bind(
					[](fstream& fs4, string selfIP)
				{
					fs4 << ",\"" << selfIP << "\"";
				},
				ref(fs4),
				placeholders::_1
				));
				fs4 << endl;
			}
		},
		ref(fs4),
		placeholders::_1
		));
	}

	if (fs5)
	{
		fs5 << "\"HWAddress\",\"Go Out IP\"" << endl;
		for_each(myHash.begin(), myHash.end(), bind(
			[](fstream& fs5, decltype(*myHash.begin()) hashItem)
		{
			if (hashItem.content.sOtherIP.size())
			{
				fs5 << "\"" << hashItem.content.device << "\",\"" << hashItem.content.sOtherIP.size() << "\"";
				for_each(hashItem.content.sOtherIP.begin(), hashItem.content.sOtherIP.end(), bind(
					[](fstream& fs5, string otherIP)
				{
					fs5 << ",\"" << otherIP << "\"";
				},
				ref(fs5),
				placeholders::_1
				));
				fs5 << endl;
			}
		},
		ref(fs5),
		placeholders::_1
		));
	}

	if (fs6)
	{
		fs6 << "\"HWAddress\",\"Agent\"" << endl;
		for_each(myHash.begin(), myHash.end(), bind(
			[](fstream& fs6, decltype(*myHash.begin()) hashItem)
		{
			if (hashItem.content.sAgent.size())
			{
				fs6 << "\"" << hashItem.content.device << "\",\"" << hashItem.content.sAgent.size() << "\"";
				for_each(hashItem.content.sAgent.begin(), hashItem.content.sAgent.end(), bind(
					[](fstream& fs6, string agent)
				{
					fs6 << ",\"" << agent << "\"";
				},
				ref(fs6),
				placeholders::_1
				));
				fs6 << endl;
			}
		},
		ref(fs6),
		placeholders::_1
		));
	}
	
	if (fs7)
	{
		fs7 << "\"HWAddress\",\"RSSI\"" << endl;
		for_each(myHash.begin(), myHash.end(), bind(
			[](fstream& fs7, decltype(*myHash.begin()) hashItem)
		{
			if (hashItem.content.vRSSITime.size())
			{
				fs7 << "\"" << hashItem.content.device << "\",\"" << hashItem.content.vRSSITime.size() << "\"";
				for_each(hashItem.content.vRSSITime.begin(), hashItem.content.vRSSITime.end(), bind(
					[](fstream& fs7, string RSSITime)
				{
					fs7 << ",\"" << RSSITime << "\"";
				},
					ref(fs7),
					placeholders::_1
					));
				fs7 << endl;

				fs7 << "\"\",\"\"";
				for_each(hashItem.content.vRSSI.begin(), hashItem.content.vRSSI.end(), bind(
					[](fstream& fs7, int RSSI)
				{
					fs7 << ",\"" << RSSI << "\"";
				},
					ref(fs7),
					placeholders::_1
					));
				fs7 << endl;
			}
		},
			ref(fs7),
			placeholders::_1
			));
	}
	

	vector<any> vvCustomer;
	vvCustomer.push_back(vector<string>());    // 0 HWAddress
	vvCustomer.push_back(vector<double>());    // 1 Come Time
	vvCustomer.push_back(vector<double>());    // 2 Go Time
	vvCustomer.push_back(vector<double>());    // 3 Stay Time
	vvCustomer.push_back(vector<int>());       // 4 Min RSSI
	vvCustomer.push_back(vector<int>());       // 5 Max RSSI
	vvCustomer.push_back(vector<double>());    // 6 Average RSSI
	vvCustomer.push_back(vector<int>());       // 7 TCP Packets
	vvCustomer.push_back(vector<int>());       // 8 UDP Packets
	vvCustomer.push_back(vector<int>());       // 9 HTTP Packets

	for_each(myHash.begin(), myHash.end(), bind(
		[](vector<any>& vvCustomer, decltype(*(myHash.begin())) item)
	{
		double comeTime, goTime;
		any_cast<vector<string>>(vvCustomer[0]).push_back(item.content.device);
		comeTime = atob<double>(item.content.comeTime);
		goTime = atob<double>(item.content.goTime);
		any_cast<vector<double>>(vvCustomer[1]).push_back(comeTime);
		any_cast<vector<double>>(vvCustomer[2]).push_back(goTime);
		any_cast<vector<double>>(vvCustomer[3]).push_back(goTime - comeTime);
		any_cast<vector<int>>(vvCustomer[4]).push_back(item.content.minRSSI);
		any_cast<vector<int>>(vvCustomer[5]).push_back(item.content.maxRSSI);
		any_cast<vector<double>>(vvCustomer[6]).push_back(item.content.sumRSSI / item.content.rssiCounter);
		any_cast<vector<int>>(vvCustomer[7]).push_back(item.content.tcpCounter);
		any_cast<vector<int>>(vvCustomer[8]).push_back(item.content.udpCounter);
		any_cast<vector<int>>(vvCustomer[9]).push_back(item.content.httpCounter);
	},
		ref(vvCustomer),
		placeholders::_1
		));

	write_CSV(fs3, vector<string>({ "HWAddress", "Come Time", "Go Time", "Stay Time", "Min RSSI", "Max RSSI", "Average RSSI", "TCP Packets", "UDP Packets", "HTTP Packets" }), vvCustomer);


	/* do kmeans with SSIDs of probe requests*/
	/*
	vector<Content> vContentHasProbe;
	Content *backContentPtr, *kmeansPoint, *kmeansCent;
	for (hIter = myHash.begin(); hIter != myHash.end(); ++hIter)
	{
		if (!(*hIter).content.sProbeRqSSID.empty()){
			vContentHasProbe.push_back((*hIter).content);
			backContentPtr = &vContentHasProbe.back();
			backContentPtr->vProbeRqFlag.reserve(sAllProbe.size());
			for (set<string>::iterator siter = sAllProbe.begin(); siter != sAllProbe.end(); ++siter)
			{
				if (backContentPtr->sProbeRqSSID.find(*siter) == backContentPtr->sProbeRqSSID.end()) //not found
				{
					backContentPtr->vProbeRqFlag.push_back(0);
				}
				else backContentPtr->vProbeRqFlag.push_back(1);
			}
		} 
	}
	kmeansPoint = vContentHasProbe.data();
	kmeansCent = lloyd(kmeansPoint, vContentHasProbe.size(), 5, distFunction(), findCentroidFunction());
	//kmeansCent = lloyd(kmeansPoint, vContentHasProbe.size(), 5, distFuncTS(), findCentFuncTS());
	*/

	/*write kmeans result into the csv*/
	/*
	double probeCount;
	fs2 << "\"SrcAddr\",\"group\"";
	fs2 << ",\"count\"";
	for (set<string>::iterator siter = sAllProbe.begin(); siter != sAllProbe.end(); ++siter)
	{
		fs2 << ",\"" << *siter << "\"";
	}
	fs2 << endl;
	for (vector<Content>::iterator vciter = vContentHasProbe.begin(); vciter != vContentHasProbe.end(); ++vciter)
	{
		probeCount = 0;
		for (vector<double>::iterator vditer = (*vciter).vProbeRqFlag.begin(); vditer != (*vciter).vProbeRqFlag.end(); ++vditer)
		{
			probeCount += *vditer;
		}
		fs2 << "\"" << (*vciter).device << "\",\"" << (*vciter).group << "\"";
		fs2 << ",\"" << probeCount << "\"";
		for (vector<double>::iterator viiter = (*vciter).vProbeRqFlag.begin(); viiter != (*vciter).vProbeRqFlag.end(); ++viiter)
		{
			fs2 << ",\"" << *viiter << "\"";
		}
		fs2 << endl;
	}
	*/


	system("pause");
}
