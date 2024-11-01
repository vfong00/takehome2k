// Copyright Mass Media. All rights reserved. DO NOT redistribute.

////////////////////////////////////////////////////////////////////////////////////////////////////
// Task List
////////////////////////////////////////////////////////////////////////////////////////////////////
// Notes
//	* This test requires a compiler with C++17 support and was built for Visual Studio 2017.
// 		* Tested on Linux (Ubuntu 20.04) with: g++ -Wall -Wextra -pthread -std=c++17 MainTest.cpp
//		* Tested on Mac OS Big Sur, 11.0.1 and latest XCode updates.
//	* Correct output for all three sorts is in the CorrectOutput folder. BeyondCompare is recommended for comparing output.
//	* Functions, classes, and algorithms can be added and changed as needed.
//	* DO NOT use std::sort().
// Objectives
//	* 20 points - Make the program produce a SingleAscending.txt file that matches CorrectOutput/SingleAscending.txt.
//	* 10 points - Make the program produce a SingleDescending.txt file that matches CorrectOutput/SingleDescending.txt.
//	* 10 points - Make the program produce a SingleLastLetter.txt file that matches CorrectOutput/SingleLastLetter.txt.
//	* 20 points - Write a brief report on what you found, what you did, and what other changes to the code you'd recommend.
//	* 10 points - Make the program produce three MultiXXX.txt files that match the equivalent files in CorrectOutput; it must be multi-threaded.
//	* 20 points - Improve performance as much as possible on both single-threaded and multi-threaded versions; speed is more important than memory usage.
//	* 10 points - Improve safety and stability; fix memory leaks and handle unexpected input and edge cases.
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <string>
#include <iostream>
#include <fstream>
#include <thread>
#include <atomic>
#include <ctime>
#include <vector>

#ifndef INCLUDE_STD_FILESYSTEM_EXPERIMENTAL
#   if defined(__cpp_lib_filesystem)
#       define INCLUDE_STD_FILESYSTEM_EXPERIMENTAL 0
#   elif defined(__cpp_lib_experimental_filesystem)
#       define INCLUDE_STD_FILESYSTEM_EXPERIMENTAL 1
#   elif !defined(__has_include)
#       define INCLUDE_STD_FILESYSTEM_EXPERIMENTAL 1
#   elif __has_include(<filesystem>)
#       ifdef _MSC_VER
#           if __has_include(<yvals_core.h>)
#               include <yvals_core.h>
#               if defined(_HAS_CXX17) && _HAS_CXX17
#                   define INCLUDE_STD_FILESYSTEM_EXPERIMENTAL 0
#               endif
#           endif
#           ifndef INCLUDE_STD_FILESYSTEM_EXPERIMENTAL
#               define INCLUDE_STD_FILESYSTEM_EXPERIMENTAL 1
#           endif
#       else
#           define INCLUDE_STD_FILESYSTEM_EXPERIMENTAL 0
#       endif
#   elif __has_include(<experimental/filesystem>)
#       define INCLUDE_STD_FILESYSTEM_EXPERIMENTAL 1
#   else
#       error Could not find system header "<filesystem>" or "<experimental/filesystem>"
#   endif
#   if INCLUDE_STD_FILESYSTEM_EXPERIMENTAL
#       include <experimental/filesystem>
     	namespace fs = std::experimental::filesystem;
#   else
#       include <filesystem>
#		if __APPLE__
			namespace fs = std::__fs::filesystem;
#		else
			namespace fs = std::filesystem;
#		endif
#   endif
#endif

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Definitions and Declarations
////////////////////////////////////////////////////////////////////////////////////////////////////
#define MULTITHREADED_ENABLED 0

enum class ESortType { AlphabeticalAscending, AlphabeticalDescending, LastLetterAscending };

class IStringComparer {
public:
	virtual bool IsFirstAboveSecond(string _first, string _second) = 0;
};

class AlphabeticalAscendingStringComparer : public IStringComparer {
public:
	virtual bool IsFirstAboveSecond(string _first, string _second);
};

void DoSingleThreaded(vector<string> _fileList, ESortType _sortType, string _outputName);
void DoMultiThreaded(vector<string> _fileList, ESortType _sortType, string _outputName);
vector<string> ReadFile(string _fileName);
void ThreadedReadFile(string _fileName, vector<string>* _listOut);
vector<string> BubbleSort(vector<string> _listToSort, ESortType _sortType);
void WriteAndPrintResults(const vector<string>& _masterStringList, string _outputName, int _clocksTaken);

