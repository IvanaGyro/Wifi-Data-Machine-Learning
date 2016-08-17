#include <fstream>
#include <string>
#include <vector>
#include <cstring>


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

int readCSV(char* file, std::vector<std::vector<std::string>>& data){
	std::string line;
	std::fstream fs;
	char *buf, *ptr, *ptr2;
	int state = 0, idx, filesize, attrcount;
	
	fs.open(file, std::ios::in | std::ios::binary);
	if (!fs) return 0;
	fs.seekg(0, std::ios::end);
	filesize = static_cast<int>(fs.tellg());
	fs.seekg(0, std::ios::beg);
	buf = new char[filesize + 2];
	ptr = buf;
	fs.read(buf, filesize);
	if (buf[filesize - 1] == '\n') buf[filesize] = 0;
	else{
		buf[filesize] = '\n';
		buf[filesize + 1] = 0;
	}

	clearvv(data);
	
	ptr2 = strchr(ptr, '\n');
	line.assign(ptr, ptr2 - ptr);
	ptr = ptr2 + 1;
	for (unsigned int i = 0; i < line.length(); ++i){
		if (state == 0){ // not in string
			if (line[i] == '"'){
				state = 1;
				idx = i + 1;
			}
		}
		else if (state == 1){
			while (state == 1){
				if (line[i] != '"') i++;
				else if (line[i + 1] == '"'){
					line.erase(i, 1);
					i++;
				}
				else{
					data.push_back(std::vector<std::string>(1, line.substr(idx, i - idx)));
					state = 0;
				}
			}
		}
	}

	while (*ptr){

		ptr2 = strchr(ptr, '\n');
		line.assign(ptr, ptr2 - ptr);
		ptr = ptr2 + 1;

		attrcount = 0;
		for (unsigned int i = 0; i < line.length(); i++){
			if (state == 0){ // not in string
				if (line[i] == '"'){
					state = 1;
					idx = i + 1;
				}
			}
			else if (state == 1){
				while (state == 1){
					if (line[i] != '"') i++;
					else if (line[i + 1] == '"'){
						line.erase(i, 1);
						i++;
					}
					else{
						data[attrcount].push_back(line.substr(idx, i - idx));
						attrcount++;
						state = 0;
					}
				}		
			}
		}
	}
	return 1;
}

