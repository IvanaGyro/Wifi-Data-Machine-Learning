#include <iostream>
#include <vector>
#include <set>
#include <tuple>
#include <string>
#include <ctime>
#include <fstream>
#include <algorithm>
#include "CSVReader.h"
#include "hash.h"
#include "KMeans.h"

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

enum Label_CSV{
	NO,
	TIME,
	HWSRC,
	HWDEST,
	PROTO,
	SUBTYPE,
	SSID
};

struct Content{
	string device;
	string comeTime;
	string goTime;
	set<string> sProbeRqSSID; //SSID of probe request
	vector<double> vProbeRqFlag;
	int group;
	int httpCounter;
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
			cout << "centroid["<< i << "].group" << centroid[i].group << endl;
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

int main(int argc, char** argv){
	clock_t START1, START2;

	while (argc != 2) return 0;
	vector<vector<string>> data;
	START1 = clock();
	if (!readCSV(argv[1], data)){
		cout << "Error: Can NOT read the csv file" << endl;
		return 0;
	}
	for (int i = 1; i < data[0].size(); ++i){
		data[HWSRC][i] = data[HWSRC][i].substr(0, data[HWSRC][i].find_first_of(" "));
		data[HWDEST][i] = data[HWDEST][i].substr(0, data[HWDEST][i].find_first_of(" "));
	}
	START1 = clock() - START1;

	cout << "Read complited, use: " << START1 << " msec" << endl;
	Content content; //Device, come, go

	fstream fs1, fs2, fs3;

	//deal with output files' names
	string fs1FileName, fs2FileName, fs3FileName;
	fs1FileName = argv[1];
	fs1FileName = fs1FileName.substr(0, fs1FileName.find_last_of('.'));
	fs2FileName = fs1FileName;
	fs3FileName = fs1FileName;
	fs1FileName += "_hash.txt";
	fs2FileName += "_probe_kmeans.csv";
	fs3FileName += "_customer.csv";
	fs1.open(fs1FileName, ios::out | ios::trunc);
	fs2.open(fs2FileName, ios::out | ios::trunc);
	fs3.open(fs3FileName, ios::out | ios::trunc);

	//check wheather the output files can be open
	if (!fs1 || !fs2 || !fs3)
	{
		cout << "Can not open the output file." << endl;
		return 1;
	}

	START1 = clock();
	Hash<string, Content> myHash(126, new StringSum());
	Hash<string, Content>::iterator hIter;
	set<string> sAllProbe;

	for (int i = 1; i < data[0].size(); ++i){
		if (i % 100000 == 0) cout << "do " << i << " data" << endl;
		if (myHash.find(data[HWSRC][i], hIter)){
			(*hIter).content.goTime = data[TIME][i];
		}
		else{
			myHash.insert(hIter, data[HWSRC][i], Content{ data[HWSRC][i], data[TIME][i], data[TIME][i] });
		}
		if (data[SUBTYPE][i] == "Probe Request" && data[SSID][i] != ""){
			(*hIter).content.sProbeRqSSID.insert(data[SSID][i]);
			sAllProbe.insert(data[SSID][i]);
		}
		if (data[PROTO][i] == "HTTP" || data[PROTO][i] == "HTTP/XML"){
			(*hIter).content.httpCounter++;
		}
		
		if (myHash.find(data[HWDEST][i], hIter)){
			(*hIter).content.goTime = data[TIME][i];
		}
		else
		{
			myHash.insert(hIter, data[HWDEST][i], Content{ data[HWDEST][i], data[TIME][i], data[TIME][i] });
		}
		if (data[PROTO][i] == "HTTP" || data[PROTO][i] == "HTTP/XML"){
			(*hIter).content.httpCounter++;
		}
		
	}
	START1 = clock() - START1;
	cout << "Hash: " << START1 << " msec" << endl;

	for (hIter = myHash.begin(); hIter != myHash.end(); ++hIter){
		fs1 << "Dev: " << (*hIter).content.device << endl;
		fs1 << "\tLeave time: " << atoi((*hIter).content.goTime.c_str()) - atoi((*hIter).content.comeTime.c_str()) << " s" << endl;
		fs1 << "\tProbe Requet SSID:" << endl;
		for (set<string>::iterator seiter = (*hIter).content.sProbeRqSSID.begin(); seiter != (*hIter).content.sProbeRqSSID.end(); ++seiter){
			fs1 << "\t\t" << *seiter << endl;
		}
	}

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

	fs3 << "\"HWAddress\",\"Come Time\",\"Go Time\",\"Leave Time\",\"HTTP Packets\",\"HTTP Interval\"" << endl;
	class print_fs3_content
	{
		fstream *fs3Ptr;
	public:
		print_fs3_content(fstream *fsPtr) :fs3Ptr(fsPtr){}
		void operator()(Hash<string, Content>::value_type item)
		{
			*fs3Ptr << "\"" << item.content.device << "\"";
			*fs3Ptr << ",\"" << item.content.comeTime << "\"";
			*fs3Ptr << ",\"" << item.content.goTime << "\"";
			int leaveTime = atoi(item.content.goTime.c_str()) - atoi(item.content.comeTime.c_str());
			*fs3Ptr << ",\"" << leaveTime << "\"";
			*fs3Ptr << ",\"" << item.content.httpCounter << "\"";
			if (item.content.httpCounter) *fs3Ptr << ",\"" << static_cast<int>(leaveTime / item.content.httpCounter) << "\"";
			else *fs3Ptr << ",\"" << -1 << "\"";
			*fs3Ptr << endl;
		}
	};
	for_each(myHash.begin(), myHash.end(), print_fs3_content(&fs3));
}
