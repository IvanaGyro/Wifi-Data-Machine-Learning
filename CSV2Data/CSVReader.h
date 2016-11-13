#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>


//define the buffer size accroding to number of bits of compiler
#if PTRDIFF_MAX==0x7fffffff  //32-bit compiler
	#define CSVREADER_BUF_SIZE 0x20000000    //500MB
#else                        //64-bit compiler
	#define CSVREADER_BUF_SIZE 0x40000000     //1GB
#endif


class myString{
public:
	myString():size(0), capacity(199){
		str = (char*)malloc(sizeof(char)* 200);
		str[0] = 0;
	}
	
	myString& operator+=(const char ch){
		if (size == capacity) extend();
		str[size] = ch;
		size++;
		str[size] = 0;
		return *this;
	}

	char* c_str(){
		return str;
	}

	void clear(){
		size = 0;
		str[size] = 0;
	}
private:
	int size;
	int capacity;
	char* str;
	void extend(){
		capacity += capacity + 1;
		char* tmp = (char*)malloc(sizeof(char)*(capacity + 1));
		for (int i = 0; i < size; ++i) tmp[i] = str[i];
		free(str);
		str = tmp;
	}
};

template <class T>
void clearvv(std::vector<std::vector<T>>& vv){
	for (std::vector<std::vector<T>>::iterator viter = vv.begin(); viter != vv.end(); viter++){
		(*viter).clear();
	}
	vv.clear();
}

void parse_csv(std::string& match, std::string& line)
{
	int begIdx, endIdx;

	begIdx = 0;
	if (line[begIdx] == ',') ++begIdx;
	if (line[begIdx] == '"') // in double quote
	{
		++begIdx;
		endIdx = begIdx;
		while (line[endIdx] != '"' || (line[endIdx] == '"' && line[endIdx + 1] == '"'))
		{
			if (line[endIdx] != '"') ++endIdx;
			else
			{
				line.erase(endIdx, 1);
				endIdx += 1;
			}
		}
		match = line.substr(begIdx, endIdx - begIdx);
		line = line.substr(endIdx + 1);
	}
	else   // not in double quote
	{
		endIdx = begIdx;
		while (line[endIdx] != ',' && line[endIdx] != 0) ++endIdx;
		match = line.substr(begIdx, endIdx - begIdx);
		line = line.substr(endIdx);
	}
}

void parse_csv_line(std::vector<std::string>& vec, std::string line)
{
	int begIdx, endIdx;

	begIdx = 0;
	int length = line.length();
	while (begIdx < length - 1)
	{
		if (line[begIdx] == ',') ++begIdx;
		if (line[begIdx] == '"') // in double quote
		{
			++begIdx;
			endIdx = begIdx;
			while (line[endIdx] != '"' || (line[endIdx] == '"' && line[endIdx + 1] == '"'))
			{
				if (line[endIdx] != '"') ++endIdx;
				else
				{
					line.erase(endIdx, 1);
					length -= 1;
					endIdx += 1;
				}
			}
			vec.push_back(line.substr(begIdx, endIdx - begIdx));
			begIdx = endIdx + 1;
		}
		else   // not in double quote
		{
			endIdx = begIdx;
			while (line[endIdx] != ',' && line[endIdx] != 0) ++endIdx;
			vec.push_back(line.substr(begIdx, endIdx - begIdx));
			begIdx = endIdx;
		}
	}
}

void line2vv(std::vector<std::vector<std::string>>& data, std::string& line, bool& isInit)
{
	std::string match;
	std::vector<std::string> row;
	line += static_cast<char>(0);

	parse_csv_line(row, line);

	if (!isInit)
	{	
		for (std::vector<std::string>::iterator iter = row.begin(); iter != row.end(); ++iter)
		{
			data.push_back(std::vector<std::string>(1, *iter));
		}
		isInit = true;
	}
	else
	{
		int attrcount = 0;
		int maxAttrCount = data.size();
		for (std::vector<std::string>::iterator iter = row.begin(); iter != row.end() && attrcount < maxAttrCount; ++iter)
		{
			data[attrcount].push_back(*iter);
			++attrcount;
		}
	}
}

int readCSV(std::fstream& fs, std::vector<std::vector<std::string>>& data){
	char *buf, *ptr, *ptr2;
	bool isInit = false;
	
	//allocate buffer
	buf = new char[CSVREADER_BUF_SIZE];
	memset(buf, 0, CSVREADER_BUF_SIZE);
	
	clearvv(data);

	std::string line;
	ptr = buf;
	while (!fs.eof())
	{
		fs.read(ptr, CSVREADER_BUF_SIZE - (ptr - buf));
		ptr = buf;
		int count = 0;
		while ((ptr2 = strchr(ptr, '\n')) != NULL)
		{
			line.assign(ptr, ptr2 - ptr);
			ptr = ptr2 + 1;
			line2vv(data, line, isInit);
			count++;
		}
		if (buf != ptr)
		{
			strcpy_s(buf, CSVREADER_BUF_SIZE - (ptr - buf), ptr);
			ptr = buf + CSVREADER_BUF_SIZE - (ptr - buf);
			memset(ptr, 0, CSVREADER_BUF_SIZE - (ptr - buf));
		}
	}
	while ((ptr2 = strchr(ptr, '\n')) != NULL)
	{
		line.assign(ptr, ptr2 - ptr);
		line2vv(data, line, isInit);
	}
	line = ptr;
	line2vv(data, line, isInit);
	
	delete[] buf;
	return 0;
}
