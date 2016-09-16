#ifndef CSVWRITER_H
#define CSVWRITER_H

#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <iomanip>


int write_CSV(std::fstream& outputF, std::vector<std::string> vTitle, std::vector<std::vector<std::string>> vContent)
{
	if (!outputF) return 1;
	int numColumn;
	int maxRow = 0;
	if ((numColumn = vTitle.size()) != vContent.size() || numColumn == 0) return 2;

	//write title
	outputF << "\"" << vTitle[0] << "\"";
	if (numColumn > 1)
	{
		std::for_each(vTitle.begin() + 1, vTitle.end(), std::bind(
			[](std::fstream& outputF, std::string str){ outputF << ",\"" << str << "\""; },
			std::ref(outputF),
			std::placeholders::_1
			));
	}
	outputF << std::endl;
	//finx max number of row of columns
	for_each(vContent.begin(), vContent.end(), std::bind(
		[](int& maxRow, std::vector<std::string>& column){ if (column.size() > maxRow) maxRow = column.size(); },
		std::ref(maxRow),
		std::placeholders::_1
		));
	//write content
	for (int i = 0; i < maxRow; ++i)
	{
		if (vContent.size() <= i) outputF << "\"\"";
		else outputF << "\"" << vContent[0][i] << "\"";
		if (numColumn > 1)
		{
			for_each(vContent.begin() + 1, vContent.end(), std::bind(
				[](std::fstream& outputF, int row, std::vector<std::string>& column){
				if (column.size() <= row) outputF << ",\"\"";
				else outputF << ",\"" << column[row] << "\"";
			},
				std::ref(outputF),
				i,
				std::placeholders::_1
				));
		}
		outputF << std::endl;
	}
	return 0;
}

#ifdef IVEN_ANY //need any.h created by Iven CJ7


/*
* This function only support vector<int>,vector<string>,vector<float>,vector<double> in "any".
* If there is any other type in vContent (vector<any>), will occur a bad_typeid error which
* can be catch by std::exception.
*/

int write_CSV(std::fstream& outputF, std::vector<std::string> vTitle, std::vector<any_type::any> vContent)
{
	enum TYPE_VEC{
		INT,
		FLOAT,
		DOUBLE,
		STRING
	};
	if (!outputF) return 1;
	int numColumn;
	int maxRow = 0;
	if ((numColumn = vTitle.size()) != vContent.size() || numColumn == 0) return 2;

	//write title
	outputF << "\"" << vTitle[0] << "\"";
	if (numColumn > 1)
	{
		std::for_each(vTitle.begin() + 1, vTitle.end(), std::bind(
			[](std::fstream& outputF, std::string& str){ outputF << ",\"" << str << "\""; },
			std::ref(outputF),
			std::placeholders::_1
			));
	}
	outputF << std::endl;
	//find max number of row of columns, and record the type of vector in vContent
	std::vector<TYPE_VEC> vType;
	std::vector<int> vRow;
	for_each(vContent.begin(), vContent.end(), std::bind(
		[](int& maxRow, std::vector<TYPE_VEC>& vType, std::vector<int>& vRow, any_type::any& column){
			int row;
			if (column.type() == typeid(std::vector<int>))
			{
				row = any_type::any_cast<std::vector<int>>(column).size();
				vType.push_back(INT);
			}
			else if (column.type() == typeid(std::vector<float>))
			{
				row = any_type::any_cast<std::vector<float>>(column).size();
				vType.push_back(FLOAT);
			}
			else if (column.type() == typeid(std::vector<double>))
			{	
				row = any_type::any_cast<std::vector<double>>(column).size();
				vType.push_back(DOUBLE);
			}
			else
			{
				row = any_type::any_cast<std::vector<std::string>>(column).size();
				vType.push_back(STRING);
			}
			vRow.push_back(row);
			if (row > maxRow) maxRow = row; 
		},
		std::ref(maxRow),
		std::ref(vType),
		std::ref(vRow),
		std::placeholders::_1
		));
	//write content
	int count;
	outputF << std::setprecision(std::numeric_limits<double>::max_digits10);
	for (int i = 0; i < maxRow; ++i)
	{
		std::string tmpstr;
		if (vRow[0] <= i) outputF << "\"\"";
		else
		{
			outputF << "\"";
			if (vType[0] == INT) outputF << any_type::any_cast<std::vector<int>>(vContent[0])[i];
			else if (vType[0] == FLOAT) outputF << any_type::any_cast<std::vector<float>>(vContent[0])[i];
			else if (vType[0] == DOUBLE) outputF << any_type::any_cast<std::vector<double>>(vContent[0])[i];
			else outputF << any_type::any_cast<std::vector<std::string>>(vContent[0])[i];
			outputF << "\"";
		}
		if (numColumn > 1)
		{
			count = 1;
			for_each(vContent.begin() + 1, vContent.end(), std::bind(
				[](std::fstream& outputF, int& count, int row, std::vector<TYPE_VEC>& vType, std::vector<int>& vRow, any_type::any& column){
				if (vRow[count] <= row) outputF << ",\"\"";
				else
				{
					outputF << ",\"";
					if (vType[count] == INT) outputF << any_type::any_cast<std::vector<int>>(column)[row];
					else if (vType[count] == FLOAT) outputF << any_type::any_cast<std::vector<float>>(column)[row];
					else if (vType[count] == DOUBLE) outputF << any_type::any_cast<std::vector<double>>(column)[row];
					else outputF << any_type::any_cast<std::vector<std::string>>(column)[row];
					outputF << "\"";
				}
				++count;
			},
				std::ref(outputF),
				std::ref(count),
				i,
				std::ref(vType),
				std::ref(vRow),
				std::placeholders::_1
				));
		}
		outputF << std::endl;
	}
	return 0;
}


#endif //IVEN_ANY 


#endif //CSVWRITER_H