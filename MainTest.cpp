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
#include <mutex>

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
#define MULTITHREADED_ENABLED 1

enum class ESortType { AlphabeticalAscending, AlphabeticalDescending, LastLetterAscending };

class IStringComparer {
public:
	virtual bool IsFirstAboveSecond(string _first, string _second) = 0;
};

class AlphabeticalAscendingStringComparer : public IStringComparer {
private:
    ESortType sortType;
public:
	AlphabeticalAscendingStringComparer(ESortType type) : sortType(type) {}
	virtual bool IsFirstAboveSecond(string _first, string _second);
};

void DoSingleThreaded(vector<string> _fileList, ESortType _sortType, string _outputName);
void DoMultiThreaded(vector<string> _fileList, ESortType _sortType, string _outputName);

vector<string> BubbleSort(vector<string> _listToSort, ESortType _sortType);
vector<string> RadixSort(vector<string> _listToSort, ESortType _sortType);

vector<string> ReadFile(string _fileName);
void ThreadedReadFile(string _fileName, vector<string>* _listOut);
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
		masterStringList.insert(masterStringList.end(), fileStringList.begin(), fileStringList.end());
	}
	masterStringList = RadixSort(masterStringList, _sortType);
	clock_t endTime = clock();

	WriteAndPrintResults(masterStringList, _outputName, endTime - startTime);
}

void DoMultiThreaded(vector<string> _fileList, ESortType _sortType, string _outputName) {
	clock_t startTime = clock();
	vector<string> masterStringList;
	vector<thread> workerThreads;
	mutex mtx;

    auto sortFile = [&](const string& f) {
        vector<string> fileContents = ReadFile(f);
        lock_guard<mutex> lock(mtx);
        masterStringList.insert(masterStringList.end(), fileContents.begin(), fileContents.end());
    };
    for (const auto& f : _fileList) {
        workerThreads.emplace_back(sortFile, f);
    }
	for (auto& t : workerThreads) {
        t.join();
    }

	masterStringList = RadixSort(masterStringList, _sortType);
	clock_t endTime = clock();

	WriteAndPrintResults(masterStringList, _outputName, endTime - startTime);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// File Processing
////////////////////////////////////////////////////////////////////////////////////////////////////
vector<string> ReadFile(string _fileName) {
	vector<string> listOut;
    ifstream fileIn(_fileName, ifstream::in);
    if (!fileIn) {
		cout << endl << "File not found: " << _fileName;
        return listOut;
    }
    
    string line;
    while (getline(fileIn, line)) {
		// no carriage return at the end of the file made operations annoying down the line
		if (fileIn.peek() == EOF) {
			line += '\r';
		}
        listOut.push_back(line);
    }
    
    fileIn.close();
    return listOut;
}

void ThreadedReadFile(string _fileName, vector<string>* _listOut) {
	*_listOut = ReadFile(_fileName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Sorting
////////////////////////////////////////////////////////////////////////////////////////////////////
bool AlphabeticalAscendingStringComparer::IsFirstAboveSecond(string _first, string _second) {
	if (sortType == ESortType::AlphabeticalAscending) {
		return lexicographical_compare(_first.begin(), _first.end(), _second.begin(), _second.end());
	} else if (sortType == ESortType::AlphabeticalDescending) {
		return lexicographical_compare(_second.begin(), _second.end(), _first.begin(), _first.end());
	} else {
		// string f = tolower(_first)
		// string f = tolower(_second)
		return lexicographical_compare(_first.rbegin(), _first.rend(), _second.rbegin(), _second.rend());
	}
}

vector<string> BubbleSort(vector<string> _listToSort, ESortType _sortType) {
	AlphabeticalAscendingStringComparer* stringSorter = new AlphabeticalAscendingStringComparer(_sortType);
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

vector<string> RadixSort(vector<string> _listToSort, ESortType _sortType) {
	if (_listToSort.empty()) return _listToSort;
	vector<string> sortedList = _listToSort;

    size_t maxLen = 0;
    for (auto& s : sortedList) {
        maxLen = max(maxLen, s.size());
    }
	
    for (int i = maxLen - 1; i >= 0; --i) {
        vector<vector<string>> buckets(27);
		if (_sortType == ESortType::LastLetterAscending) {
			buckets.resize(53);
		}

        for (const auto& s : sortedList) {
			char letter;
			int bucket;
            if (_sortType == ESortType::LastLetterAscending) {
				// for last letter ascending, letters go in buckets back-to-front
				// also have to account for output being uppercase sensitive
				if (i < s.size()) { 
					letter = s[s.size() - 1 - i];
					if (letter >= 'a' && letter <= 'z') {
						bucket = letter - 'a' + 27;
					} else if (letter >= 'A' && letter <= 'Z') {
						bucket = letter - 'A' + 1;
					} else {
						bucket = 0;
					}
				} else {
					bucket = 0;
				}

				// better solution if we were doing a true alphabetical sort without intending
				// for uppercase letters to be "smaller" than lowercase letters
				// letter = (i < s.size()) ? tolower(s[s.size() - 1 - i]) : '\0';
				// bucket = (letter >= 'a' && letter <= 'z') ? letter - 'a' + 1: 0;
            } else {
				// other sorts put letters in buckets from front-to-back
				letter = (i < s.size()) ? tolower(s[i]) : '\0';
				bucket = (letter >= 'a' && letter <= 'z') ? letter - 'a' + 1: 0;
            }
            buckets[bucket].push_back(s);
        }

        sortedList.clear();
        if (_sortType == ESortType::AlphabeticalDescending) {
			// read buckets backwards for descending order
            for (int i = buckets.size() - 1; i >= 0; --i) {
                sortedList.insert(sortedList.end(), buckets[i].begin(), buckets[i].end());
            }
        } else {
            for (auto& bucket : buckets) {
                sortedList.insert(sortedList.end(), bucket.begin(), bucket.end());
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
