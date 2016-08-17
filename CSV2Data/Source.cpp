#include <iostream>
#include <vector>
#include <set>
#include <tuple>
#include <string>
#include <ctime>
#include <fstream>
#include "CSVReader.h"
#include "hash.h"
#include "KMeans.h"

using namespace std;

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
};

class distFunction{
public:
	distFunction(){}
	/*double operator()(Content *a, Content *b){
		double sum = 0;
		for (int i = 0; i < a->vProbeRqFlag.size(); ++i)
		{
			sum += abs(a->vProbeRqFlag[i] - b->vProbeRqFlag[i]);
		}
		return sum;
	}*/
	double operator()(Content *a, Content *b){
		double sumA = 0, sumB = 0;
		for (int i = 0; i < a->vProbeRqFlag.size(); ++i)
		{
			sumA += a->vProbeRqFlag[i];
			sumB += b->vProbeRqFlag[i];
		}
		return abs(sumA-sumB);
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

	fstream fs1, fs2;
	fs1.open("hash.txt", ios::out | ios::trunc);
	fs2.open("probe_kmeans.csv", ios::out | ios::trunc);
	if (!fs1 || !fs2)
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
		
		if (myHash.find(data[HWDEST][i], hIter)){
			(*hIter).content.goTime = data[TIME][i];
		}
		else
		{
			myHash.insert(hIter, data[HWDEST][i], Content{ data[HWDEST][i], data[TIME][i], data[TIME][i] });
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
}
