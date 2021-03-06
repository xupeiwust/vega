/*
 * Copyright (C) Alneos, s. a r. l. (contact@alneos.fr)
 * Released under the GNU General Public License
 *
 * NastranTokenizer.cpp
 *
 *  Created on: Sep 1, 2013
 *      Author: devel
 */

#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include "NastranTokenizer.h"
#include "../Abstract/SolverInterfaces.h"
#include <ciso646>

using namespace std;
using boost::tokenizer;
using boost::offset_separator;
using boost::lexical_cast;
using boost::trim_copy;

namespace vega {

namespace nastran {

NastranTokenizer::NastranTokenizer(istream& stream, vega::LogLevel logLevel, const string fileName,
		const vega::ConfigurationParameters::TranslationMode translationMode) :
		Tokenizer(stream, logLevel, fileName, translationMode),
		currentField(0), currentSection(SectionType::SECTION_EXECUTIVE),
		nextSymbolType{SymbolType::SYMBOL_KEYWORD} {
}

const string NastranTokenizer::HM_COMMENT_START = "$HMNAME ";

const map<string, NastranTokenizer::CommentType> NastranTokenizer::commentTypeByString = {
        { "BEAMSECTCOLS", CommentType::BEAMSECTCOLS },
        { "BEAMSECTS", CommentType::BEAMSECTS },
        { "COMP", CommentType::COMP },
        { "COMPS", CommentType::COMPS },
        { "CSURF", CommentType::CSURF },
        { "DEQUATIONS", CommentType::DEQUATIONS },
        { "GROUP", CommentType::GROUP },
        { "LOADCOL", CommentType::LOADCOL },
        { "LOADCOLS", CommentType::LOADCOLS },
        { "LOADSTEP", CommentType::LOADSTEP },
        { "MAT", CommentType::MAT },
        { "MATS", CommentType::MATS },
        { "OBJECTIVES", CommentType::OBJECTIVES },
        { "OPTICONSTRAINTS", CommentType::OPTICONSTRAINTS },
        { "PROP", CommentType::PROP },
        { "SYSTCOL", CommentType::SYSTCOL },
        { "VECTORCOL", CommentType::VECTORCOL },
};

NastranTokenizer::LineType NastranTokenizer::getLineType(const string& line) {
	const string beginning = line.substr(0, 8);
	if (beginning.find(",") == string::npos) {
		if (beginning.find("*") == string::npos) {
			return LineType::SHORT_FORMAT;
		} else {
			return LineType::LONG_FORMAT;
		}
	} else {
		return LineType::FREE_FORMAT;
	}
}


void NastranTokenizer::replaceTabs(string& line, bool longFormat) {
	bool found = true;
	do {
		size_t pos = line.find("\t");
		found = pos != string::npos;
		if (found) {
			int FIELD_SIZE = SFSIZE;
			int offset = 0;
			int ipos = static_cast<int>(pos);
			if (longFormat){
				if (ipos>7){
					offset = SFSIZE;
					FIELD_SIZE = LFSIZE;
				}
				if (ipos>71) FIELD_SIZE=LFSIZE;
			}
			int numSpacesNeeded = FIELD_SIZE - ((ipos-offset) % FIELD_SIZE);
			string filler(numSpacesNeeded, ' ');
			boost::replace_first(line, "\t", filler);
		}
	} while (found);
}


string NastranTokenizer::nextSymbolString() {

    if (this->currentField >= this->currentLineVector.size()){
        this->nextSymbolType = SymbolType::SYMBOL_KEYWORD;
        return "";
    }

    string result = boost::trim_right_copy(currentLineVector[currentField]);
    if (this->nextSymbolType == SymbolType::SYMBOL_KEYWORD) {
        boost::to_upper(result);
    }
    this->nextSymbolType = SymbolType::SYMBOL_FIELD;
    this->currentField++;
    if (this->currentField >= this->currentLineVector.size()){
        this->nextSymbolType = SymbolType::SYMBOL_KEYWORD;
    }

    return result;
}

bool NastranTokenizer::readLineSkipComment(string& line, bool firstLine) {
	bool eof = true;
	while (getline(this->instrream, line)) {
		lineNumber += 1;
		bool blankLine = all_of(line.begin(), line.end(), [](int c) {return isblank(c);});
		if (not line.empty() and not blankLine and line[0] != '$') {
			boost::iterator_range<string::iterator> middle_dollar = boost::find_first(line, "$");
			if (middle_dollar) {
				boost::erase_tail(line,
						static_cast<int>(line.size())
								- static_cast<int>(distance(line.begin(), middle_dollar.begin())));
			}
			//if the line is not blank exit the loop
			if (!boost::all(line, [](int c) { return isblank(c); })){
				eof = false;
				break;
			}
        } else if (not firstLine and blankLine) {
            eof = false;
            break;
		} else if (boost::starts_with(line, HM_COMMENT_START)) {
		    boost::erase_head(line, static_cast<int>(HM_COMMENT_START.size()));
		    vector<string> commentParts;
		    boost::split(commentParts, line, boost::is_any_of(" \""), boost::token_compress_on);
            if (commentParts.size() >= 3) {
                bool isPart2Int = all_of(commentParts[1].begin(), commentParts[1].end(), ::isdigit);
                auto result = commentTypeByString.find(commentParts[0]);
                if (isPart2Int and result != commentTypeByString.end()) {
                    labelByCommentTypeAndId[{result->second, stoi(commentParts[1])}] = commentParts[2];
                }
            }
		}
	}
	return eof;
}

void NastranTokenizer::splitFreeFormat(string line, bool firstLine) {
	if (firstLine) {
		split(currentLineVector, line, boost::is_any_of(","));
	} else {
		//skip first field;
		string lineNoContinuation = line.substr(line.find(',') + 1);
		vector<string> otherLine;
		split(otherLine, lineNoContinuation, boost::is_any_of(","));
		currentLineVector.insert(currentLineVector.end(), otherLine.begin(), otherLine.end());
	}
	bool explicitContinuation = false;
	for (size_t fieldIndex = 1; fieldIndex < currentLineVector.size(); fieldIndex += 8) {
		string field = trim_copy(currentLineVector[fieldIndex]);
		if (field[0] == '+') {
			explicitContinuation = true;
			currentLineVector.erase(currentLineVector.begin() + fieldIndex);
		}
	}
	char c = static_cast<char>(this->instrream.peek());
    string line2;
    if (explicitContinuation || c == ',' || c == '+' || c == '*') {
		readLineSkipComment(line2, false);
		splitFreeFormat(line2, false);
	}
}

void NastranTokenizer::parseBulkSectionLine(string line) {
	LineType lineType = getLineType(line);
	switch (lineType) {
	case LineType::LONG_FORMAT:
		splitFixedFormat(line, true, true);
		break;
	case LineType::SHORT_FORMAT:
		splitFixedFormat(line, false, true);
		break;
	case LineType::FREE_FORMAT:
		splitFreeFormat(line, true);
		break;
	default:
		throw vega::ParsingException("line format not recognized: Line N ", this->fileName,
				lineNumber);
	}
}

void NastranTokenizer::executiveControlSection() {
	this->currentSection = SectionType::SECTION_EXECUTIVE;
}


void NastranTokenizer::bulkSection() {
	this->currentSection = SectionType::SECTION_BULK;
//if not first line, read again the current line
	if (not currentLineVector.empty()) {
		currentLineVector.clear();
		//enough in 99% of lines
		currentLineVector.reserve(64);
		currentField = 0;
		this->nextSymbolType = SymbolType::SYMBOL_KEYWORD;
		parseBulkSectionLine(this->currentLine);
	}
}

void NastranTokenizer::parseParameters() {
	split(currentLineVector, this->currentLine, boost::is_any_of("\\="));
}

bool NastranTokenizer::isNextInt() {
	if (nextSymbolType != SymbolType::SYMBOL_FIELD) {
		return false;
	}
	string curField = trim_copy(currentLineVector[currentField]);
	return !curField.empty() && (strspn(curField.c_str(), "-0123456789") == curField.size());
}

bool NastranTokenizer::isNextTHRU() {
	if (nextSymbolType != SymbolType::SYMBOL_FIELD) {
		return false;
	}
	string curField = trim_copy(currentLineVector[currentField]);
	boost::to_upper(curField);
	return curField == "THRU";
}

bool NastranTokenizer::isNextBY() {
	if (nextSymbolType != SymbolType::SYMBOL_FIELD) {
		return false;
	}
	string curField = trim_copy(currentLineVector[currentField]);
	boost::to_upper(curField);
	return curField == "BY";
}

bool NastranTokenizer::isNextDouble() {
	if (nextSymbolType != SymbolType::SYMBOL_FIELD) {
		return false;
	}
	string curField = trim_copy(currentLineVector[currentField]);
	boost::algorithm::erase_all(curField, " ");
	return !curField.empty() && (strspn(curField.c_str(), "-+0123456789.eEdD") == curField.size());
}

bool NastranTokenizer::isNextEmpty(int n) {
    bool result = true;
    for(int i = 0; i < n; i++) {
        if (nextSymbolType != SymbolType::SYMBOL_FIELD) {
            result = false;
            break;
        }
        string curField = trim_copy(currentLineVector[currentField + i]);
        result &= curField.empty();
    }
	return result;
}

bool NastranTokenizer::isEmptyUntilNextKeyword() {
	if (nextSymbolType == SymbolType::SYMBOL_KEYWORD) {
		return true;
	} else if (nextSymbolType == SymbolType::SYMBOL_EOF) {
		return true;
	}
	bool result = true;
	for (size_t i = currentField; i < this->currentLineVector.size() && result; i++) {
		string curField = trim_copy(currentLineVector[i]);
		result &= curField.empty();
	}
	return result;
}

string NastranTokenizer::remainingTextUntilNextKeyword() {
	if (nextSymbolType == SymbolType::SYMBOL_KEYWORD) {
		return "";
	} else if (nextSymbolType == SymbolType::SYMBOL_EOF) {
		return "";
	}
    ostringstream oss;
	for (size_t i = currentField; i < this->currentLineVector.size(); i++) {
        string curfield = trim_copy(currentLineVector[i]);
        if (!curfield.empty()) {
            oss << "," << curfield;
        }
	}
	return oss.str();
}

void NastranTokenizer::nextLine() {

	currentLineVector.clear();
//enough in 99% of lines
	currentLineVector.reserve(128);
	currentField = 0;

	bool iseof = readLineSkipComment(this->currentLine, true);
	if (!iseof) {
		switch (currentSection) {
		case SectionType::SECTION_EXECUTIVE:
			boost::algorithm::trim(this->currentLine);
			split(currentLineVector, this->currentLine, boost::is_any_of("\t\\= "), boost::algorithm::token_compress_on);
			break;
		case SectionType::SECTION_BULK:
			parseBulkSectionLine(this->currentLine);
			break;
		default:
            throw logic_error("Section type not (yet) handled");
		}
		this->nextSymbolType = SymbolType::SYMBOL_KEYWORD;
	} else {
		this->nextSymbolType = SymbolType::SYMBOL_EOF;
	}
}

void NastranTokenizer::splitFixedFormat(string& line, const bool longFormat, const bool firstLine) {
	boost::offset_separator f;

	int fieldMax;
	if (longFormat) {
		int offsets[] = { SFSIZE, LFSIZE, LFSIZE, LFSIZE, LFSIZE, SFSIZE };
		fieldMax = 5;
		f = offset_separator(offsets, offsets + 6);
	} else {
		int offsets[] = { SFSIZE, SFSIZE, SFSIZE, SFSIZE, SFSIZE, SFSIZE, SFSIZE, SFSIZE, SFSIZE, SFSIZE };
		fieldMax = 9;
		f = offset_separator(offsets, offsets + 10);
	}

	replaceTabs(line, longFormat);
	tokenizer<offset_separator> tok(line, f);
	tokenizer<offset_separator>::iterator beg = tok.begin();
	int count = 0;
	if (!firstLine) {
		//todo:check that explicit continuation tokens are the same
		++beg;
		count++;
	}
	bool explicitContinuation = false;
	for (; beg != tok.end(); ++beg) {
		string trimCopy = trim_copy(*beg);
		//erase all the long format specifiers
		if (count == 0) {
			boost::erase_all(trimCopy, "*");
		}
		currentLineVector.push_back(trimCopy);
		if (++count == fieldMax) {
			explicitContinuation = (++beg != tok.end()) && !(trim_copy(*beg).empty());
			if (explicitContinuation && this->logLevel >= vega::LogLevel::TRACE) {
				cout << "explicitContinuation" << endl;
			}
			break;
		}
	}
	string line2;
	char c0 = static_cast<char>(this->instrream.peek());

	while (c0 == '$') { // Trying to skip "comment inside card case"
        getline(this->instrream, line2);
        lineNumber += 1;
        c0 = static_cast<char>(this->instrream.peek());
    }

	if (explicitContinuation or c0 == '+') {
		//todo:check that continuation tokens are the same
		bool iseof = readLineSkipComment(line2, false);
		if (!iseof) {
			splitFixedFormat(line2, longFormat, false);
		} else {
			throw "Continuation Expected: Line N " + to_string(this->lineNumber);
		}
	} else {
		/** Test for automatic continuation : we allow tabulation
		 *  Even if it's, strictly speaking, not authorized by Nastran
		 */
		char c = static_cast<char>(this->instrream.peek());
		if (c == ' ' || c == '+' || c == '*' || c=='\t') {
			readLineSkipComment(line2, false);
			//fill the current line with empty fields
			for (; count < fieldMax; count++) {
				currentLineVector.push_back("");
			}
			bool longFormat2 = (c == '*');
			splitFixedFormat(line2, longFormat2, false);
		}
	}

}

string NastranTokenizer::nextString(bool returnDefaultIfNotFoundOrBlank, string defaultValue) {
    string value =  trim_copy(nextSymbolString());
    boost::to_upper(value);
    if (value.empty()) {
        if (returnDefaultIfNotFoundOrBlank){
            return defaultValue;
        } else {
            string message = "Missing String value for Field Number " + to_string(currentField - 1);
            handleParsingError(message);
        }
    }
    return value;
}

void NastranTokenizer::skip(int fields) {
	if (this->nextSymbolType == SymbolType::SYMBOL_EOF) {
		throw "Attempt to read past the end of file. Line:" + this->lineNumber;
	}

	this->currentField = min(static_cast<unsigned int>(this->currentLineVector.size()),
			this->currentField + fields);

	if (this->currentField == this->currentLineVector.size()) {
		this->nextSymbolType = SymbolType::SYMBOL_KEYWORD;
	} else {
		this->nextSymbolType = SymbolType::SYMBOL_FIELD;
	}
}

void NastranTokenizer::skipToNotEmpty() {
	while (this->isNextEmpty())
		this->skip(1);
}

void NastranTokenizer::skipToNextKeyword() {
    this->currentField= static_cast<int>(this->currentLineVector.size());
    this->nextSymbolType = SymbolType::SYMBOL_KEYWORD;
}


int NastranTokenizer::nextInt(bool returnDefaultIfNotFoundOrBlank, int defaultValue) {
	int result = 0;
	string value = trim_copy(nextSymbolString());
	if (value.empty()) {
	    if (returnDefaultIfNotFoundOrBlank){
	        return defaultValue;
	    }else{
	        string message = "Missing Integer value for Field Number " + to_string(currentField - 1);
	        handleParsingError(message);
	    }
	}
	try {
		result = lexical_cast<int>(value);
	} catch (boost::bad_lexical_cast &) {
		string currentFieldstr =
				currentField == 0 ? "LAST" : to_string(currentField - 1);
		string message = "Value [" + value + "] can't be converted to int. Field Num: "
				+ currentFieldstr;
		handleParsingError(message);
	}
	return result;
}

list<int> NastranTokenizer::nextInts() {
	list<int> result;
	while(isNextInt() or isNextTHRU()) {
        if (isNextInt()) {
            result.push_back(nextInt());
        } else if (isNextTHRU()) {
            skip(1);
            int start = result.back();
            int endint = nextInt();
            int step = 1;
            if (start > endint) {
                result.pop_back();
                result.push_back(endint);
                swap(start, endint);
            }
            if (isNextBY()) {
                skip(1);
                step = nextInt();
            }
            for(int i=start+1;i<=endint;i+=step) {
                result.push_back(i);
            }
        }
	}
	return result;
}

list<double> NastranTokenizer::nextDoubles() {
	list<double> result;
	while(isNextDouble()) {
      result.push_back(nextDouble());
	}
	return result;
}

double NastranTokenizer::nextDouble(bool returnDefaultIfNotFoundOrBlank, double defaultValue) {
	double result = 0.0;
	string value = trim_copy(nextSymbolString());
	if (value.empty()) {
	    if (returnDefaultIfNotFoundOrBlank) {
	        return defaultValue;
	    } else {
	        string message = "Missing Double value for Field Number " + to_string(currentField - 1);
	        handleParsingError(message);
	    }
	}

	boost::replace_all(value, "d", "e");
	boost::replace_all(value, "D", "E");
	boost::algorithm::erase_all(value, " ");
	size_t position = value.find_first_of("+-", 1);
	if (position != string::npos and position != value.find_first_of("eE", 1) + 1) {
		value.insert(position, "E");
	}
	try {
		result = lexical_cast<double>(value);
	} catch (boost::bad_lexical_cast &) {
		string currentFieldstr =
				currentField == 0 ? "LAST" : to_string(currentField - 1);
		string message = "Value [" + value + "] can't be converted to double. Field Num: "
				+ currentFieldstr;
		handleParsingError(message);
	}
	return result;
}

vector<string> NastranTokenizer::currentDataLine() const {
	return currentLineVector;
}

string NastranTokenizer::currentRawDataLine() const {
	return this->currentLine;
}

} /* namespace nastran */

} /* namespace vega */