////////////////////////////////////////////////////////////////////////////////////////////////////
// Main
////////////////////////////////////////////////////////////////////////////////////////////////////
int main() {
	// Enumerate the directory for input files
	vector<string> fileList;
    string inputDirectoryPath = "../InputFiles";
    for (const auto & entry : fs::directory_iterator(inputDirectoryPath)) {
		if (!fs::is_directory(entry)) {
			fileList.push_back(entry.path().string());
		}
	}

	// Do the stuff
	DoSingleThreaded(fileList, ESortType::AlphabeticalAscending,	"SingleAscending");
	DoSingleThreaded(fileList, ESortType::AlphabeticalDescending,	"SingleDescending");
	DoSingleThreaded(fileList, ESortType::LastLetterAscending,		"SingleLastLetter");
#if MULTITHREADED_ENABLED
	DoMultiThreaded(fileList, ESortType::AlphabeticalAscending,		"MultiAscending");
	DoMultiThreaded(fileList, ESortType::AlphabeticalDescending,	"MultiDescending");
	DoMultiThreaded(fileList, ESortType::LastLetterAscending,		"MultiLastLetter");
#endif

	// Wait
	cout << endl << "Finished...";
	getchar();
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// The Stuff
////////////////////////////////////////////////////////////////////////////////////////////////////
void DoSingleThreaded(vector<string> _fileList, ESortType _sortType, string _outputName) {
	clock_t startTime = clock();
	vector<string> masterStringList;
	for (unsigned int i = 0; i < _fileList.size(); ++i) {
		vector<string> fileStringList = ReadFile(_fileList[i]);
		for (unsigned int j = 0; j < fileStringList.size(); ++j) {
			masterStringList.push_back(fileStringList[j]);
		}

		masterStringList = BubbleSort(masterStringList, _sortType);
		_fileList.erase(_fileList.begin() + i);
	}
	clock_t endTime = clock();

	WriteAndPrintResults(masterStringList, _outputName, endTime - startTime);
}

void DoMultiThreaded(vector<string> _fileList, ESortType _sortType, string _outputName) {
	clock_t startTime = clock();
	vector<string> masterStringList;
	vector<thread> workerThreads(_fileList.size());
	for (unsigned int i = 0; i < _fileList.size() - 1; ++i) {
		workerThreads[i] = thread(ThreadedReadFile, _fileList[i], &masterStringList);
	}
	
	workerThreads[workerThreads.size() - 1].join(); 

	masterStringList = BubbleSort(masterStringList, _sortType);
	clock_t endTime = clock();

	WriteAndPrintResults(masterStringList, _outputName, endTime - startTime);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// File Processing
////////////////////////////////////////////////////////////////////////////////////////////////////
vector<string> ReadFile(string _fileName) {
	vector<string> listOut;
	streampos positionInFile = 0;
	bool endOfFile = false;
	while (!endOfFile) {
		ifstream fileIn(_fileName, ifstream::in);
		fileIn.seekg(positionInFile, ios::beg);

		string* tempString = new string();
		getline(fileIn, *tempString);

		endOfFile = fileIn.peek() == EOF;
		positionInFile = endOfFile ? ios::beg : fileIn.tellg();
		if (!endOfFile)
			listOut.push_back(*tempString);

		fileIn.close();
	}
	return listOut; 
}

void ThreadedReadFile(string _fileName, vector<string>* _listOut) {
	*_listOut = ReadFile(_fileName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Sorting
////////////////////////////////////////////////////////////////////////////////////////////////////
bool AlphabeticalAscendingStringComparer::IsFirstAboveSecond(string _first, string _second) {
	unsigned int i = 0;
	while (i < _first.length() && i < _second.length()) {
		if (_first[i] < _second[i])
			return true;
		else if (_first[i] > _second[i])
			return false;
		++i;
	}
	return (i == _second.length());
}

vector<string> BubbleSort(vector<string> _listToSort, ESortType _sortType) {
	AlphabeticalAscendingStringComparer* stringSorter = new AlphabeticalAscendingStringComparer();
	vector<string> sortedList = _listToSort;
	for (unsigned int i = 0; i < sortedList.size() - 1; ++i) {
		for (unsigned int j = 0; j < sortedList.size() - 1; ++j) {
			if (!stringSorter->IsFirstAboveSecond(sortedList[j], sortedList[j+1])) {
				string tempString = sortedList[j];
				sortedList[j] = sortedList[j+1];
				sortedList[j+1] = tempString;
			}
		}
	}
	return sortedList; 
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Output
////////////////////////////////////////////////////////////////////////////////////////////////////
void WriteAndPrintResults(const vector<string>& _masterStringList, string _outputName, int _clocksTaken) {
	cout << endl << _outputName << "\t- Clocks Taken: " << _clocksTaken << endl;
	
	ofstream fileOut(_outputName + ".txt", ofstream::trunc);
	for (unsigned int i = 0; i < _masterStringList.size(); ++i) {
		fileOut << _masterStringList[i] << endl;
	}
	fileOut.close();
}
