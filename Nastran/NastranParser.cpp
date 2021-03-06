/*
 * Copyright (C) Alneos, s. a r. l. (contact@alneos.fr)
 * This file is part of Vega.
 *
 *   Vega is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   Vega is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Vega.  If not, see <http://www.gnu.org/licenses/>.
 *
 * NastranParser.cpp
 *
 *  Created on: Dec 24, 2012
 *      Author: dallolio
 */

#include "NastranParser.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/tokenizer.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <ciso646>

namespace vega {

constexpr int Globals::UNAVAILABLE_INT; // For C++11
constexpr double Globals::UNAVAILABLE_DOUBLE; // For C++11

namespace nastran {

using namespace std;
namespace fs = boost::filesystem;
namespace alg = boost::algorithm;
using boost::trim;
using boost::trim_copy;
using boost::to_upper;

const unordered_map<string, NastranParser::parseElementFPtr> NastranParser::PARSE_FUNCTION_BY_KEYWORD =
        {
                { "BCONP", &NastranParser::parseBCONP },
                { "BCTABLE", &NastranParser::parseBCTABLE },
                { "BCBODY", &NastranParser::parseBCBODY },
                { "BFRIC", &NastranParser::parseBFRIC },
                { "BLSEG", &NastranParser::parseBLSEG },
                { "BSCONP", &NastranParser::parseBSCONP },
                { "BSSEG", &NastranParser::parseBSSEG },
                { "BSURF", &NastranParser::parseBSURF },
                { "CBAR", &NastranParser::parseCBAR },
                { "CBEAM", &NastranParser::parseCBEAM },
                { "CBUSH", &NastranParser::parseCBUSH },
                { "CDAMP1", &NastranParser::parseCDAMP1 },
                { "CGAP", &NastranParser::parseCGAP },
                { "CELAS1", &NastranParser::parseCELAS1 },
                { "CELAS2", &NastranParser::parseCELAS2 },
                { "CELAS4", &NastranParser::parseCELAS4 },
                { "CIHEX1", &NastranParser::parseCHEXA },
                { "CIHEX2", &NastranParser::parseCHEXA },
                { "CHEXA", &NastranParser::parseCHEXA },
                { "CMASS2", &NastranParser::parseCMASS2 },
                { "CONM1", &NastranParser::parseCONM1 },
                { "CONM2", &NastranParser::parseCONM2 },
                { "CONROD", &NastranParser::parseCONROD },
                { "CORD1R", &NastranParser::parseCORD1R },
                { "CORD2C", &NastranParser::parseCORD2C },
                { "CORD2R", &NastranParser::parseCORD2R },
                { "CORD2S", &NastranParser::parseCORD2S },
                { "CPENTA", &NastranParser::parseCPENTA },
                { "CPYRA", &NastranParser::parseCPYRAM },
                { "CPYRAM", &NastranParser::parseCPYRAM },
                { "CQUAD", &NastranParser::parseCQUAD },
                { "CQUAD4", &NastranParser::parseCQUAD4 },
                { "CQUAD8", &NastranParser::parseCQUAD8 },
                { "CQUADR", &NastranParser::parseCQUADR },
                { "CRIGD1", &NastranParser::parseCRIGD1 },
                { "CROD", &NastranParser::parseCROD },
                { "CTETRA", &NastranParser::parseCTETRA },
                { "CTRIA3", &NastranParser::parseCTRIA3 },
                { "CTRIA6", &NastranParser::parseCTRIA6 },
                { "CTRIAR", &NastranParser::parseCTRIAR },
                { "DAREA", &NastranParser::parseDAREA },
                { "DELAY", &NastranParser::parseDELAY },
                { "DLOAD", &NastranParser::parseDLOAD },
                { "DMIG", &NastranParser::parseDMIG },
                { "DPHASE", &NastranParser::parseDPHASE },
                { "EIGB", &NastranParser::parseEIGB },
                { "EIGR", &NastranParser::parseEIGR },
                { "EIGRL", &NastranParser::parseEIGRL },
                { "FORCE", &NastranParser::parseFORCE },
                { "FORCE1", &NastranParser::parseFORCE1 },
                { "FORCE2", &NastranParser::parseFORCE2 },
                { "FREQ", &NastranParser::parseFREQ },
                { "FREQ1", &NastranParser::parseFREQ1 },
                { "FREQ3", &NastranParser::parseFREQ3 },
                { "FREQ4", &NastranParser::parseFREQ4 },
                { "GRAV", &NastranParser::parseGRAV },
                { "GRID", &NastranParser::parseGRID },
                { "INCLUDE", &NastranParser::parseInclude },
                { "LSEQ", &NastranParser::parseLSEQ },
                { "LOAD", &NastranParser::parseLOAD },
                { "MAT1", &NastranParser::parseMAT1 },
                { "MAT8", &NastranParser::parseMAT8 },
                { "MATHP", &NastranParser::parseMATHP },
                { "MATS1", &NastranParser::parseMATS1 },
                { "MOMENT", &NastranParser::parseMOMENT },
                { "MPC", &NastranParser::parseMPC },
                { "NLPARM", &NastranParser::parseNLPARM },
                { "NLPCI", &NastranParser::parseNLPCI },
                { "PARAM", &NastranParser::parsePARAM },
                { "PBAR", &NastranParser::parsePBAR },
                { "PBARL", &NastranParser::parsePBARL },
                { "PBEAM", &NastranParser::parsePBEAM },
                { "PBEAML", &NastranParser::parsePBEAML },
                { "PBUSH", &NastranParser::parsePBUSH },
                { "PCOMP", &NastranParser::parsePCOMP },
                { "PDAMP", &NastranParser::parsePDAMP },
                { "PELAS", &NastranParser::parsePELAS },
                { "PGAP", &NastranParser::parsePGAP },
                { "PLOAD", &NastranParser::parsePLOAD },
                { "PLOAD1", &NastranParser::parsePLOAD1 },
                { "PLOAD2", &NastranParser::parsePLOAD2 },
                { "PLOAD4", &NastranParser::parsePLOAD4 },
                { "PROD", &NastranParser::parsePROD },
                { "PLSOLID", &NastranParser::parsePLSOLID },
                { "PSHELL", &NastranParser::parsePSHELL },
                { "PIHEX", &NastranParser::parsePSHELL },
                { "PSOLID", &NastranParser::parsePSOLID },
                { "RBAR", &NastranParser::parseRBAR },
                { "RBAR1", &NastranParser::parseRBAR1 },
                { "RBE2", &NastranParser::parseRBE2 },
                { "RBE3", &NastranParser::parseRBE3 },
                { "RFORCE", &NastranParser::parseRFORCE },
                { "RLOAD1", &NastranParser::parseRLOAD1 },
                { "RLOAD2", &NastranParser::parseRLOAD2 },
                { "SET1", &NastranParser::parseSET1 },
                { "SET3", &NastranParser::parseSET3 },
                { "SLOAD", &NastranParser::parseSLOAD },
                { "SPC", &NastranParser::parseSPC },
                { "SPC1", &NastranParser::parseSPC1 },
                { "SPCD", &NastranParser::parseSPCD },
                { "SPCADD", &NastranParser::parseSPCADD },
                { "SPOINT", &NastranParser::parseSPOINT },
                { "TABDMP1", &NastranParser::parseTABDMP1 },
                { "TABLED1", &NastranParser::parseTABLED1 },
                { "TABLES1", &NastranParser::parseTABLES1 },
                { "TEMP", &NastranParser::parseTEMP },
                { "GRDSET", &NastranParser::parseGRDSET }
        };

const std::unordered_map<std::string, NastranParser::NastranAnalysis> NastranParser::ANALYSIS_BY_LABEL = {
        { "1", NastranAnalysis::STATIC },
        { "101", NastranAnalysis::STATIC },
        { "1,1", NastranAnalysis::STATIC },
        { "STATIC", NastranAnalysis::STATIC },
        { "STATICS", NastranAnalysis::STATIC },
        { "SESTATIC", NastranAnalysis::STATIC },
        { "SESTATICS", NastranAnalysis::STATIC },
        { "3", NastranAnalysis::MODES },
        { "103", NastranAnalysis::MODES },
        { "MODES", NastranAnalysis::MODES },
        { "SEMODES", NastranAnalysis::MODES },
        { "5", NastranAnalysis::BUCKL },
        { "105", NastranAnalysis::BUCKL },
        { "BUCKL", NastranAnalysis::BUCKL },
        { "SEBUCKL", NastranAnalysis::BUCKL },
        { "6", NastranAnalysis::NLSTATIC },
        { "106", NastranAnalysis::NLSTATIC },
        { "NLSTATIC", NastranAnalysis::NLSTATIC },
        { "108", NastranAnalysis::DFREQ },
        { "SEDFREQ", NastranAnalysis::DFREQ },
        { "109", NastranAnalysis::DTRAN },
        { "SEDTRAN", NastranAnalysis::DTRAN },
        { "110", NastranAnalysis::MCEIG },
        { "SEMCEIG", NastranAnalysis::MCEIG },
        { "111", NastranAnalysis::MFREQ },
        { "SEMFREQ", NastranAnalysis::MFREQ },
        { "112", NastranAnalysis::MTRAN },
        { "SEMTRAN", NastranAnalysis::MTRAN },
        { "200", NastranAnalysis::DESOPT },
        { "DESOPT", NastranAnalysis::DESOPT },
};

string NastranParser::parseSubcase(NastranTokenizer& tok, Model& model,
        map<string, string> context) {
    const auto& inputContext = tok.getInputContext();
    int subCaseId = tok.nextInt(true, 0);
    string nextKeyword;
    bool bParseSubcase =true;

    while (bParseSubcase){
        tok.nextLine();
        if (tok.nextSymbolType == NastranTokenizer::SymbolType::SYMBOL_EOF){
            bParseSubcase=false;
            continue;
        }
        nextKeyword = tok.nextString(true,"");
        trim(nextKeyword);boost::to_upper(nextKeyword);
        if (nextKeyword == "SET") {
            addSet(tok, model);
        } else if ((!nextKeyword.empty()) && (nextKeyword != "BEGIN") && (nextKeyword != "SUBCASE")  && (nextKeyword != "SUBCOM")) {
            string line ="";
            string sep="";
            while (tok.nextSymbolType == NastranTokenizer::SymbolType::SYMBOL_FIELD) {
                line+=sep+tok.nextString(true,"");
                sep="_";
            }
            string nextKeywordWithoutOptions = nextKeyword.substr(0, nextKeyword.find('('));
            if (ACCEPTED_CASE_CONTROL_COMMANDS.find(nextKeywordWithoutOptions) == ACCEPTED_CASE_CONTROL_COMMANDS.end()) {
                handleParsingWarning("Unknown case control command: " + nextKeywordWithoutOptions + " in subcase " + to_string(subCaseId), tok, model);
            }
            context[nextKeyword] = line;
            if (model.configuration.logLevel >= LogLevel::TRACE) {
                cout << "Put into context :" << nextKeyword << "=" << line << endl;
            }
        } else {
            bParseSubcase=false;
        }
    }

    try{
        addAnalysis(tok, model, context, subCaseId, inputContext);
    }catch (std::string&){
        return nextKeyword;
    }
    return nextKeyword;
}

string NastranParser::parseSubcom(NastranTokenizer& tok, Model& model,
        map<string, string> context) {
    int subComId = tok.nextInt(true, 0);
    string nextKeyword;
    bool bParseSubcom =true;

    while (bParseSubcom){
        tok.nextLine();
        if (tok.nextSymbolType == NastranTokenizer::SymbolType::SYMBOL_EOF){
            bParseSubcom=false;
            continue;
        }
        nextKeyword = tok.nextString(true,"");
        trim(nextKeyword);boost::to_upper(nextKeyword);
        if ((!nextKeyword.empty()) && (nextKeyword != "BEGIN") && (nextKeyword != "SUBCASE")  && (nextKeyword != "SUBCOM")) {
            string line ="";
            string sep="";
            while (tok.nextSymbolType == NastranTokenizer::SymbolType::SYMBOL_FIELD) {
                line+=sep+tok.nextString(true,"");
                sep="_";
            }
            string nextKeywordWithoutOptions = nextKeyword.substr(0, nextKeyword.find('('));
            if (ACCEPTED_CASE_CONTROL_COMMANDS.find(nextKeywordWithoutOptions) == ACCEPTED_CASE_CONTROL_COMMANDS.end()) {
                handleParsingWarning("Unknown case control command: " + nextKeywordWithoutOptions + " in subcom " + to_string(subComId), tok, model);
            }
            context[nextKeyword] = line;
        } else {
            bParseSubcom=false;
        }
    }

    try{
        addCombinationAnalysis(tok, model, context, subComId);
    }catch (std::string&){
        return nextKeyword;
    }
    return nextKeyword;
}

void NastranParser::addSet(NastranTokenizer& tok, Model& model) {
    string line = trim_copy(tok.currentRawDataLine());
    while (line.back() == ',') {
        tok.nextLine();
        line += trim_copy(tok.currentRawDataLine());
    }
    vector<string> parts;
    split(parts, line, boost::is_any_of("="), boost::algorithm::token_compress_on);
    if (parts.size() == 1) {
        std::size_t equalPos = line.find_last_of(" ");
        if (equalPos == string::npos) {
            handleParsingError("no = nor space in SET should never happen", tok, model);
        }
        parts.clear();
        parts.push_back(line.substr(0, equalPos));
        parts.push_back(line.substr(equalPos + 1));
    } else if (parts.size() >= 3) {
        handleParsingError("multiple = in SET should never happen", tok, model);
    }
    const int setid = stoi(trim_copy(parts[0].substr(4)));
    vector<string> parvalparts;
    set<int> values;
    trim(parts[1]);boost::to_upper(parts[1]);

    //const list<string>& types = {"TRIA3","TRIA6","TRIAR","QUAD4","QUAD8","QUADR","HEXA","PENTA","TETRA"};

    if (boost::starts_with(parts[1],"ALL")) {
        handleParsingWarning("ALL in SET not yet implemented, ignoring", tok, model);
    } else if (boost::starts_with(parts[1],"INCLUDE")
               or boost::starts_with(parts[1],"EXCLUDE")
               or boost::starts_with(parts[1],"ELEMENTS")
               or boost::starts_with(parts[1],"GRID")) {
        handleParsingWarning(parts[1] + " in SET not yet implemented, ignoring", tok, model);
    } else if (boost::algorithm::contains(parts[1],"EXCEPT")) {
        handleParsingWarning("EXCEPT in SET not yet implemented, ignoring", tok, model);
    } else if (not isdigit(parts[1][0])) {
        handleParsingWarning(parts[1] + " in SET not yet implemented, ignoring", tok, model);
    } else {
        split(parvalparts, parts[1], boost::is_any_of(", "), boost::algorithm::token_compress_on);
        for (size_t i = 0;i < parvalparts.size(); i++) {
            if (i + 2 < parvalparts.size() and parvalparts[i+1] == "THRU") {
                for (int j = stoi(parvalparts[i]); j <= stoi(parvalparts[i+2]); j++) {
                    values.insert(j);
                }
                i += 2;
            } else {
                values.insert(stoi(parvalparts[i]));
            }
        }
        const auto& setValue = make_shared<SetValue<int>>(model, values, setid);
        setValue->setInputContext(tok.getInputContext());
        model.add(setValue);
    }
}

void NastranParser::parseExecutiveSection(NastranTokenizer& tok, Model& model,
        map<string, string>& context) {
    bool canContinue = true;
    bool readNewKeyword = true;
    bool subCaseFound = false;
    string keyword = "";

    tok.nextLine();
    keyword = tok.nextString(true, "");
    trim(keyword);boost::to_upper(keyword);

    while (canContinue) {

        try {
            tok.setCurrentKeyword(keyword);

            if (keyword.empty()) {
                tok.skipToNextKeyword();
                canContinue = canContinue && (tok.nextSymbolType != NastranTokenizer::SymbolType::SYMBOL_EOF);
            } else if (keyword.find("BEGIN") != string::npos) {
                canContinue = false;
                if (!subCaseFound) {
                    addAnalysis(tok, model, context, Analysis::NO_ORIGINAL_ID, tok.getInputContext());
                }
            } else if (keyword == "B2GG") {
                // Selects direct input damping matrix or matrices.
                string line;
                while (tok.nextSymbolType == NastranTokenizer::SymbolType::SYMBOL_FIELD) {
                    line += tok.nextString(true,"");
                }
                trim(line);
                /*
             The matrices are additive if multiple matrices are referenced on the B2GG
             command.
             The formats of the name list:
             a. Names without factor.
             Names separated by comma or blank.
             b. Names with factors.
             Each entry in the list consists of a factor followed by a star followed by a
             name. The entries are separated by comma or blank. The factors are real
             numbers. Each name must be with a factor including 1.0.
                 */
                if (line.find_first_of(", *") != std::string::npos) {
                    handleParsingError("complex names not yet implemented", tok, model);
                }
                istringstream iss(line);
                int num = 0;
                if (!(iss >> num).fail()) {
                    handleParsingError("set references not yet implemented " + to_string(num), tok, model);
                }
                /*
                The matrix must be symmetric in form (field 4 on DMIG Bulk Data entry must contain the integer 6).
                */
                const auto& matrix = make_shared<DampingMatrix>(model, MatrixType::SYMMETRIC); // LD : TODO string identifier here
                directMatrixByName.insert({line, matrix->getReference()});
                matrix->setInputContext(tok.getInputContext());
                model.add(matrix);
            } else if (keyword == "CEND") {
                //Nothing to do
                tok.skipToNextKeyword();
            } else if (keyword == "K2GG") {
                // Selects direct input stiffness matrix or matrices.
                string line;
                while (tok.nextSymbolType == NastranTokenizer::SymbolType::SYMBOL_FIELD) {
                    line += tok.nextString();
                }
                trim(line);
                /*
             The matrices are additive if multiple matrices are referenced on the K2GG
             command.
             The formats of the name list:
             a. Names without factor.
             Names separated by comma or blank.
             b. Names with factors.
             Each entry in the list consists of a factor followed by a star followed by a
             name. The entries are separated by comma or blank. The factors are real
             numbers. Each name must be with a factor including 1.0.
                 */
                if (line.find_first_of(", *") != std::string::npos) {
                    handleParsingError("complex names not yet implemented", tok, model);
                }
                istringstream iss(line);
                int num = 0;
                if (!(iss >> num).fail()) {
                    handleParsingError("set references not yet implemented " + to_string(num), tok, model);
                }
                /*
                The matrix must be symmetric in form (field 4 on DMIG Bulk Data entry must contain the integer 6).
                */
                const auto& matrix = make_shared<StiffnessMatrix>(model, MatrixType::SYMMETRIC); // LD : TODO string identifier here
                directMatrixByName.insert({line, matrix->getReference()});
                matrix->setInputContext(tok.getInputContext());
                model.add(matrix);
            } else if (keyword == "M2GG") {
                // Selects direct input mass matrix or matrices.
                string line;
                while (tok.nextSymbolType == NastranTokenizer::SymbolType::SYMBOL_FIELD) {
                    line += tok.nextString();
                }
                trim(line);
                /*
             The matrices are additive if multiple matrices are referenced on the M2GG
             command.
             The formats of the name list:
             a. Names without factor.
             Names separated by comma or blank.
             b. Names with factors.
             Each entry in the list consists of a factor followed by a star followed by a
             name. The entries are separated by comma or blank. The factors are real
             numbers. Each name must be with a factor including 1.0.
                 */
                if (line.find_first_of(", *") != std::string::npos) {
                    handleParsingError("complex names not yet implemented", tok, model);
                }
                istringstream iss(line);
                int num = 0;
                if (!(iss >> num).fail()) {
                    handleParsingError("set references not yet implemented " + to_string(num), tok, model);
                }
                /*
                The matrix must be symmetric in form (field 4 on DMIG Bulk Data entry must contain the integer 6).
                */
                const auto& matrix = make_shared<MassMatrix>(model, MatrixType::SYMMETRIC); // LD : TODO string identifier here

                directMatrixByName.insert({line, matrix->getReference()});
                matrix->setInputContext(tok.getInputContext());
                model.add(matrix);
            } else if (keyword == "SUBCASE") {
                keyword = parseSubcase(tok, model, context);
                readNewKeyword = false;
                subCaseFound = true;
            } else if (keyword == "SUBCOM") {
                keyword = parseSubcom(tok, model, context);
                readNewKeyword = false;
                subCaseFound = true;
            } else if (keyword == "SUBTITLE") {
                string line;
                while (tok.nextSymbolType == NastranTokenizer::SymbolType::SYMBOL_FIELD)
                    line += tok.nextString(true,"");
                model.description = line;
            } else if (keyword == "TITLE") {
                string line;
                while (tok.nextSymbolType == NastranTokenizer::SymbolType::SYMBOL_FIELD)
                    line += tok.nextString(true,"");
                model.title = line;
            } else if (keyword == "SET") {
                addSet(tok, model);
            } else {
                if (tok.nextSymbolType != NastranTokenizer::SymbolType::SYMBOL_FIELD) {
                    handleParsingWarning("Case control parameter " + keyword + ": value cannot be parsed. Line:" + tok.currentRawDataLine(), tok, model);
                    context[keyword] = "";
                } else {
                    vector<string> parts;
                    const auto& currentRawDataLine = tok.currentRawDataLine();
                    split(parts, currentRawDataLine, boost::is_any_of("="), boost::algorithm::token_compress_on);
                    if (parts.size() == 1) {
                        vector<string> parvalparts;
                        split(parvalparts, parts[0], boost::is_any_of(" "), boost::algorithm::token_compress_on);
                        if (parts.size() >= 2) {
                            handleParsingError("multiple spaces in parameter not yet implemented", tok, model);
                        }
                        context[keyword] = parvalparts[1];
                    } else {
                        trim(parts[0]);boost::to_upper(parts[0]);
                        string partWithoutOptions = parts[0].substr(0, parts[0].find('('));
                        if (ACCEPTED_CASE_CONTROL_COMMANDS.find(partWithoutOptions) == ACCEPTED_CASE_CONTROL_COMMANDS.end()) {
                            handleParsingWarning("Unknown case control command: " + partWithoutOptions + " in executive section", tok, model);
                        }
                        context[parts[0]] = parts[1];
                    }
                }
            }

        } catch (std::string&) {
            // Parsing errors are catched by VegaCommandLine.
            // If we are not in strict mode, we dismiss this command and continue, hoping for the best.
            tok.skipToNextKeyword();
            canContinue = canContinue && (tok.nextSymbolType != NastranTokenizer::SymbolType::SYMBOL_EOF);
        }

        if (readNewKeyword) {
            tok.nextLine();
            if (tok.nextSymbolType != NastranTokenizer::SymbolType::SYMBOL_EOF){
                keyword = tok.nextString(true, "");
                trim(keyword);boost::to_upper(keyword);
            }
        } else {
            //new keyword was read by a parsing method
            readNewKeyword = true;
        }

        canContinue = canContinue && (tok.nextSymbolType != NastranTokenizer::SymbolType::SYMBOL_EOF);

    }
}

NastranParser::parseElementFPtr NastranParser::findCmdParser(string keyword) const {
    auto result = PARSE_FUNCTION_BY_KEYWORD.find(keyword);
    if (result != PARSE_FUNCTION_BY_KEYWORD.end()) {
        return result->second;
    } else {
        return nullptr;
    }
}

void NastranParser::parseBULKSection(NastranTokenizer &tok, Model& model) {

    while (tok.nextSymbolType == NastranTokenizer::SymbolType::SYMBOL_KEYWORD) {
        string keyword = tok.nextString(true,"");
        tok.setCurrentKeyword(keyword);
        try{
            auto parser = findCmdParser(keyword);
            if (parser != nullptr) {
                (this->*parser)(tok, model);

            } else if (IGNORED_KEYWORDS.find(keyword) != IGNORED_KEYWORDS.end()) {
                if (model.configuration.logLevel >= LogLevel::TRACE) {
                    cout << "Keyword " << keyword << " ignored." << endl;
                }
                tok.skipToNextKeyword();

            } else if (!keyword.empty()) {
                handleParsingError("Unknown keyword.", tok, model);
                tok.skipToNextKeyword();
            }

            //Warning if there are unparsed fields. Skip the empty ones
            if (!tok.isEmptyUntilNextKeyword()) {
                handleParsingError("Parsing of line not complete:[" + tok.remainingTextUntilNextKeyword()+"]", tok, model);
            }

        } catch (std::string&) {
            // Parsing errors are catched by VegaCommandLine.
            // If we are not in strict mode, we dismiss this command and continue, hoping for the best.
            tok.skipToNextKeyword();
        }
        tok.nextLine();
    }

}

fs::path NastranParser::findModelFile(const string& filename) {
    if (!fs::exists(filename)) {
        throw invalid_argument("Can't find file : " + fs::absolute(filename).string());
    }
    fs::path inputFilePath(filename);
    return inputFilePath;
}

unique_ptr<Model> NastranParser::parse(const ConfigurationParameters& configuration) {
    this->translationMode = configuration.translationMode;
    this->logLevel = configuration.logLevel;

    const string filename = configuration.inputFile;

    fs::path inputFilePath = findModelFile(filename);
    const string modelName = inputFilePath.filename().string();
    unique_ptr<Model> model = make_unique<Model>(modelName, "UNKNOWN", SolverName::NASTRAN,
            configuration.getModelConfiguration());
    map<string, string> executive_section_context;
    const string inputFilePathStr = inputFilePath.string();
    ifstream istream(inputFilePathStr);
    NastranTokenizer tok {istream, logLevel, inputFilePath.string(), this->translationMode};

    if (model->configuration.logLevel >= LogLevel::DEBUG) {
        cout << "Parsing Executive section." << endl;
    }
    parseExecutiveSection(tok, *model, executive_section_context);

    if (model->configuration.logLevel >= LogLevel::DEBUG) {
        cout << "Parsing BULK section." << endl;
    }
    tok.bulkSection();
    parseBULKSection(tok, *model);
    istream.close();

    if (model->configuration.logLevel >= LogLevel::DEBUG) {
        cout << "Parsing finished." << endl;
    }

    return model;
}

string NastranParser::defaultAnalysis() const {
    return ""; // instead of "101": see github #15
}

NastranParser::NastranAnalysis NastranParser::autoSubcaseAnalysis(map<string, string> &context) const {
    for (const auto& contextEntry: context) {
        if (contextEntry.first.find("NLPARM") != string::npos) {
            return NastranAnalysis::NLSTATIC;
        }
    }
    bool hasMethod = false;
    bool hasCMethod = false;
    for (const auto& contextEntry: context) {
        if (contextEntry.first.find("METHOD") != string::npos) {
            hasMethod = true;
            break;
        }
    }
    for (const auto& contextEntry: context) {
        if (contextEntry.first.find("CMETHOD") != string::npos) {
            hasCMethod = true;
            break;
        }
    }
    if (hasCMethod) {
        return NastranAnalysis::MCEIG;
    } else if (hasMethod) {
        for (const auto& contextEntry: context) {
            if (contextEntry.first.find("SDAMPING") != string::npos) {
                return NastranAnalysis::MFREQ;
            }
        }
        return NastranAnalysis::MODES;
    }
    return NastranAnalysis::STATIC;
}

void NastranParser::addAnalysis(NastranTokenizer& tok, Model& model, map<string, string> &context,
        int analysis_id, const InputContext& inputContext) {

    string analysis_str;
    const auto& it0 = context.find("SOL");
    if (it0 != context.end())
        analysis_str = trim_copy(it0->second);
    else
        analysis_str = defaultAnalysis();

    if (analysis_str.empty()) {
      return;
    }

    auto str_entry = ANALYSIS_BY_LABEL.find(analysis_str);
    if (str_entry == ANALYSIS_BY_LABEL.end()) {
        handleParsingError("Analysis " + analysis_str + " Not implemented", tok, model);
    }
    NastranAnalysis analysis_type = str_entry->second;

    shared_ptr<Analysis> previous = nullptr;
    if (not model.analyses.empty()) {
        previous = model.analyses.last();
    }

    if (analysis_type == NastranAnalysis::DESOPT) {
        auto it = context.find("ANALYSIS");
        if (it != context.end()) {
            string analysis_label = trim_copy(it->second);
            cout << it->first << "=" << it->second << endl;
            auto label_entry = ANALYSIS_BY_LABEL.find(analysis_label);
            if (label_entry != ANALYSIS_BY_LABEL.end())
                analysis_type = label_entry->second;
            else {
                analysis_type = autoSubcaseAnalysis(context); // when empty ANALYSIS keyword (happened in SMOT)
            }
        } else
            analysis_type = autoSubcaseAnalysis(context); // no analysis keyword
    }

    shared_ptr<Analysis> analysis = nullptr;

    // Finding label
    string labelAnalysis="";
    if (analysis_id != Analysis::NO_ORIGINAL_ID) {
        labelAnalysis += "Analysis_" + to_string(analysis_id);
    }
    const auto& it1 = context.find("LABEL");
    if (it1 != context.end())
        labelAnalysis = trim_copy(it1->second);
    else {
        auto commentEntry = tok.labelByCommentTypeAndId.find({NastranTokenizer::CommentType::LOADSTEP, analysis_id});
        if (commentEntry != tok.labelByCommentTypeAndId.end())
            labelAnalysis = commentEntry->second;
    }

    if (analysis_type == NastranAnalysis::STATIC or (analysis_type == NastranAnalysis::BUCKL and previous == nullptr)) {

        analysis = make_shared<LinearMecaStat>(model, labelAnalysis, analysis_id);

    } else if (analysis_type == NastranAnalysis::BUCKL and previous != nullptr) {

        map<string, string>::iterator it;
        int frequency_search_sid = 0;
        for (it = context.begin(); it != context.end(); it++) {
            if (it->first.find("METHOD") != string::npos) {
                frequency_search_sid = stoi(it->second);
                break;
            }
        }
        if (it == context.end())
            handleParsingError("METHOD not found for linear buckling analysis", tok, model);

        analysis = make_shared<LinearBuckling>(model, Reference<ObjectiveSet>{ObjectiveSet::Type::METHOD, frequency_search_sid}, labelAnalysis, analysis_id);

    } else if (analysis_type == NastranAnalysis::MCEIG) {

        map<string, string>::iterator it;
        int frequency_search_sid = 0;
        for (it = context.begin(); it != context.end(); it++) {
            if (it->first.find("METHOD") != string::npos) {
                frequency_search_sid = stoi(it->second);
                break;
            }
        }
        if (it == context.end())
            handleParsingError("METHOD not found for linear complex modal analysis", tok, model);
        int complex_method_sid = 0;
        for (it = context.begin(); it != context.end(); it++) {
            if (it->first.find("CMETHOD") != string::npos) {
                complex_method_sid = stoi(it->second);
                break;
            }
        }
        if (it == context.end())
            handleParsingError("CMETHOD not found for linear complex modal analysis", tok, model);

        analysis = make_shared<LinearModalComplex>(model, Reference<ObjectiveSet>{ObjectiveSet::Type::METHOD, frequency_search_sid}, Reference<ObjectiveSet>{ObjectiveSet::Type::CMETHOD, complex_method_sid}, labelAnalysis, analysis_id);

    } else if (analysis_type == NastranAnalysis::MODES) {

        map<string, string>::iterator it;
        int frequency_search_sid = 0;
        for (it = context.begin(); it != context.end(); it++) {
            if (it->first.find("METHOD") != string::npos) {
                frequency_search_sid = stoi(it->second);
                break;
            }
        }
        if (it == context.end())
            handleParsingError("METHOD not found for linear modal analysis", tok, model);

        analysis = make_shared<LinearModal>(model, Reference<ObjectiveSet>{ObjectiveSet::Type::METHOD, frequency_search_sid}, labelAnalysis, analysis_id);

    } else if (analysis_type == NastranAnalysis::NLSTATIC) {

        auto itparam = context.find("NLPARM");
        int strategy_sid = 0;
        if (itparam == context.end())
            handleParsingError("NLPARM not found for non linear analysis", tok, model);
        else
            strategy_sid = stoi(itparam->second);

        analysis = make_shared<NonLinearMecaStat>(model, Reference<ObjectiveSet>{ObjectiveSet::Type::NONLINEAR_STRATEGY, strategy_sid}, labelAnalysis, analysis_id);


    } else if (analysis_type == NastranAnalysis::DFREQ) {
        int frequency_excit_sid = 0;
        map<string, string>::iterator it;
        for (it = context.begin(); it != context.end(); it++) {
            if (it->first.find("FREQ") == 0)
                frequency_excit_sid = stoi(it->second);
        }
        if (frequency_excit_sid == 0)
            handleParsingError("FREQ not found for linear dynamic direct frequency analysis", tok, model);
        analysis = make_shared<LinearDynaDirectFreq>(model, Reference<ObjectiveSet>{ObjectiveSet::Type::FREQ, frequency_excit_sid}, labelAnalysis, analysis_id);

    } else if (analysis_type == NastranAnalysis::MFREQ) {

        int frequency_search_sid = 0;
        int modal_damping_sid = 0;
        int frequency_excit_sid = 0;

        map<string, string>::iterator it;
        for (it = context.begin(); it != context.end(); it++) {
            if (it->first.find("METHOD") == 0)
                frequency_search_sid = stoi(it->second);
            if (it->first.find("SDAMPING") == 0)
                modal_damping_sid = stoi(it->second);
            if (it->first.find("FREQ") == 0)
                frequency_excit_sid = stoi(it->second);
        }

//        if (modal_damping_sid == 0)
//            handleParsingError("SDAMPING not found for linear dynamic modal frequency analysis", tok, model);
        if (frequency_excit_sid == 0)
            handleParsingError("FREQ not found for linear dynamic modal frequency analysis", tok, model);

        bool residual_vector = false;
        string search_for = "RESVEC";
        it = context.lower_bound(search_for);
        if (it != context.end() and it->first.compare(0, search_for.size(), search_for) == 0 and it->second == "YES")
            residual_vector = true;

        if (frequency_search_sid == 0)
            handleParsingError("METHOD not found for linear dynamic modal frequency analysis", tok, model);

        analysis = make_shared<LinearDynaModalFreq>(model, Reference<ObjectiveSet>{ObjectiveSet::Type::METHOD, frequency_search_sid},
                                                    Reference<ObjectiveSet>{ObjectiveSet::Type::SDAMP, modal_damping_sid},
                                                    Reference<ObjectiveSet>{ObjectiveSet::Type::FREQ, frequency_excit_sid},
                                                    residual_vector, labelAnalysis, analysis_id);

    } else {
        handleParsingError("Analysis " + analysis_str + " Not implemented", tok, model);
    }

    for (const auto& contextPair : context) {
        string key = contextPair.first;
        int id = 0;
        string value = boost::algorithm::trim_copy(contextPair.second);
        if (not value.empty() and value.find_first_not_of("0123456789") == std::string::npos) {
            id = stoi(value); // Avoid exception catch to simplify debugging
        }
        vector<string> options;
        boost::split(options, key, boost::is_any_of("(, )"), boost::token_compress_on);
        options.erase( remove_if( options.begin(), options.end(), [](string option){return option.size() == 0;} ));
        if (options.size() >= 2) {
            key = options[0];
            options.erase(options.begin());
        }
        if (!key.compare(0, 3, "SPC")) {
            Reference<ConstraintSet> constraintReference(ConstraintSet::Type::SPC, id);
            analysis->add(constraintReference);
            if (!model.find(constraintReference)) { // constraintSet is added in the model if not found in the model
                const auto& constraintSet = make_shared<ConstraintSet>(model, ConstraintSet::Type::SPC, id);
                constraintSet->setInputContext(tok.getInputContext());
                model.add(constraintSet);
            }
        } else if (!key.compare(0, 3, "MPC")) {
            Reference<ConstraintSet> constraintReference(ConstraintSet::Type::MPC, id);
            analysis->add(constraintReference);
            if (!model.find(constraintReference)) { // constraintSet is added in the model if not found in the model
                const auto& constraintSet = make_shared<ConstraintSet>(model, ConstraintSet::Type::MPC, id);
                constraintSet->setInputContext(tok.getInputContext());
                model.add(constraintSet);
            }
        } else if (!key.compare(0, 7, "LOADSET")) {
            Reference<LoadSet> loadsetReference(LoadSet::Type::LOADSET, id);
            analysis->add(loadsetReference);
            if (!model.find(loadsetReference)) { // loadSet is added in the model if not found in the model
                const auto& loadSet = make_shared<LoadSet>(model, LoadSet::Type::LOADSET, id);
                loadSet->setInputContext(tok.getInputContext());
                model.add(loadSet);
            }
        } else if (!key.compare(0, 4, "LOAD")) {
            Reference<LoadSet> loadsetReference(LoadSet::Type::LOAD, id);
            analysis->add(loadsetReference);
            if (!model.find(loadsetReference)) { // loadSet is added in the model if not found in the model
                const auto& loadSet = make_shared<LoadSet>(model, LoadSet::Type::LOAD, id);
                loadSet->setInputContext(tok.getInputContext());
                model.add(loadSet);
            }
        } else if (!key.compare(0, 5, "DLOAD")) {
            Reference<LoadSet> loadsetReference(LoadSet::Type::DLOAD, id);
            analysis->add(loadsetReference);
            if (!model.find(loadsetReference)) { // loadSet is added in the model if not found in the model
                const auto& loadSet = make_shared<LoadSet>(model, LoadSet::Type::DLOAD, id);
                loadSet->setInputContext(tok.getInputContext());
                model.add(loadSet);
            }
        } else if (!key.compare(0, 4, "BCONTACT")) {
            Reference<ConstraintSet> constraintReference(ConstraintSet::Type::CONTACT, id);
            analysis->add(constraintReference);
            if (!model.find(constraintReference)) { // constraintSet is added in the model if not found in the model
                const auto& constraintSet = make_shared<ConstraintSet>(model, ConstraintSet::Type::CONTACT, id);
                constraintSet->setInputContext(tok.getInputContext());
                model.add(constraintSet);
            }
        } else if (!key.compare(0, 4, "DISP")) {
            if (id != 0) {
                const Reference<ObjectiveSet>& objectiveSetReference{ObjectiveSet::Type::DISP, id};
                analysis->add(objectiveSetReference);
                auto objectiveSet = model.find(objectiveSetReference);
                if (model.find(objectiveSetReference) == nullptr) {
                    objectiveSet = make_shared<ObjectiveSet>(model, ObjectiveSet::Type::DISP, id);
                    objectiveSet->setInputContext(tok.getInputContext());
                    model.add(objectiveSet);
                }
                auto nodalOutput = make_shared<NodalDisplacementOutput>(model, objectiveSet, make_shared<Reference<NamedValue>>(Value::Type::SET, id));
                for (const auto& option:options) {
                    if (option == "PHASE") {
                        nodalOutput->complexOutput = NodalDisplacementOutput::ComplexOutputType::PHASE_MAGNITUDE;
                    }
                }
                nodalOutput->setInputContext(tok.getInputContext());
                model.add(nodalOutput);
            }
        } else if (!key.compare(0, 4, "OFREQUENCY")) {
            if (id != 0) {
                Reference<ObjectiveSet> objectiveSetReference(ObjectiveSet::Type::OFREQ, id);
                analysis->add(objectiveSetReference);
                auto objectiveSet = model.find(objectiveSetReference);
                if (model.find(objectiveSetReference) == nullptr) {
                    objectiveSet = make_shared<ObjectiveSet>(model, ObjectiveSet::Type::OFREQ, id);
                    objectiveSet->setInputContext(tok.getInputContext());
                    model.add(objectiveSet);
                }
                auto frequencyOutput = make_shared<FrequencyOutput>(model, objectiveSet, make_shared<Reference<NamedValue>>(Value::Type::SET, id));
                frequencyOutput->setInputContext(tok.getInputContext());
                model.add(frequencyOutput);
            }
        } else if (!key.compare(0, 4, "STRESS")) {
            if (id != 0) {
                Reference<ObjectiveSet> objectiveSetReference(ObjectiveSet::Type::STRESS, id);
                analysis->add(objectiveSetReference);
                auto objectiveSet = model.find(objectiveSetReference);
                if (model.find(objectiveSetReference) == nullptr) {
                    objectiveSet = make_shared<ObjectiveSet>(model, ObjectiveSet::Type::STRESS, id);
                    objectiveSet->setInputContext(tok.getInputContext());
                    model.add(objectiveSet);
                }
                shared_ptr<VonMisesStressOutput> stressOutput = nullptr;
                for (const auto& option:options) {
                    if (option == "VMIS") {
                        stressOutput = make_shared<VonMisesStressOutput>(model, objectiveSet, make_shared<Reference<NamedValue>>(Value::Type::SET, id));
                    }
                }
                stressOutput->setInputContext(tok.getInputContext());
                model.add(stressOutput);
            }
        }
    }

    /*
         The subcase structure provides a unique means of changing loads,
         boundary conditions, and solution methods by making selections from the Bulk Data.
         Confining the discussion to SOL 66 (or 106) and SOL 99 (or 129),
         loads and solution methods may change from subcase to subcase
         on an incremental basis. However, constraints can be changed
         from subcase to subcase only in the static solution sequence.
         As a result, the subcase structure determines a sequence of loading
         and constraint paths in a nonlinear analysis.
         The subcase structure also allows the user to selectand change
         output requests for printout, plot, etc., by specifying set numbers with keywords.
         Any selections made above the subcase specifications are applicable to all the subcases.
         Selectionsmade in an individual subcase supersede the selections made above the subcases.
         */
    analysis->previousAnalysis = previous;
    analysis->setInputContext(inputContext);
    model.add(analysis);

}

void NastranParser::addCombinationAnalysis(NastranTokenizer& tok, Model& model, map<string, string> &context,
        int analysis_id) {
    // Finding label
    string labelAnalysis = "";
    if (analysis_id != Analysis::NO_ORIGINAL_ID) {
        labelAnalysis += "Analysis_" + to_string(analysis_id);
    }
    const auto& label = context.find("LABEL");
    if (label != context.end())
        labelAnalysis = trim_copy(label->second);
    else {
        auto commentEntry = tok.labelByCommentTypeAndId.find({NastranTokenizer::CommentType::LOADSTEP, analysis_id});
        if (commentEntry != tok.labelByCommentTypeAndId.end())
            labelAnalysis = commentEntry->second;
    }
    const auto& combination = make_shared<Combination>(model, labelAnalysis, analysis_id);

    // A SUBSEQ command must follow this command.
    const auto& subseq = context.find("SUBSEQ");
    vector<string> coefStrings(model.analyses.size());
    boost::split(coefStrings,subseq->second,boost::is_any_of(","));
    for (size_t i = 0; i < coefStrings.size(); i++) {
        auto it = model.analyses.begin();
        advance(it, i);
        const auto& analysis = *it;
        double coef = stod(coefStrings[i++]);
        if (model.configuration.logLevel >= LogLevel::TRACE) {
            cout << "Assigning analysis " << *analysis << " to combination " << *combination << " with coefficient " << to_string(coef) << endl;
        }
        combination->coefByAnalysis[analysis->getReference()] = coef;
    }
    combination->setInputContext(tok.getInputContext());
    model.add(combination);
}

void NastranParser::parseBCBODY(NastranTokenizer& tok, Model& model) {
    int bid = tok.nextInt();
    string dim = tok.nextString(true,"3D");
    if (dim != "3D")
        handleParsingError("BCBODY dim only implemented in 3D case (yet)", tok, model);
    string behav = tok.nextString(true,"DEFORM");
    if (behav != "DEFORM")
        handleParsingError("BCBODY behav only implemented in DEFORM case (yet)", tok, model);
    int bsid = tok.nextInt();
    if (not tok.isEmptyUntilNextKeyword()) {
        handleParsingError("BCBODY optional fields not yet handled", tok, model);
    }
    const auto& body = make_shared<ContactBody>(model, Reference<Target>(Target::Type::BOUNDARY_SURFACE, bsid), bid);
    body->setInputContext(tok.getInputContext());
    model.add(body);
}

void NastranParser::parseBCGRID(NastranTokenizer& tok, Model& model) {
    int id = tok.nextInt();
    const auto& nodecloud = make_shared<BoundaryNodeCloud>(model, tok.nextInts(), id);
    nodecloud->setInputContext(tok.getInputContext());
    model.add(nodecloud);
}

void NastranParser::parseBLSEG(NastranTokenizer& tok, Model& model) {
    int id = tok.nextInt();
    const auto& nodeline = make_shared<BoundaryNodeLine>(model, tok.nextInts(), id);
    nodeline->setInputContext(tok.getInputContext());
    model.add(nodeline);
}

void NastranParser::parseBSSEG(NastranTokenizer& tok, Model& model) {
    int id = tok.nextInt();
    const auto& surface = make_shared<BoundaryNodeSurface>(model, tok.nextInts(), id);
    surface->setInputContext(tok.getInputContext());
    model.add(surface);
}

void NastranParser::parseBSURF(NastranTokenizer& tok, Model& model) {
    int id = tok.nextInt();
    string gname = "BSURF_" + to_string(id);
    auto gsurf = model.mesh.createCellGroup(gname, CellGroup::NO_ORIGINAL_ID, "BSURF");
    gsurf->addCellIds(tok.nextInts());
    const auto& surface = make_shared<BoundarySurface>(model, id);
    surface->add(*gsurf);
    surface->setInputContext(tok.getInputContext());
    model.add(surface);
}

void NastranParser::parseBFRIC(NastranTokenizer& tok, Model& model) {
    int fid = tok.nextInt();
    tok.skip(2);
    double fstif = tok.nextDouble();
    if (not tok.isEmptyUntilNextKeyword()) {
        handleParsingError("BFRIC optional fields not yet handled", tok, model);
    }
    const auto& friction = make_shared<ScalarValue<double>>(model, fstif, fid);
    friction->setInputContext(tok.getInputContext());
    model.add(friction);
}

void NastranParser::parseBCTABLE(NastranTokenizer& tok, Model& model) {
    int set_id = tok.nextInt();
    Reference<ConstraintSet> constraintSetReference(ConstraintSet::Type::CONTACT, set_id);
    if (!model.find(constraintSetReference)) {
        const auto& constraintSet = make_shared<ConstraintSet>(model, ConstraintSet::Type::CONTACT, set_id);
        constraintSet->setInputContext(tok.getInputContext());
        model.add(constraintSet);
    }
    int idslave = tok.nextInt(true,0);
    int idmaster = tok.nextInt(true,0);
    if (idslave != 0 and idmaster != 0) {
        const auto& zone = make_shared<ZoneContact>(model, Reference<Target>(Target::Type::BOUNDARY_SURFACE, idmaster), Reference<Target>(Target::Type::BOUNDARY_SURFACE, idslave));
        zone->setInputContext(tok.getInputContext());
        model.add(zone);
        model.addConstraintIntoConstraintSet(*zone, constraintSetReference);
    }
    int ngroup = tok.nextInt(true, 0);
    for (int igroup = 0; igroup < ngroup; igroup++) {
        tok.skipToNotEmpty();
        if (boost::to_upper_copy(tok.nextString()) != "SLAVE") {
            handleParsingError("BCTABLE unknown field before SLAVE line", tok, model);
        }
        int idslave2 = tok.nextInt(true,0);
        tok.skipToNotEmpty();
        if (boost::to_upper_copy(tok.nextString()) != "MASTERS") {
            handleParsingError("BCTABLE unknown field before MASTERS line", tok, model);
        }
        int idmaster2 = tok.nextInt(true,0);
        if (not (tok.isNextEmpty() or tok.isEmptyUntilNextKeyword())) {
            handleParsingError("BCTABLE with more than one master on MASTERS line", tok, model);
        }
        const auto& zone = make_shared<ZoneContact>(model, Reference<Target>(Target::Type::BOUNDARY_SURFACE, idmaster2), Reference<Target>(Target::Type::BOUNDARY_SURFACE, idslave2));
        zone->setInputContext(tok.getInputContext());
        model.add(zone);
        model.addConstraintIntoConstraintSet(*zone, constraintSetReference);
    }
    if (not tok.isEmptyUntilNextKeyword()) {
        handleParsingError("BCBODY optional fields not yet handled", tok, model);
    }
}

void NastranParser::parseBCONP(NastranTokenizer& tok, Model& model) {
    int id = tok.nextInt();
    int slaveId = tok.nextInt();
    int masterId = tok.nextInt();
    tok.skip(1);
    double sfact = tok.nextDouble(true, 1.0);
    UNUSEDV(sfact);
    int fricid = tok.nextInt();
    int ptype = tok.nextInt(true, 1);
    UNUSEDV(ptype);
    int cid = tok.nextInt();
    UNUSEDV(cid);

    if (not tok.isEmptyUntilNextKeyword()) {
        handleParsingError("BCONP optional fields not yet handled", tok, model);
    }
    const auto& slide = make_shared<SlideContact>(model, Reference<NamedValue>(Value::Type::SCALAR, fricid),
                       Reference<Target>(Target::Type::BOUNDARY_NODELINE, masterId),
                       Reference<Target>(Target::Type::BOUNDARY_NODELINE, slaveId), id);
    slide->setInputContext(tok.getInputContext());
    model.add(slide);
    model.addConstraintIntoConstraintSet(*slide, *model.commonConstraintSet);
}

void NastranParser::parseBSCONP(NastranTokenizer& tok, Model& model) {
    int id = tok.nextInt();
    int slaveId = tok.nextInt();
    int masterId = tok.nextInt();

    if (not tok.isEmptyUntilNextKeyword()) {
        handleParsingError("BCONP optional fields not yet handled", tok, model);
    }
    const auto& surface = make_shared<SurfaceContact>(model,Reference<Target>(Target::Type::BOUNDARY_NODESURFACE, masterId),
                           Reference<Target>(Target::Type::BOUNDARY_NODESURFACE, slaveId),
                           id);
    surface->setInputContext(tok.getInputContext());
    model.add(surface);
    model.addConstraintIntoConstraintSet(*surface, *model.commonConstraintSet);
}

void NastranParser::parseCONROD(NastranTokenizer& tok, Model& model) {
    int eid = tok.nextInt();
    int g1 = tok.nextInt();
    int g2 = tok.nextInt();
    int mid = tok.nextInt();
    double a = tok.nextDouble();
    double j = tok.nextDouble(true, 0.0);
    double c = tok.nextDouble(true, 0.0);
    if (!is_equal(c, 0)) {
        handleParsingWarning("Stress coefficient (C) not supported and dismissed.", tok, model);
    }
    double nsm = tok.nextDouble(true, 0.0);
    model.mesh.addCell(eid, CellType::SEG2, {g1, g2});
    const auto& genericSectionBeam = make_shared<GenericSectionBeam>(model, a, 0, 0, j, 0, 0, GenericSectionBeam::BeamModel::TRUSS, nsm);
    genericSectionBeam->assignMaterial(Reference<Material>(Material::Type::MATERIAL,mid));
    shared_ptr<CellGroup> cellGroup = model.mesh.createCellGroup("CONROD_" + to_string(eid), Group::NO_ORIGINAL_ID, "CONROD");
    cellGroup->addCellId(eid);
    genericSectionBeam->add(*cellGroup);
    genericSectionBeam->setInputContext(tok.getInputContext());
    model.add(genericSectionBeam);
}

void NastranParser::parseCONM1(NastranTokenizer& tok, Model& model) {
    int eid = tok.nextInt();
    int g = tok.nextInt(); // Grid point identification number
    int ci = tok.nextInt(true, 0);
    if (ci == -1) {
        handleParsingWarning("coordinate system CID=-1 not supported and dismissed.", tok, model);
        ci = 0;
    }
    int cpos = ci == 0 ? CoordinateSystem::GLOBAL_COORDINATE_SYSTEM_ID : model.mesh.findOrReserveCoordinateSystem(Reference<CoordinateSystem>(CoordinateSystem::Type::ABSOLUTE, ci));
    int cellPosition = model.mesh.addCell(eid, CellType::POINT1, { g }, false, cpos);
    auto mnodale = model.mesh.createCellGroup("CONM1_" + to_string(eid), CellGroup::NO_ORIGINAL_ID, "NODAL MASS");
    mnodale->addCellPosition(cellPosition);

    const auto& nodalMassMatrix = make_shared<DiscretePoint>(model, MatrixType::SYMMETRIC, eid);

    for (dof_int row = 0; row < 5; row++) {
        const DOF rowdof = DOF::findByPosition(row);
        for (dof_int col = 0; col <= row; col++) {
            const DOF coldof = DOF::findByPosition(col);
            if (!tok.isNextDouble()) {
                break;
            }
            nodalMassMatrix->addMass(rowdof, coldof, tok.nextDouble(true, 0.0));
        }
    }

    nodalMassMatrix->add(*mnodale);
    nodalMassMatrix->setInputContext(tok.getInputContext());
    model.add(nodalMassMatrix);
}

void NastranParser::parseCONM2(NastranTokenizer& tok, Model& model) {
    int eid = tok.nextInt();
    int g = tok.nextInt(); // Grid point identification number
    int ci = tok.nextInt(true, 0);
    if (ci == -1) {
        handleParsingWarning("coordinate system CID=-1 not supported and dismissed.", tok, model);
        ci = 0;
    }
    const double mass = tok.nextDouble();
    const double x1 = tok.nextDouble(true, 0.0);
    const double x2 = tok.nextDouble(true, 0.0);
    const double x3 = tok.nextDouble(true, 0.0);

    // User defined CID is only important if the offset is not null
    if ((ci != 0) and (not is_zero(x1) or not is_zero(x2) or not is_zero(x3))) {
        handleParsingWarning("coordinate system CID not supported and dismissed.", tok, model);
        ci = 0;
    }
    tok.skip(1);

    const double i11 = tok.nextDouble(true, 0.0);
    const double i21 = tok.nextDouble(true, 0.0);
    const double i22 = tok.nextDouble(true, 0.0);
    const double i31 = tok.nextDouble(true, 0.0);
    const double i32 = tok.nextDouble(true, 0.0);
    const double i33 = tok.nextDouble(true, 0.0);

    const auto& nodalMass = make_shared<NodalMass>(model, mass, i11, i22, i33, -i21, -i31, -i32, x1, x2, x3, eid);

    int cpos = ci == 0 ? CoordinateSystem::GLOBAL_COORDINATE_SYSTEM_ID : model.mesh.findOrReserveCoordinateSystem(Reference<CoordinateSystem>(CoordinateSystem::Type::ABSOLUTE, ci));
    int cellPosition = model.mesh.addCell(eid, CellType::POINT1, { g }, false, cpos);
    auto mnodale = model.mesh.createCellGroup("CONM2_" + to_string(eid), CellGroup::NO_ORIGINAL_ID, "NODAL MASS");
    mnodale->addCellPosition(cellPosition);
    nodalMass->add(*mnodale);
    nodalMass->setInputContext(tok.getInputContext());
    model.add(nodalMass);
}

void NastranParser::parseCORD1R(NastranTokenizer& tok, Model& model) {

    while (tok.isNextInt()) {
        int cid = tok.nextInt();
        int nA  = tok.nextInt();
        int nB  = tok.nextInt();
        int nC  = tok.nextInt();
        CartesianCoordinateSystem coordinateSystem(model.mesh, nA, nB, nC, CoordinateSystem::GLOBAL_COORDINATE_SYSTEM, cid);
        model.mesh.add(coordinateSystem);
        }
}

void NastranParser::parseCORD2C(NastranTokenizer& tok, Model& model) {
    int cid = tok.nextInt();
    //reference coordinate system 0 for global.
    int rid = tok.nextInt(true, CoordinateSystem::GLOBAL_COORDINATE_SYSTEM_ID);

    double coor[3];
    VectorialValue vect[3];
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++)
            coor[j] = tok.nextDouble(true,0.0);
        vect[i] = VectorialValue(coor[0], coor[1], coor[2]);
    }

    VectorialValue ez = vect[1] - vect[0];
    VectorialValue ex = (vect[2] - vect[0]).orthonormalized(ez);
    VectorialValue ey = ez.cross(ex);

    CylindricalCoordinateSystem coordinateSystem(model.mesh, vect[0], ex, ey, Reference<CoordinateSystem>(CoordinateSystem::Type::ABSOLUTE, rid), cid);
    model.mesh.add(coordinateSystem);
}

void NastranParser::parseCORD2R(NastranTokenizer& tok, Model& model) {
    int cid = tok.nextInt();
    int rid = tok.nextInt(true, CoordinateSystem::GLOBAL_COORDINATE_SYSTEM_ID);
    double coor[3];
    VectorialValue vect[3];
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++)
            coor[j] = tok.nextDouble(true, 0.0);
        vect[i] = VectorialValue(coor[0], coor[1], coor[2]);
    }

    VectorialValue ez = vect[1] - vect[0];
    VectorialValue ex = (vect[2] - vect[0]).orthonormalized(ez);
    VectorialValue ey = ez.cross(ex);

    CartesianCoordinateSystem coordinateSystem(model.mesh, vect[0], ex, ey, Reference<CoordinateSystem>(CoordinateSystem::Type::ABSOLUTE, rid), cid);
    model.mesh.add(coordinateSystem);
}

void NastranParser::parseCORD2S(NastranTokenizer& tok, Model& model) {
    int cid = tok.nextInt();
    //reference coordinate system 0 for global.
    int rid = tok.nextInt(true, CoordinateSystem::GLOBAL_COORDINATE_SYSTEM_ID);

    double coor[3];
    VectorialValue vect[3];
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++)
            coor[j] = tok.nextDouble(true,0.0);
        vect[i] = VectorialValue(coor[0], coor[1], coor[2]);
    }

    VectorialValue ez = vect[1] - vect[0];
    VectorialValue ex = (vect[2] - vect[0]).orthonormalized(ez);
    VectorialValue ey = ez.cross(ex);

    SphericalCoordinateSystem coordinateSystem(model.mesh, vect[0], ex, ey, Reference<CoordinateSystem>(CoordinateSystem::Type::ABSOLUTE, rid), cid);
    model.mesh.add(coordinateSystem);
}

void NastranParser::parseCRIGD1(NastranTokenizer& tok, Model& model) {
    int original_id = tok.nextInt();
    if (tok.isNextEmpty()) {
        tok.skip(1);
    } else {
        handleParsingWarning("CRIGD1 Format not respected (NASA manual v16)", tok, model);
    }
    int masterId = tok.nextInt();
    const auto& qrc = make_shared<RigidConstraint>(model, masterId, original_id);
    while (tok.isNextInt()) {
        qrc->addSlave(tok.nextInt());
    }
    qrc->setInputContext(tok.getInputContext());
    model.add(qrc);
    model.addConstraintIntoConstraintSet(*qrc, *model.commonConstraintSet);
}

void NastranParser::parseDAREA(NastranTokenizer& tok, Model& model) {
    int loadset_id = tok.nextInt();
    const auto& loadSet = model.getOrCreateLoadSet(loadset_id, LoadSet::Type::EXCITEID);
    while (tok.isNextInt()) {
        int node_id = tok.nextInt();
        int ci = tok.nextInt(true, 123456);
        double ai = tok.nextDouble();

        const DOFS& dofs = DOFS::nastranCodeToDOFS(ci);
        double tx = dofs.contains(DOF::DX) ? ai : 0;
        double ty = dofs.contains(DOF::DY) ? ai : 0;
        double tz = dofs.contains(DOF::DZ) ? ai : 0;
        double rx = dofs.contains(DOF::RX) ? ai : 0;
        double ry = dofs.contains(DOF::RY) ? ai : 0;
        double rz = dofs.contains(DOF::RZ) ? ai : 0;

        const auto& force1 = make_shared<NodalForce>(model, loadSet, tx, ty, tz, rx, ry, rz, Loading::NO_ORIGINAL_ID);
        force1->addNodeId(node_id);
        force1->setInputContext(tok.getInputContext());
        model.add(force1);
    }
}

//TODO: Delay should not be a DynaPhase object
//TODO: But I guess this will be enough for now
void NastranParser::parseDELAY(NastranTokenizer& tok, Model& model) {
    int original_id = tok.nextInt();
    int p = tok.nextInt(true);
    if (p!=Globals::UNAVAILABLE_INT){
        handleParsingWarning("Point identification (P) ignored and dismissed.", tok, model);
    }
    int c = tok.nextInt(true);
    if (c!=Globals::UNAVAILABLE_INT){
        handleParsingWarning("Component number (C) ignored and dismissed.", tok, model);
    }
    double delay = tok.nextDouble();

    if (!tok.isEmptyUntilNextKeyword()){
        handleParsingWarning("Second dynamic load phase dismissed.", tok, model);
    }
    const auto& dynaphase = make_shared<DynaPhase>(model, -2*M_PI*delay, original_id);
    dynaphase->setInputContext(tok.getInputContext());
    model.add(dynaphase);
}



void NastranParser::parseDLOAD(NastranTokenizer& tok, Model& model) {
    int loadset_id = tok.nextInt();
    shared_ptr<LoadSet> loadSetMaster = model.getOrCreateLoadSet(loadset_id, LoadSet::Type::DLOAD);

    double S = tok.nextDouble(true, 1);
    while (tok.isNextDouble()) {
        double scale = tok.nextDouble(true, 1);
        int rload2_id = tok.nextInt();
        Reference<LoadSet> loadSetReference(LoadSet::Type::DLOAD, rload2_id);
        loadSetMaster->embedded_loadsets.push_back({loadSetReference, S * scale});
    }
}

void NastranParser::parseDMIG(NastranTokenizer& tok, Model& model) {
    string name = tok.nextString();
    if (name == "UACCEL") {
        handleParsingWarning("UACCEL not supported and dismissed.", tok, model);
        tok.skipToNextKeyword();
        return;
    }
    if (name == "CDSHUT") {
        handleParsingWarning("CDSHUT is ignored.", tok, model);
        tok.skipToNextKeyword();
        return; // currently ignored, see CDPCH
    }

    auto it = directMatrixByName.find(name);

    int headerIndicator = tok.nextInt();
    if (headerIndicator == 0) {

        // If the matrix doesn't exist, it means it's not used by the model.
        // It's often the case on industrial cases, when various matrices are written in the same file, but only one is used.
        if (it == directMatrixByName.end()) {
            handleParsingWarning("Matrix "+name+" is not used by the model and ignored.", tok, model);
            tok.skipToNextKeyword();
            return;
        }

        //  Field 3 of the header entry must contain an integer 0.
        int ifo = tok.nextInt();
        if (ifo != 6) {
            handleParsingError("Non-symmetric DMIG not yet implemented.", tok, model);
        }
        int tin = tok.nextInt();
        if (tin != 1 && tin != 2) {
            handleParsingError("Non-real or non-single precision DMIG not yet implemented", tok, model);
        }
        int tout = tok.nextInt(true, 0);
        if (tout != 0) {
            handleParsingError("TOUT in DMIG not yet implemented", tok, model);
        }
        int polar = tok.nextInt(true, 0);
        if (polar != 0) {
            handleParsingError("POLAR in DMIG not yet implemented", tok, model);
        }

        tok.skip(1);
        int ncol = tok.nextInt(true,0); // NCOL is not used for now
        UNUSEDV(ncol);
        return;
    }else{
        // Matrix doesn't exists, we skip it (quietly, because the warning message was already displayed once).
        if (it == directMatrixByName.end()) {
            tok.skipToNextKeyword();
            return;
        }
    }

    const auto& matrix = static_pointer_cast<MatrixElement>(model.find(it->second));

    int gj = headerIndicator;
    int cj = tok.nextInt();
    DOF dofj = *(DOFS::nastranCodeToDOFS(cj).begin());
    tok.skip(1);
    while (tok.isNextInt()) {
        int g1 = tok.nextInt();
        int c1 = tok.nextInt();
        DOF dof1 = *(DOFS::nastranCodeToDOFS(c1).begin());
        double a1 = tok.nextDouble();
        tok.skip(1);
        matrix->addComponent(gj, dofj, g1, dof1, a1);
    }
}

void NastranParser::parseDPHASE(NastranTokenizer& tok, Model& model) {
    int original_id = tok.nextInt();
    int p = tok.nextInt(true);
    if (p!=Globals::UNAVAILABLE_INT){
        handleParsingWarning("Point identification (P) ignored and dismissed.", tok, model);
    }
    int c = tok.nextInt(true);
    if (c!=Globals::UNAVAILABLE_INT){
        handleParsingWarning("Component number (C) ignored and dismissed.", tok, model);
    }
    double dphase = tok.nextDouble();

    if (!tok.isEmptyUntilNextKeyword()){
        handleParsingWarning("Second dynamic load phase dissmissed.", tok, model);
    }
    const auto& dynaphase = make_shared<DynaPhase>(model, dphase, original_id);
    dynaphase->setInputContext(tok.getInputContext());
    model.add(dynaphase);
}

void NastranParser::parseEIGC(NastranTokenizer& tok, Model& model) {
    int sid = tok.nextInt();
    const auto& objectiveSet = model.getOrCreateObjectiveSet(sid, ObjectiveSet::Type::CMETHOD);

    FrequencySearch::NormType norm; UNUSEDV(norm);
    string normString = tok.nextString(true, "MASS");
    if (normString == "MASS")
        norm = FrequencySearch::NormType::MASS;
    else if (normString == "MAX")
        norm = FrequencySearch::NormType::MAX;
    else {
        handleParsingWarning("Only MASS and MAX normalizing method (NORM) supported. Default (MASS) assumed.", tok, model);
        norm = FrequencySearch::NormType::MAX;
    }
}

void NastranParser::parseEIGB(NastranTokenizer& tok, Model& model) {
    int sid = tok.nextInt();
    const auto& objectiveSet = model.getOrCreateObjectiveSet(sid, ObjectiveSet::Type::METHOD);
    string method = tok.nextString(true); UNUSEDV(method);
    double lower = tok.nextDouble(true);
    double upper = tok.nextDouble(true);
    int ne = tok.nextInt(true); UNUSEDV(ne);// Estimate of number of roots in range: not use by the LANCZOS method

    int ndp = tok.nextInt(true); // Desired number of positive roots in range
    int ndn = tok.nextInt(true); UNUSEDV(ndn);// Desired number of negative roots in range
    handleParsingWarning("Desired number of negative roots in range NDN ignored.", tok, model);

    // See Nastran Quick reference guide for the treatment of nd in the Lancszos method
    if ((ndp==Globals::UNAVAILABLE_INT) && (is_equal(upper, Globals::UNAVAILABLE_DOUBLE))){
        ndp=1;
    }

    tok.skip(2);

    FrequencySearch::NormType norm;
    string normString = tok.nextString(true, "MASS");
    if (normString == "MASS")
        norm = FrequencySearch::NormType::MASS;
    else if (normString == "MAX")
        norm = FrequencySearch::NormType::MAX;
    else {
        handleParsingWarning("Only MASS and MAX normalizing method (NORM) supported. Default (MASS) assumed.", tok, model);
        norm = FrequencySearch::NormType::MAX;
    }
    int g = tok.nextInt(true);
    if (g!=Globals::UNAVAILABLE_INT){
        handleParsingWarning("Grid identification number (G) not supported.", tok, model);
    }
    int c = tok.nextInt(true);
    if (c!=Globals::UNAVAILABLE_INT){
        handleParsingWarning("Component number (C) not supported.", tok, model);
    }

    const auto& bandRange = make_shared<BandRange>(model, lower, ndp, upper);
    //bandRange.setParaX(Function::ParaName::FREQ);
    const auto& frequencyTarget = make_shared<FrequencySearch>(model, objectiveSet, FrequencySearch::FrequencyType::BAND, *bandRange, norm);

    bandRange->setInputContext(tok.getInputContext());
    model.add(bandRange);
    frequencyTarget->setInputContext(tok.getInputContext());
    model.add(frequencyTarget);
}

void NastranParser::parseEIGR(NastranTokenizer& tok, Model& model) {
    int sid = tok.nextInt();
    const auto& objectiveSet = model.getOrCreateObjectiveSet(sid, ObjectiveSet::Type::METHOD);
    string method = tok.nextString(true);
    if (method !="LAN"){
        handleParsingError("Only Lanczos method (LAN) is supported.", tok, model);
    }
    double lower = tok.nextDouble(true);
    double upper = tok.nextDouble(true);
    int ne = tok.nextInt(true); // Estimate of number of roots in range: not use by the LANCZOS method
    UNUSEDV(ne);
    int nd = tok.nextInt(true); // Desired number of roots in range

    // See Nastran Quick reference guide for the treatment of nd in the Lancszos method
    if ((nd==Globals::UNAVAILABLE_INT) && (is_equal(upper, Globals::UNAVAILABLE_DOUBLE))){
        nd=1;
    }

    tok.skip(2);

    FrequencySearch::NormType norm;
    string normString = tok.nextString(true, "MASS");
    if (normString == "MASS")
        norm = FrequencySearch::NormType::MASS;
    else if (normString == "MAX")
        norm = FrequencySearch::NormType::MAX;
    else {
        handleParsingWarning("Only MASS and MAX normalizing method (NORM) supported. Default (MASS) assumed.", tok, model);
        norm = FrequencySearch::NormType::MAX;
    }
    int g = tok.nextInt(true);
    if (g!=Globals::UNAVAILABLE_INT){
        handleParsingWarning("Grid identification number (G) not supported.", tok, model);
    }
    int c = tok.nextInt(true);
    if (c!=Globals::UNAVAILABLE_INT){
        handleParsingWarning("Component number (C) not supported.", tok, model);
    }

    const auto& bandRange = make_shared<BandRange>(model, lower, nd, upper);
    //bandRange.setParaX(Function::ParaName::FREQ);
    const auto& frequencySearch = make_shared<FrequencySearch>(model, objectiveSet, FrequencySearch::FrequencyType::BAND, *bandRange, norm);

    bandRange->setInputContext(tok.getInputContext());
    model.add(bandRange);
    frequencySearch->setInputContext(tok.getInputContext());
    model.add(frequencySearch);
}

void NastranParser::parseEIGRL(NastranTokenizer& tok, Model& model) {
    int sid = tok.nextInt();
    const auto& objectiveSet = model.getOrCreateObjectiveSet(sid, ObjectiveSet::Type::METHOD);
    double lower = tok.nextDouble(true);
    double upper = tok.nextDouble(true);
    int nd = tok.nextInt(true);

    // See Nastran Quick reference guide for the treatment of nd
    if ((nd==Globals::UNAVAILABLE_INT) && (is_equal(upper, Globals::UNAVAILABLE_DOUBLE))){
        nd=1;
    }

    // Diagnostic level: not supported, but useless.
    int msglvl = tok.nextInt(true);
    UNUSEDV(msglvl);

    // Unsupported fields
    int maxset = tok.nextInt(true);
    if (maxset!=Globals::UNAVAILABLE_INT){
        handleParsingWarning("Numbers of vectors in set (MAXSET) not supported and dismissed.", tok, model);
    }
    double shfscl = tok.nextDouble(true);
    if (!is_equal(shfscl, Globals::UNAVAILABLE_DOUBLE)){
        handleParsingWarning("Estimate of the first flexible mode natural frequency (SHFSCL) not supported and dismissed.", tok, model);
    }

    // Normalization method
    FrequencySearch::NormType norm;
    string normString = tok.nextString(true, "MASS");
    if (normString == "MASS")
        norm = FrequencySearch::NormType::MASS;
    else if (normString == "MAX")
        norm = FrequencySearch::NormType::MAX;
    else {
        handleParsingWarning("Only MASS and MAX normalizing method (NORM) supported. Default (MASS) assumed.", tok, model);
        norm = FrequencySearch::NormType::MAX;
    }
    const auto& bandRange = make_shared<BandRange>(model, lower, nd, upper);
    //bandRange.setParaX(Function::ParaName::FREQ);
    const auto& frequencySearch = make_shared<FrequencySearch>(model, objectiveSet, FrequencySearch::FrequencyType::BAND, *bandRange, norm);

    bandRange->setInputContext(tok.getInputContext());
    model.add(bandRange);
    frequencySearch->setInputContext(tok.getInputContext());
    model.add(frequencySearch);
}

void NastranParser::parseFORCE(NastranTokenizer& tok, Model& model) {

    int loadset_id = tok.nextInt();
    int node_id = tok.nextInt();
    int csid = tok.nextInt(true, CoordinateSystem::GLOBAL_COORDINATE_SYSTEM_ID);
    double force = tok.nextDouble(true,0.0);
    double fx = tok.nextDouble(true,0.0) * force;
    double fy = tok.nextDouble(true,0.0) * force;
    double fz = tok.nextDouble(true,0.0) * force;

    const auto& loadSet = model.getOrCreateLoadSet(loadset_id, LoadSet::Type::LOAD);
    const auto& force1 = make_shared<NodalForce>(model, loadSet, fx, fy, fz, 0., 0., 0., Loading::NO_ORIGINAL_ID,
            Reference<CoordinateSystem>(CoordinateSystem::Type::ABSOLUTE, csid));
    force1->addNodeId(node_id);
    force1->setInputContext(tok.getInputContext());
    model.add(force1);
}

void NastranParser::parseFORCE1(NastranTokenizer& tok, Model& model) {
//nodal force two nodes
    int loadset_id = tok.nextInt();
    int node_id = tok.nextInt();
    double force = tok.nextDouble();
    int node1 = tok.nextInt();
    int node2 = tok.nextInt();

    const auto& loadSet = model.getOrCreateLoadSet(loadset_id, LoadSet::Type::LOAD);
    const auto& force1 = make_shared<NodalForceTwoNodes>(model, loadSet, node1, node2, force);
    force1->addNodeId(node_id);
    force1->setInputContext(tok.getInputContext());
    model.add(force1);
}

void NastranParser::parseFORCE2(NastranTokenizer& tok, Model& model) {
    int sid = tok.nextInt();
    int node_id = tok.nextInt();
    double force = tok.nextDouble();
    int node1 = tok.nextInt();
    int node2 = tok.nextInt();
    int node3 = tok.nextInt();
    int node4 = tok.nextInt();

    const auto& loadSet = model.getOrCreateLoadSet(sid, LoadSet::Type::LOAD);
    const auto& force2 = make_shared<NodalForceFourNodes>(model, loadSet, node1, node2, node3, node4, force);
    force2->addNodeId(node_id);
    force2->setInputContext(tok.getInputContext());
    model.add(force2);
}

void NastranParser::parseFREQ(NastranTokenizer& tok, Model& model) {
    int sid = tok.nextInt();
    const auto& objectiveSet = model.getOrCreateObjectiveSet(sid, ObjectiveSet::Type::FREQ);
    list<double> frequencies = tok.nextDoubles();

    const auto& frequencyValue = make_shared<ListValue<double>>(model, frequencies);
    frequencyValue->setInputContext(tok.getInputContext());
    model.add(frequencyValue);
    const auto& frequencyRange = make_shared<FrequencyExcit>(model, objectiveSet, FrequencyExcit::FrequencyType::LIST, *frequencyValue, FrequencyExcit::NormType::MASS);
    frequencyRange->setInputContext(tok.getInputContext());
    model.add(frequencyRange);
}

void NastranParser::parseFREQ1(NastranTokenizer& tok, Model& model) {
    int sid = tok.nextInt();
    const auto& objectiveSet = model.getOrCreateObjectiveSet(sid, ObjectiveSet::Type::FREQ);
    double start = tok.nextDouble();
    double step = tok.nextDouble();
    int count = tok.nextInt(true, 1);

    const auto& stepRange = make_shared<StepRange>(model, start, step, count);
    //stepRange.setParaX(Function::ParaName::FREQ);
    const auto& frequencyExcit = make_shared<FrequencyExcit>(model, objectiveSet, FrequencyExcit::FrequencyType::STEP, *stepRange, FrequencyExcit::NormType::MASS);
    stepRange->setInputContext(tok.getInputContext());
    model.add(stepRange);
    frequencyExcit->setInputContext(tok.getInputContext());
    model.add(frequencyExcit);
}

void NastranParser::parseFREQ3(NastranTokenizer& tok, Model& model) {
    int sid = tok.nextInt();
    const auto& objectiveSet = model.getOrCreateObjectiveSet(sid, ObjectiveSet::Type::FREQ);
    double startf = tok.nextDouble();
    double endf = tok.nextDouble();
    string type = tok.nextString(true, "LINEAR");
    if (type != "LINEAR")
        handleParsingError("Only LINEAR FREQ3 supported.", tok, model);
    int nef = tok.nextInt(true, 10);
    double cluster = tok.nextDouble(true, 1.0);
    if (not is_equal(cluster, 1.0))
        handleParsingError("CLUSTER in FREQ3 not (yet) supported.", tok, model);

    const auto& bandRange = make_shared<BandRange>(model, startf, nef, endf);
    //stepRange.setParaX(Function::ParaName::FREQ);
    const auto& frequencyExcit = make_shared<FrequencyExcit>(model, objectiveSet, FrequencyExcit::FrequencyType::INTERPOLATE, *bandRange, FrequencyExcit::NormType::MASS);
    bandRange->setInputContext(tok.getInputContext());
    model.add(bandRange);
    frequencyExcit->setInputContext(tok.getInputContext());
    model.add(frequencyExcit);
}

void NastranParser::parseFREQ4(NastranTokenizer& tok, Model& model) {
    int sid = tok.nextInt();
    const auto& objectiveSet = model.getOrCreateObjectiveSet(sid, ObjectiveSet::Type::FREQ);
    double f1 = tok.nextDouble();
    double f2 = tok.nextDouble();
    double spread = tok.nextDouble();
    int nef = tok.nextInt(true, 1);

    const auto& bandRange = make_shared<BandRange>(model, f1, nef, f2);
    //spreadRange.setParaX(Function::ParaName::FREQ);
    const auto& frequencyExcit = make_shared<FrequencyExcit>(model, objectiveSet, FrequencyExcit::FrequencyType::SPREAD, *bandRange, FrequencyExcit::NormType::MASS);
    frequencyExcit->spread = spread;
    bandRange->setInputContext(tok.getInputContext());
    model.add(bandRange);
    frequencyExcit->setInputContext(tok.getInputContext());
    model.add(frequencyExcit);
}

void NastranParser::parseGRAV(NastranTokenizer& tok, Model& model) {
    int sid = tok.nextInt();
    int csid = tok.nextInt(true, CoordinateSystem::GLOBAL_COORDINATE_SYSTEM_ID);
    if (csid != CoordinateSystem::GLOBAL_COORDINATE_SYSTEM_ID) {
        handleParsingWarning("CoordinateSystem not supported.", tok, model);
    }
    double acceleration = tok.nextDouble(true, 0);
    double x = tok.nextDouble(true, 0);
    double y = tok.nextDouble(true, 0);
    double z = tok.nextDouble(true, 0);
    int mb = tok.nextInt(true, 0);
    if (mb != 0) {
        handleParsingWarning("MB not supported.", tok, model);
    }

    const auto& loadSet = model.getOrCreateLoadSet(sid, LoadSet::Type::LOAD);
    const auto& gravity = make_shared<Gravity>(model, loadSet, acceleration, VectorialValue(x, y, z));
    gravity->setInputContext(tok.getInputContext());
    model.add(gravity);
}
void NastranParser::parseInclude(NastranTokenizer& tok, Model& model) {
    string currentRawDataLine = tok.currentRawDataLine();
    string fileName = currentRawDataLine.substr(7, currentRawDataLine.length() - 7);
    trim(fileName);
    if (!fileName.compare(0, 1, "'")
            && !fileName.compare(fileName.size() - 1, fileName.size(), "'"))
        fileName = fileName.substr(1, fileName.size() - 2);
    fs::path currentFname(tok.getFileName());
    fs::path includePath = currentFname.parent_path() / fileName;
    const string includePathStr = includePath.string();
    if (fs::exists(includePath)) {
        ifstream istream(includePathStr);
        NastranTokenizer tok2 {istream, this->logLevel, includePathStr, this->translationMode};
        tok2.bulkSection();
        tok2.nextLine();
        parseBULKSection(tok2, model);
        istream.close();
    } else {
        handleParsingError("Missing include file "+includePathStr, tok, model);
    }
    tok.skipToNextKeyword();
}

void NastranParser::parseLSEQ(NastranTokenizer& tok, Model& model) {
    int set_id = tok.nextInt();
    // LSEQ will not be used unless selected in the Case Control Section with the LOADSET command.
    shared_ptr<LoadSet> loadSetMaster = model.getOrCreateLoadSet(set_id, LoadSet::Type::LOADSET);
    int darea_id = tok.nextInt(); UNUSEDV(darea_id);
    // LD : not sure about this interpretation
//    Reference<LoadSet> dareaReference(LoadSet::Type::LOAD, darea_id);
//    loadSetMaster->embedded_loadsets.push_back(
//                    pair<Reference<LoadSet>, double>(dareaReference, 1.0));
    int loadSet_id = tok.nextInt();
    Reference<LoadSet> loadSetReference(LoadSet::Type::LOAD, loadSet_id);
    loadSetMaster->embedded_loadsets.push_back({loadSetReference, 1.0});

}

void NastranParser::parseLOAD(NastranTokenizer& tok, Model& model) {
    int set_id = tok.nextInt();
    shared_ptr<LoadSet> loadSetMaster = model.getOrCreateLoadSet(set_id, LoadSet::Type::LOAD);
    double S = tok.nextDouble(true, 1);
    while (tok.isNextDouble()) {
        double scale = tok.nextDouble(true, 1);
        int loadSet_id = tok.nextInt();
        Reference<LoadSet> loadSetReference(LoadSet::Type::LOAD, loadSet_id);
        loadSetMaster->embedded_loadsets.push_back({loadSetReference, S * scale});
    }
}

void NastranParser::parseMAT1(NastranTokenizer& tok, Model& model) {
    int material_id = tok.nextInt();
    double e = tok.nextDouble(true, Globals::UNAVAILABLE_DOUBLE);
    double g = tok.nextDouble(true, Globals::UNAVAILABLE_DOUBLE);
    double nu = tok.nextDouble(true, Globals::UNAVAILABLE_DOUBLE);
    double rho = tok.nextDouble(true, Globals::UNAVAILABLE_DOUBLE);
    double a = tok.nextDouble(true, Globals::UNAVAILABLE_DOUBLE);
    double tref = tok.nextDouble(true, Globals::UNAVAILABLE_DOUBLE);
    double ge = tok.nextDouble(true, Globals::UNAVAILABLE_DOUBLE);

    // Default behavior from page 1664 of MDN Nastran 2006 Quick Reference Guide
    if ((is_equal(e,Globals::UNAVAILABLE_DOUBLE)) and (is_equal(g,Globals::UNAVAILABLE_DOUBLE))) {
        handleParsingWarning("Material " + to_string(material_id)+": E and G may not both be blank.", tok, model);
    }
    if (is_equal(nu,Globals::UNAVAILABLE_DOUBLE)) {
        if (is_equal(g,Globals::UNAVAILABLE_DOUBLE)) {
            nu = 0.0;
            g = 0.0;
        } else if (is_equal(e,Globals::UNAVAILABLE_DOUBLE)) {
            nu = 0.0;
            e = 0.0;
        } else {
            nu = e/(2.0*g)-1;
        }
    }
    if ((is_equal(e,Globals::UNAVAILABLE_DOUBLE))&&
            (!is_equal(g,Globals::UNAVAILABLE_DOUBLE))&&(!is_equal(nu,Globals::UNAVAILABLE_DOUBLE))){
        e= 2.0 * (1+nu) * g;
    }
    if ((is_equal(g,Globals::UNAVAILABLE_DOUBLE))&&
            (!is_equal(e,Globals::UNAVAILABLE_DOUBLE))&&(!is_equal(nu,Globals::UNAVAILABLE_DOUBLE))){
        g = e / (2.0 * (1+nu));
    }

    /*  ST, SC, SS
     (Real)
     Stress limits for tension, compression, and shear are optionally
     supplied, used only to compute margins of safety in certain elements;
     and have no effect on the computational procedures. See “Beam
     Element (CBEAM)” in Chapter 3 of the MSC.Nastran Reference Guide.
     (Real > 0.0 or blank)*/
    double st = tok.nextDouble(true, Globals::UNAVAILABLE_DOUBLE);
    if (!is_equal(st, Globals::UNAVAILABLE_DOUBLE)) {
        handleParsingWarning("st value ignored " + to_string(st), tok, model);
    }
    double sc = tok.nextDouble(true, Globals::UNAVAILABLE_DOUBLE);
    if (!is_equal(sc, Globals::UNAVAILABLE_DOUBLE)) {
        handleParsingWarning("sc value ignored " + to_string(sc), tok, model);
    }
    double ss = tok.nextDouble(true, Globals::UNAVAILABLE_DOUBLE);
    if (!is_equal(ss, Globals::UNAVAILABLE_DOUBLE)) {
        handleParsingWarning("ss value ignored " + to_string(ss), tok, model);
    }
    /*  MCSID
     Material coordinate system identification number. Used only for
     PARAM,CURV processing. See “Parameters” on page 659.
     (Integer > 0 or blank)*/
    int mcsid = tok.nextInt(true, 0);
    if (mcsid != 0) {
        handleParsingWarning("mcsid value ignored " + to_string(mcsid), tok, model);
    }
    shared_ptr<Material> material = model.getOrCreateMaterial(material_id);
    material->addNature(make_shared<ElasticNature>(model, e, nu, g, rho, a, tref, ge));
}

void NastranParser::parseMAT8(NastranTokenizer& tok, Model& model) {
    int material_id = tok.nextInt();
    double e1 = tok.nextDouble();
    double e2 = tok.nextDouble();
    double nu12 = tok.nextDouble();
    double g12 = tok.nextDouble();
    double g1Z = tok.nextDouble(true, Globals::UNAVAILABLE_DOUBLE);
    double g2Z = tok.nextDouble(true, Globals::UNAVAILABLE_DOUBLE);
    double rho = tok.nextDouble(true, Globals::UNAVAILABLE_DOUBLE);
    shared_ptr<Material> material = model.getOrCreateMaterial(material_id);
    material->addNature(make_shared<OrthotropicNature>(model, e1, e2, nu12, g12, g2Z, g1Z, rho));
}

void NastranParser::parseMATHP(NastranTokenizer& tok, Model& model) {
    int mid = tok.nextInt();
    auto material = model.getOrCreateMaterial(mid);
    double a10 = tok.nextDouble(true, 0.0);
    double a01 = tok.nextDouble(true, 0.0);
    double d1 = tok.nextDouble(true, 1000*(a10+a01));
    double rho = tok.nextDouble(true, 0.0);
    double av = tok.nextDouble(true, 0.0); UNUSEDV(av);
    double tref = tok.nextDouble(true, 0.0); UNUSEDV(tref);
    double ge = tok.nextDouble(true, 0.0); UNUSEDV(ge);
    tok.skip(1);
    double na = tok.nextInt(true, 1);
    if (na > 3) {
        handleParsingError("MATHP NA " + to_string(na) + " too big, not yet implemented.", tok,
                model);
    }
    double nd = tok.nextInt(true, 1);
    if (nd > 1) {
        handleParsingError("MATHP ND " + to_string(nd) + " too big, not yet implemented.", tok,
                model);
    }
    tok.skip(5);
    double a20 = tok.nextDouble(true, 0.0);
    material->addNature(make_shared<HyperElasticNature>(model, a10, a01, a20, d1, rho));
}

void NastranParser::parseMATS1(NastranTokenizer& tok, Model& model) {
    int mid = tok.nextInt();
    auto material = model.getOrCreateMaterial(mid);
    int tid = tok.nextInt(true, 0);
    string type = tok.nextString();
    double h = tok.nextDouble(true, Globals::UNAVAILABLE_DOUBLE);
    int yf = tok.nextInt(true, 1);
    int hr = tok.nextInt(true, 1);
    double limit1 = tok.nextDouble(true, Globals::UNAVAILABLE_DOUBLE);
    double limit2 = tok.nextDouble(true, Globals::UNAVAILABLE_DOUBLE);
    if (type == "NLELAST") {
        material->addNature(make_shared<NonLinearElasticNature>(model, tid));
    } else if (type == "PLASTIC") {
        if (tid == 0) {
            const auto& biNature = make_shared<BilinearElasticNature>(model);
            biNature->elastic_limit = limit1;
            if (!is_equal(limit2, Globals::UNAVAILABLE_DOUBLE)) {
                handleParsingError("MATS1 limit2 " + to_string(yf) + " not yet implemented.", tok,
                        model);
            }
            biNature->secondary_slope = h;
            switch (hr) {
            case 1:
                biNature->hardening_rule_isotropic = true;
                break;
            default:
                handleParsingError("MATS1 hr " + to_string(hr) + " not yet implemented.", tok,
                        model);
                break;
            }
            switch (yf) {
            case 1:
                biNature->yield_function_von_mises = true;
                break;
            default:
                handleParsingError("MATS1 yf " + to_string(yf) + " not yet implemented.", tok,
                        model);
                break;
            }
            material->addNature(biNature);
        } else {
            handleParsingError(
                    "MATS1 TID " + to_string(tid) + " not yet implemented for plastic law.", tok,
                    model);
        }
    } else {
        handleParsingError("MATS1 type " + type + " not implemented.", tok, model);
    }
}

void NastranParser::parseMOMENT(NastranTokenizer& tok, Model& model) {
    int loadset_id = tok.nextInt();
    int node_id = tok.nextInt();
    int csid = tok.nextInt(true, CoordinateSystem::GLOBAL_COORDINATE_SYSTEM_ID);
    if (csid != CoordinateSystem::GLOBAL_COORDINATE_SYSTEM_ID) {
        handleParsingWarning("MOMENT coordinate system not supported", tok, model);
    }
    double scale = tok.nextDouble(true);
    double frx = tok.nextDouble(true) * scale;
    double fry = tok.nextDouble(true) * scale;
    double frz = tok.nextDouble(true) * scale;

    const auto& loadSet = model.getOrCreateLoadSet(loadset_id, LoadSet::Type::LOAD);
    const auto& force1 = make_shared<NodalForce>(model, loadSet, VectorialValue(0, 0, 0), VectorialValue(frx, fry, frz),
            Loading::NO_ORIGINAL_ID);
    force1->addNodeId(node_id);
    force1->setInputContext(tok.getInputContext());
    model.add(force1);
}

void NastranParser::parseMPC(NastranTokenizer& tok, Model& model) {
    int set_id = tok.nextInt();
    const auto& lmpc = make_shared<LinearMultiplePointConstraint>(model);
    int i = 2;
    while (tok.isNextInt()) {
        const int g1 = tok.nextInt();
        const int c1 = tok.nextInt();
        const double a1 = tok.nextDouble();
        DOFS dofs = DOFS::nastranCodeToDOFS(c1);
        i += 3;
        if (i % 8 == 0 && !tok.isEmptyUntilNextKeyword()) {
            tok.skip(2);
            i += 2;
        }
        lmpc->addParticipation(g1, dofs.contains(DOF::DX) * a1, dofs.contains(DOF::DY) * a1,
                dofs.contains(DOF::DZ) * a1, dofs.contains(DOF::RX) * a1,
                dofs.contains(DOF::RY) * a1, dofs.contains(DOF::RZ) * a1);
    }
    lmpc->setInputContext(tok.getInputContext());
    model.add(lmpc);
    model.addConstraintIntoConstraintSet(*lmpc,
            Reference<ConstraintSet>(ConstraintSet::Type::MPC, set_id));
}

void NastranParser::parseNLPARM(NastranTokenizer& tok, Model& model) {
    int sid = tok.nextInt();
    const auto& objectiveSet = model.getOrCreateObjectiveSet(sid, ObjectiveSet::Type::NONLINEAR_STRATEGY);
    int number_of_increments = tok.nextInt(true, 10);

    if (!tok.isEmptyUntilNextKeyword()){
        tok.skipToNextKeyword();
        handleParsingWarning("All NLPARM parameters are ignored except NINC.", tok, model);
    }

    const auto& nonLinearStrategy = make_shared<NonLinearStrategy>(model, objectiveSet, number_of_increments);
    nonLinearStrategy->setInputContext(tok.getInputContext());
    model.add(nonLinearStrategy);
}

void NastranParser::parseNLPCI(NastranTokenizer& tok, Model& model) {
    int sid = tok.nextInt();
    const auto& objectiveSet = model.getOrCreateObjectiveSet(sid, ObjectiveSet::Type::NONLINEAR_STRATEGY);
    if (!tok.isEmptyUntilNextKeyword()){
        tok.skipToNextKeyword();
        handleParsingWarning("All NLPCI parameters are ignored.", tok, model);
    }

    const auto& arcLengthMethod = make_shared<ArcLengthMethod>(model, objectiveSet, Reference<Objective>(Objective::Type::NONLINEAR_PARAMETERS));
    arcLengthMethod->setInputContext(tok.getInputContext());
    model.add(arcLengthMethod);
}


void NastranParser::parsePBAR(NastranTokenizer& tok, Model& model) {
    int elemId = tok.nextInt();
    int mid = tok.nextInt();
    double area = tok.nextDouble();
    const double i1 = tok.nextDouble(true, 0.0); // I1 = Izz
    const double i2 = tok.nextDouble(true, 0.0); // I2 = Iyy
    const double j = tok.nextDouble(true, 0.0);  // J = Ixx
    const double nsm = tok.nextDouble(true, 0.0);
    if (!tok.isEmptyUntilNextKeyword())
        tok.skip(1);
    double c1 = tok.nextDouble(true, 0.0);
    double c2 = tok.nextDouble(true, 0.0);
    double d1 = tok.nextDouble(true, 0.0);
    double d2 = tok.nextDouble(true, 0.0);
    double e1 = tok.nextDouble(true, 0.0);
    double e2 = tok.nextDouble(true, 0.0);
    double f1 = tok.nextDouble(true, 0.0);
    double f2 = tok.nextDouble(true, 0.0);

    // K1, K2: Area factors for shear. VEGA works with 1/K1 and 1/K2
    // Default values is infinite
    const double k1 = tok.nextDouble(true); // K1 = Kzz
    const double k2 = tok.nextDouble(true); // K2 = Kyy
    double invk1, invk2;
    if (is_equal(k1, Globals::UNAVAILABLE_DOUBLE)){
        invk1= 0.0;
    }else{
        invk1 = is_zero(k1) ? Globals::UNAVAILABLE_DOUBLE : 1.0/k1;
    }
    if (is_equal(k2, Globals::UNAVAILABLE_DOUBLE)){
        invk2= 0.0;
    }else{
        invk2 = is_zero(k2) ? Globals::UNAVAILABLE_DOUBLE : 1.0/k2;
    }

    double i12 = tok.nextDouble(true, 0.0);
    if (!is_equal(i12, 0)) {
        handleParsingWarning("PBAR i12 not implemented.", tok, model);
    }

    const auto& genericSectionBeam = make_shared<GenericSectionBeam>(model, area, i2, i1, j, invk2, invk1, Beam::BeamModel::TIMOSHENKO, nsm,
            elemId);
    genericSectionBeam->assignMaterial(Reference<Material>(Material::Type::MATERIAL,mid));
    genericSectionBeam->add(*getOrCreateCellGroup(elemId, model, "PBAR"));
    std::list<std::pair<double, double>> reccoefs = { {c1, c2}, {d1, d2}, {e1, e2}, {f1, f2} };
    for (const auto& reccoef : reccoefs) {
        if (is_zero(reccoef.first) and is_zero(reccoef.second)) {
                continue;
        }
        RecoveryPoint c1recpointa(model, 0.0, reccoef.first, reccoef.second);
        genericSectionBeam->recoveryPoints.push_back(c1recpointa);
        RecoveryPoint c1recpointb(model, 1.0, reccoef.first, reccoef.second);
        genericSectionBeam->recoveryPoints.push_back(c1recpointb);
    }
    genericSectionBeam->setInputContext(tok.getInputContext());
    model.add(genericSectionBeam);
}

void NastranParser::parsePBARL(NastranTokenizer& tok, Model& model) {
    int propertyId = tok.nextInt(); // PID
    int mid = tok.nextInt();

    string group = tok.nextString(true, "MSCBML0");
    if (group!="MSCBML0"){
        string message = "PBARL users defined groups are not supported.";
        handleParsingError(message, tok, model);
    }

    string type = tok.nextString();
    double nsm;
    if (type == "BAR") {
        tok.skip(4);
        double width = tok.nextDouble();
        double height = tok.nextDouble();
        nsm = tok.nextDouble(true, 0.0);
        const auto& rectangularSectionBeam = make_shared<RectangularSectionBeam>(model, width, height, Beam::BeamModel::TIMOSHENKO, nsm,
                propertyId);
        rectangularSectionBeam->assignMaterial(Reference<Material>(Material::Type::MATERIAL,mid));
        rectangularSectionBeam->add(*getOrCreateCellGroup(propertyId, model, "PBARL"));
        rectangularSectionBeam->setInputContext(tok.getInputContext());
        model.add(rectangularSectionBeam);
    } else if (type == "ROD") {
        tok.skip(4);
        double radius = tok.nextDouble();
        nsm = tok.nextDouble(true, 0.0);
        const auto& circularSectionBeam = make_shared<CircularSectionBeam>(model, radius, Beam::BeamModel::TIMOSHENKO, nsm, propertyId);
        circularSectionBeam->assignMaterial(Reference<Material>(Material::Type::MATERIAL,mid));
        circularSectionBeam->add(*getOrCreateCellGroup(propertyId, model, "PBARL"));
        circularSectionBeam->setInputContext(tok.getInputContext());
        model.add(circularSectionBeam);
    } else if (type == "TUBE") {
        tok.skip(4);
        double extRadius = tok.nextDouble();
        double intRadius = tok.nextDouble();
        nsm = tok.nextDouble(true, 0.0);
        const auto& tubeSectionBeam = make_shared<TubeSectionBeam>(model, intRadius, extRadius - intRadius, Beam::BeamModel::TIMOSHENKO, nsm, propertyId);
        tubeSectionBeam->assignMaterial(Reference<Material>(Material::Type::MATERIAL,mid));
        tubeSectionBeam->add(*getOrCreateCellGroup(propertyId, model, "PBARL"));
        tubeSectionBeam->setInputContext(tok.getInputContext());
        model.add(tubeSectionBeam);
    } else if (type == "I") {
        tok.skip(4);
        double beam_height = tok.nextDouble();
        double lower_flange_width = tok.nextDouble();
        double upper_flange_width = tok.nextDouble();
        double web_thickness = tok.nextDouble();
        double lower_flange_thickness = tok.nextDouble();
        double upper_flange_thickness = tok.nextDouble();
        nsm = tok.nextDouble(true, 0.0);
        const auto& iSectionBeam = make_shared<ISectionBeam>(model, upper_flange_width, lower_flange_width,
                upper_flange_thickness, lower_flange_thickness, beam_height, web_thickness,
                Beam::BeamModel::TIMOSHENKO, nsm, propertyId);
        iSectionBeam->assignMaterial(Reference<Material>(Material::Type::MATERIAL,mid));
        iSectionBeam->add(*getOrCreateCellGroup(propertyId, model, "PBARL"));
        iSectionBeam->setInputContext(tok.getInputContext());
        model.add(iSectionBeam);
    } else {
        string message = "PBARL type " + type + " not implemented.";
        handleParsingError(message, tok, model);
    }

    if (!tok.isEmptyUntilNextKeyword()) {
        tok.skipToNextKeyword();
        handleParsingWarning("Ignoring last part of PBARL line.", tok,
             model);
    }

}

void NastranParser::parsePBEAM(NastranTokenizer& tok, Model& model) {
    int elemId = tok.nextInt();
    int mid = tok.nextInt();

    double area_cross_section = tok.nextDouble(true, 0.0);
    double moment_of_inertia_Z = tok.nextDouble(true, 0.0);
    double moment_of_inertia_Y = tok.nextDouble(true, 0.0);
    double areaProductOfInertia = tok.nextDouble(true, 0.0);
    if (!is_equal(areaProductOfInertia, 0.0)) {
        handleParsingWarning("Area PBEAM product of inertia not implemented.", tok, model);
    }
    double torsionalConstant = tok.nextDouble(true, 0.0);
    double nsm = tok.nextDouble(true, 0.0);
    if (!is_equal(nsm, 0.0)) {
        handleParsingWarning("NSM not implemented.", tok, model);
    }
    double c1 = tok.nextDouble(true, 0.0);
    double c2 = tok.nextDouble(true, 0.0);
    double d1 = tok.nextDouble(true, 0.0);
    double d2 = tok.nextDouble(true, 0.0);
    double e1 = tok.nextDouble(true, 0.0);
    double e2 = tok.nextDouble(true, 0.0);
    double f1 = tok.nextDouble(true, 0.0);
    double f2 = tok.nextDouble(true, 0.0);
    if (!is_equal(c1, 0) || !is_equal(c2, 0) || !is_equal(d1, 0) || !is_equal(d2, 0)
            || !is_equal(e1, 0) || !is_equal(e2, 0) || !is_equal(f1, 0) || !is_equal(f2, 0)) {
        /*
         * Ci(A), Di(A) Ei(A), Fi(A)
         * The y and z locations (i = 1 corresponds to y
         * and i = 2 corresponds to z) in element
         * coordinates relative to the shear center (see the
         * diagram following the remarks) at end A for
         * stress data recovery. (Real)
         * y = z = 0.0
         * Ci, Di, Ei, Fi
         * The y and z locations (i = 1 corresponds to y
         * and i = 2 corresponds to z) in element
         * coordinates relative to the shear center (see
         * Figure 8-134 in Remark 10.) for the cross section
         * located at X/XB. The values are fiber locations
         * for stress data recovery. Ignored for beam p-
         * elements. (Real)
         */
        handleParsingWarning("Shear center for stress analysis not implemented.", tok,
                model);
    }
    const auto& genericSectionBeam = make_shared<GenericSectionBeam>(model, area_cross_section, moment_of_inertia_Y,
            moment_of_inertia_Z, torsionalConstant, 0.0, 0.0, GenericSectionBeam::BeamModel::EULER, nsm,
            elemId);
    genericSectionBeam->assignMaterial(Reference<Material>(Material::Type::MATERIAL,mid));
    genericSectionBeam->add(*getOrCreateCellGroup(elemId, model, "PBEAM"));
    genericSectionBeam->setInputContext(tok.getInputContext());
    model.add(genericSectionBeam);

    // Intermediate stations are not supported
    int nbStations=0;
    while (! (tok.isEmptyUntilNextKeyword() || tok.isNextDouble() || tok.isNextEmpty())){
        nbStations++;
        tok.skip(16);
        if (nbStations == 1) {
            handleParsingWarning("PBEAM intermediate stations are not supported", tok, model);
        }
    }

    // A bunch of parameters we don't support
    double K1 = tok.nextDouble(true, 1.0); // Shear stiffness factor for Plane 1
    double K2 = tok.nextDouble(true, 1.0); // Shear stiffness factor for Plane 2
    if ((!is_equal(K1, 1.0)) || (!is_equal(K2,1.0))){
        handleParsingWarning("Shear stiffness factors (K1, K2) are not supported.", tok, model);
    }

    double S1 = tok.nextDouble(true, 0.0); // Shear relief coefficient for Plane 1
    double S2 = tok.nextDouble(true, 0.0); // Shear relief coefficient for Plane 2
    if ((!is_zero(S1)) || (!is_zero(S2))){
        handleParsingWarning("Shear relief coefficients (S1, S2) are not supported.", tok, model);
    }

    double NSIA = tok.nextDouble(true, 0.0); // Nonstructural mass moment of inertia per unit length about nonstructural mass center of gravity at end A and end B.
    double NSIB = tok.nextDouble(true, 0.0);
    if ((!is_zero(NSIA)) || (!is_zero(NSIB))){
        handleParsingWarning("Nonstructural mass centers of gravity (NSIA, NSIB) are not supported.", tok, model);
    }

    double CWA = tok.nextDouble(true, 0.0); // Warping coefficient for end A and end B.
    double CWB = tok.nextDouble(true, 0.0);
    if ((!is_zero(CWA)) || (!is_zero(CWB))){
        handleParsingWarning("Warping coefficients (CWA, CWB) are not supported.", tok, model);
    }

    double M1A = tok.nextDouble(true, 0.0); // (y,z) coordinates of center of gravity of nonstructural mass for end A and end B.
    double M2A = tok.nextDouble(true, 0.0);
    double M1B = tok.nextDouble(true, 0.0);
    double M2B = tok.nextDouble(true, 0.0);
    if ( (!is_zero(M1A)) || (!is_zero(M2A)) || (!is_zero(M1B)) || (!is_zero(M2B))){
        handleParsingWarning("Center of gravity of nonstructural mass (M1A, M2A, M1B, M2B) are not supported.", tok, model);
    }

    double N1A = tok.nextDouble(true, 0.0); // (y,z) coordinates of neutral axis for end A and end B.
    double N2A = tok.nextDouble(true, 0.0);
    double N1B = tok.nextDouble(true, 0.0);
    double N2B = tok.nextDouble(true, 0.0);
    if ( (!is_zero(N1A)) || (!is_zero(N2A)) || (!is_zero(N1B)) || (!is_zero(N2B))){
        handleParsingWarning("Neutral axis (N1A, N2A, N1B, N2B) are not supported.", tok, model);
    }

}

void NastranParser::parsePBEAML(NastranTokenizer& tok, Model& model) {
    int pid = tok.nextInt();
    int mid = tok.nextInt();

    string group = tok.nextString(true, "MSCBML0");
    if (group!="MSCBML0"){
        string message = "PBARL users defined groups are not supported.";
        handleParsingError(message, tok, model);
    }


    string type = tok.nextString();
    double nsm;
    if (type == "BAR") {
        tok.skip(4);
        double width = tok.nextDouble();
        double height = tok.nextDouble();
        nsm = tok.nextDouble(true, 0.0);
        const auto& rectangularSectionBeam = make_shared<RectangularSectionBeam>(model, width, height, Beam::BeamModel::TIMOSHENKO, nsm, pid);
        rectangularSectionBeam->assignMaterial(Reference<Material>(Material::Type::MATERIAL,mid));
        rectangularSectionBeam->add(*getOrCreateCellGroup(pid, model,"PBEAML"));
        rectangularSectionBeam->setInputContext(tok.getInputContext());
        model.add(rectangularSectionBeam);
    } else if (type == "ROD") {
        tok.skip(4);
        double radius = tok.nextDouble();
        nsm = tok.nextDouble(true, 0.0);
        const auto& circularSectionBeam = make_shared<CircularSectionBeam>(model, radius, Beam::BeamModel::TIMOSHENKO, nsm, pid);
        circularSectionBeam->assignMaterial(Reference<Material>(Material::Type::MATERIAL,mid));
        circularSectionBeam->add(*getOrCreateCellGroup(pid, model,"PBEAML"));
        circularSectionBeam->setInputContext(tok.getInputContext());
        model.add(circularSectionBeam);
    } else if (type == "TUBE") {
        tok.skip(4);
        double extRadius = tok.nextDouble();
        double intRadius = tok.nextDouble();
        nsm = tok.nextDouble(true, 0.0);
        const auto& tubeSectionBeam = make_shared<TubeSectionBeam>(model, intRadius, extRadius - intRadius, Beam::BeamModel::TIMOSHENKO, nsm, pid);
        tubeSectionBeam->assignMaterial(Reference<Material>(Material::Type::MATERIAL,mid));
        tubeSectionBeam->add(*getOrCreateCellGroup(pid, model,"PBEAML"));
        tubeSectionBeam->setInputContext(tok.getInputContext());
        model.add(tubeSectionBeam);
    } else if (type == "I") {
        tok.skip(4);
        double beam_height = tok.nextDouble();
        double lower_flange_width = tok.nextDouble();
        double upper_flange_width = tok.nextDouble();
        double web_thickness = tok.nextDouble();
        double lower_flange_thickness = tok.nextDouble();
        double upper_flange_thickness = tok.nextDouble();
        nsm = tok.nextDouble(true, 0.0);
        string so = tok.nextString(true, "YES");
        const auto& iSectionBeam = make_shared<ISectionBeam>(model, upper_flange_width, lower_flange_width,
                upper_flange_thickness, lower_flange_thickness, beam_height, web_thickness,
                Beam::BeamModel::TIMOSHENKO, nsm, pid);
        iSectionBeam->assignMaterial(Reference<Material>(Material::Type::MATERIAL,mid));
        iSectionBeam->add(*getOrCreateCellGroup(pid, model,"PBEAML"));
        iSectionBeam->setInputContext(tok.getInputContext());
        model.add(iSectionBeam);
    } else {
        string message = "PBEAML type " + type + " not implemented.";
        handleParsingError(message, tok, model);
    }

    if (!tok.isEmptyUntilNextKeyword()) {
        tok.skipToNextKeyword();
        handleParsingWarning("Ignoring rest of line.", tok,
             model);
    }

}



/** Parse the NASTRAN PBUSH Keyword: Generalized Spring-And-Damper Property **/
void NastranParser::parsePBUSH(NastranTokenizer& tok, Model& model) {

    int pid = tok.nextInt();
    double k1=0.0;
    double k2=0.0;
    double k3=0.0;
    double k4=0.0;
    double k5=0.0;
    double k6=0.0;
    double b1=0.0;
    double b2=0.0;
    double b3=0.0;
    double b4=0.0;
    double b5=0.0;
    double b6=0.0;
    double ge1=0.0;
    double ge2=0.0;
    double ge3=0.0;
    double ge4=0.0;
    double ge5=0.0;
    double ge6=0.0;
    double sa=1.0;
    double st=1.0;
    double ea=1.0;
    double et=1.0;

    // Parsing the keyword: done this way to avoid warning message when the user
    // specified null B or null GE.
    while (!(tok.isEmptyUntilNextKeyword())){
        tok.skipToNotEmpty();
        string flag = tok.nextString();
        if (flag=="K") { // Stiffness values (Default 0.0)
            if (tok.isNextDouble() or tok.isNextEmpty())
                k1=tok.nextDouble(true, 0.0);
            else
                if (tok.nextString() == "RIGID")
                    k1 = DBL_MAX;
                else
                    handleParsingError("Unsupported PBUSH flag value (for now)", tok, model);
            if (tok.isNextDouble() or tok.isNextEmpty())
                k2=tok.nextDouble(true, 0.0);
            else
                if (tok.nextString() == "RIGID")
                    k2 = DBL_MAX;
                else
                    handleParsingError("Unsupported PBUSH flag value (for now)", tok, model);
            if (tok.isNextDouble() or tok.isNextEmpty())
                k3=tok.nextDouble(true, 0.0);
            else
                if (tok.nextString() == "RIGID")
                    k3 = DBL_MAX;
                else
                    handleParsingError("Unsupported PBUSH flag value (for now)", tok, model);
            if (tok.isNextDouble() or tok.isNextEmpty())
                k4=tok.nextDouble(true, 0.0);
            else
                if (tok.nextString() == "RIGID")
                    k4 = DBL_MAX;
                else
                    handleParsingError("Unsupported PBUSH flag value (for now)", tok, model);
            if (tok.isNextDouble() or tok.isNextEmpty())
                k5=tok.nextDouble(true, 0.0);
            else
                if (tok.nextString() == "RIGID")
                    k5 = DBL_MAX;
                else
                    handleParsingError("Unsupported PBUSH flag value (for now)", tok, model);
            if (tok.isNextDouble() or tok.isNextEmpty())
                k6=tok.nextDouble(true, 0.0);
            else
                if (tok.nextString() == "RIGID")
                    k6 = DBL_MAX;
                else
                    handleParsingError("Unsupported PBUSH flag value (for now)", tok, model);
        } else if (flag=="B") { // Force-Per-velocity Damping (Default 0.0)
            b1=tok.nextDouble(true, 0.0);
            b2=tok.nextDouble(true, 0.0);
            b3=tok.nextDouble(true, 0.0);
            b4=tok.nextDouble(true, 0.0);
            b5=tok.nextDouble(true, 0.0);
            b6=tok.nextDouble(true, 0.0);
        } else if (flag=="GE") { // Structural Damping constants (Default 0.0)
            ge1=tok.nextDouble(true, 0.0);
            ge2=tok.nextDouble(true, 0.0);
            ge3=tok.nextDouble(true, 0.0);
            ge4=tok.nextDouble(true, 0.0);
            ge5=tok.nextDouble(true, 0.0);
            ge6=tok.nextDouble(true, 0.0);
        } else if(flag=="RCV") { // Stress and Strain recovery coefficient (Default 1.0)
            sa=tok.nextDouble(true, 1.0);
            st=tok.nextDouble(true, 1.0);
            ea=tok.nextDouble(true, 1.0);
            et=tok.nextDouble(true, 1.0);
        } else {
            handleParsingWarning("unknown PBUSH flag: " + flag, tok, model);
        }
    }
    if (!is_equal(ge1, 0) || !is_equal(ge2, 0) || !is_equal(ge3, 0) || !is_equal(ge4, 0)
            || !is_equal(ge5, 0) || !is_equal(ge6, 0) ) {
        ge1=0.0; ge2=0.0; ge3=0.0; ge4=0.0; ge5=0.0; ge6=0.0;
        handleParsingWarning(string("Structural PBUSH Damping constants GE not supported. Default (0.0) assumed."), tok, model);
    }
    if (!is_equal(sa, 1.0) || !is_equal(st, 1.0) || !is_equal(ea, 1.0) || !is_equal(et,1.0)) {
        sa=1.0; st=1.0; ea=1.0; et=1.0;
        handleParsingWarning(string("Stress and Strain PBUSH recovery coefficients (SA, ST, EA, ET ) not supported. Default (1.0) assumed."), tok, model);
    }

    const auto& structuralElement = make_shared<StructuralSegment>(model, MatrixType::DIAGONAL, pid);
    structuralElement->add(*getOrCreateCellGroup(pid, model, "PBUSH"));
    structuralElement->addStiffness(DOF::DX, DOF::DX, k1);
    structuralElement->addStiffness(DOF::DY, DOF::DY, k2);
    structuralElement->addStiffness(DOF::DZ, DOF::DZ, k3);
    structuralElement->addStiffness(DOF::RX, DOF::RX, k4);
    structuralElement->addStiffness(DOF::RY, DOF::RY, k5);
    structuralElement->addStiffness(DOF::RZ, DOF::RZ, k6);

    structuralElement->addDamping(DOF::DX, DOF::DX, b1);
    structuralElement->addDamping(DOF::DY, DOF::DY, b2);
    structuralElement->addDamping(DOF::DZ, DOF::DZ, b3);
    structuralElement->addDamping(DOF::RX, DOF::RX, b4);
    structuralElement->addDamping(DOF::RY, DOF::RY, b5);
    structuralElement->addDamping(DOF::RZ, DOF::RZ, b6);

    structuralElement->setInputContext(tok.getInputContext());
    model.add(structuralElement);

}

void NastranParser::parsePCOMP(NastranTokenizer& tok, Model& model) {
    int pid = tok.nextInt();
    double z0 = tok.nextDouble(true, Globals::UNAVAILABLE_DOUBLE);
    if (tok.isNextEmpty(6)) {
        tok.skip(6);
    } else {
        handleParsingError("PCOMP fields not yet handled", tok, model);
    }
    const auto& composite = make_shared<Composite>(model, pid);
    int mid1 = tok.nextInt();
    composite->assignMaterial(Reference<Material>(Material::Type::MATERIAL,mid1));
    double t1 = tok.nextDouble();
    double theta1 = tok.nextDouble(true, 0.0);
    tok.skip(1); // SOUT1
    composite->addLayer(mid1, t1, theta1);
    while(not tok.isNextEmpty(4) and not tok.isEmptyUntilNextKeyword()) {
        int midn = tok.nextInt(true, mid1);
        composite->assignMaterial(Reference<Material>(Material::Type::MATERIAL,midn));
        double tn = tok.nextDouble(true, t1);
        double thetan = tok.nextDouble(true, theta1);
        tok.skip(1); // SOUTn
        composite->addLayer(midn, tn, thetan);
    }
    if (is_equal(z0, Globals::UNAVAILABLE_DOUBLE)) {
        composite->offset = composite->getTotalThickness() / 2.0; // default -1/2 element thickness
    } else {
        composite->offset = z0;
    }
    composite->setInputContext(tok.getInputContext());
    composite->add(*getOrCreateCellGroup(pid, model, "PCOMP"));
    model.add(composite);
}

void NastranParser::parsePDAMP(NastranTokenizer& tok, Model& model) {

    int nbProperties=0;
    // Up to four elastic damper properties can be defined on a single entry.
    while ((tok.isNextInt())&&(nbProperties<4)){

        const int pid = tok.nextInt();
        const double b = tok.nextDouble(true, 0.0);

        const auto& cellGroup = getOrCreateCellGroup(pid, model, "PDAMP");
        const auto& elementSet = model.elementSets.find(pid);
        if (elementSet == nullptr){
            const auto& scalarSpring = make_shared<ScalarSpring>(model, pid, Globals::UNAVAILABLE_DOUBLE, b);
            scalarSpring->add(*cellGroup);
            scalarSpring->setInputContext(tok.getInputContext());
            model.add(scalarSpring);
        } else {
            if (elementSet->type == ElementSet::Type::SCALAR_SPRING){
                const auto& springElementSet = static_pointer_cast<ScalarSpring>(elementSet);
                springElementSet->setDamping(b);
                springElementSet->add(*cellGroup);
            } else {
                handleParsingError("The part of PID " + to_string(pid) + " already exists with the wrong NATURE.", tok, model);
            }
        }
        nbProperties++;
    }
}

void NastranParser::parsePELAS(NastranTokenizer& tok, Model& model) {

    int nbProperties=0;
    // One or two elastic spring properties can be defined on a single entry.
    while (tok.isNextInt() and (nbProperties<2)){

        const int pid = tok.nextInt();
        const double k = tok.nextDouble(true, 0.0);
        const double ge= tok.nextDouble(true, 0.0);
        const double s = tok.nextDouble(true);

        // S is only used for post-treatment, and so discarded.
        if (not is_equal(s, Globals::UNAVAILABLE_DOUBLE)){
            if (this->logLevel >= LogLevel::DEBUG) {
                handleParsingWarning("Stress coefficient (S) is only used for post-treatment and dismissed.", tok, model);
            }
        }

        const auto& cellGroup = getOrCreateCellGroup(pid, model, "PELAS");
        const auto& elementSet = model.elementSets.find(pid);
        if (elementSet == nullptr){
            const auto& scalarSpring = make_shared<ScalarSpring>(model, pid, k, ge);
            scalarSpring->add(*cellGroup);
            scalarSpring->setInputContext(tok.getInputContext());
            model.add(scalarSpring);
        } else {
            if (elementSet->type == ElementSet::Type::SCALAR_SPRING){
                const auto& springElementSet = static_pointer_cast<ScalarSpring>(elementSet);
                springElementSet->setStiffness(k);
                springElementSet->setDamping(ge);
                springElementSet->add(*cellGroup);
            } else {
                handleParsingError("The part of PID "+std::to_string(pid)+" already exists with the wrong NATURE.", tok, model);
            }
        }
        nbProperties++;
    }
}

void NastranParser::parsePGAP(NastranTokenizer& tok, Model& model) {
    int pid = tok.nextInt();
    double u0 = tok.nextDouble(true, 0.0);
    double f0 = tok.nextDouble(true, 0.0);
    if (!is_equal(f0, 0)) {
        handleParsingWarning(string("Unsupported f0 ") + to_string(f0), tok, model);
        f0 = 0.0;
    }
    double ka = tok.nextDouble();
    handleParsingWarning(string("Ignored ka ") + to_string(ka), tok, model);

    // A bunch of parameters that VEGA does not use. To keep for completion and warning
    double kb = tok.nextDouble(true, 0.0001*ka); UNUSEDV(kb);
    double kt = tok.nextDouble(true);
    double mu1 = tok.nextDouble(true, 0.0);
    double mu2 = tok.nextDouble(true, mu1); UNUSEDV(mu2);
    tok.skip(1);
    double tmax = tok.nextDouble(true, 0.0); UNUSEDV(tmax);
    double mar = tok.nextDouble(true, 100); UNUSEDV(mar);
    double trmin = tok.nextDouble(true, 0.001); UNUSEDV(trmin);
    if (is_equal(kt, Globals::UNAVAILABLE_DOUBLE)){
        kt = mu1*ka;
    }

    shared_ptr<Constraint> gapPtr = model.find(Reference<Constraint>(Constraint::Type::GAP, pid));
    if (gapPtr == nullptr) {
        const auto& gapConstraint = make_shared<GapTwoNodes>(model, pid);
        gapConstraint->initial_gap_opening = u0;
        gapConstraint->setInputContext(tok.getInputContext());
        model.add(gapConstraint);
        model.addConstraintIntoConstraintSet(*gapConstraint, *model.commonConstraintSet);
    } else {
        const auto& gap = static_pointer_cast<GapTwoNodes>(gapPtr);
        gap->initial_gap_opening = u0;
    }
}

void NastranParser::parsePLOAD(NastranTokenizer& tok, Model& model) {
    int loadset_id = tok.nextInt();
    double p = tok.nextDouble();
    int g1 = tok.nextInt();
    int g2 = tok.nextInt();
    int g3 = tok.nextInt();
    int g4 = tok.nextInt(true, Globals::UNAVAILABLE_INT);
    const auto& loadSet = model.getOrCreateLoadSet(loadset_id, LoadSet::Type::LOAD);
    const auto& staticPressure = make_shared<StaticPressure>(model, loadSet, g1, g2, g3, g4, p);
    staticPressure->setInputContext(tok.getInputContext());
    model.add(staticPressure);
}

void NastranParser::parsePLOAD1(NastranTokenizer& tok, Model& model) {
    int loadset_id = tok.nextInt();
    int eid = tok.nextInt();
    string type = tok.nextString();
    string scale = tok.nextString();
    const auto& force = make_shared<FunctionTable>(model, FunctionTable::Interpolation::LINEAR, FunctionTable::Interpolation::LINEAR, FunctionTable::Interpolation::NONE, FunctionTable::Interpolation::NONE);
    DOF dof = DOF::DX;
    double x1 = tok.nextDouble(true, 0.0);
    double p1 = tok.nextDouble(true, 0.0);
    double x2 = tok.nextDouble(true, -1.0);
    double p2 = tok.nextDouble(true, 0.0);
    int smalldistancefactor = 1000;

    if (type == "FX")
        dof = DOF::DX;
    else if (type == "FY")
        dof = DOF::DY;
    else if (type == "FZ")
        dof = DOF::DZ;
    else if (type == "MX")
        dof = DOF::RX;
    else if (type == "MY")
        dof = DOF::RY;
    else if (type == "MZ")
        dof = DOF::RZ;
    else
        handleParsingError(string("PLOAD1 TYPE not yet implemented."), tok, model);

    double effx1 = x1, effx2 = x2, effp1 = p1, effp2 = p2;

    if (scale == "LE") {
        /* If SCALE = LE, the total load applied to the bar is P1(X2 - X1) in the yb direction. */
        force->setParaX(FunctionTable::ParaName::PARAX);
        effx1 = x1;
        effx2 = x2;
        effp1 = p1;
        effp2 = p2;
    } else if (scale == "FRPR" or scale == "FR") {
        /* If SCALE = FRPR (fractional projected), the Xi values are ratios of the actual distance to the length of the bar
         * and (X1 ≠ X2) the distributed load is input in terms of the projected length of the bar.*/
        // TODO LD: encapsulate all this in mesh/cell/node (but it needs model)
        force->setParaX(FunctionTable::ParaName::ABSC);
//        int cellPos = model.mesh.findCellPosition(eid);
//        const Cell& cell = model.mesh.findCell(cellPos);
//        std::shared_ptr<OrientationCoordinateSystem> ocs = cell.orientation;
//        const Node& firstNode = model.mesh.findNode(cell.nodePositions.front());
//        const Node& lastNode = model.mesh.findNode(cell.nodePositions.back());
//        const VectorialValue& barvect = VectorialValue(lastNode.x - firstNode.x, lastNode.y - firstNode.y, lastNode.z - firstNode.z);
//        double dist = barvect.norm();
        /* If SCALE = LE (length), the Xi values are actual distances along the bar x-axis,
         * and (if X1 ≠ X2) Pi are load intensities per unit length of the bar. */
//        effx1 = x1 * dist;
//        effx2 = x2 * dist;
        effx1 = x1;
        effx2 = x2;
        effp1 = p1;
        effp2 = p2;
    } else {
        handleParsingError(string("PLOAD1 SCALE not yet implemented."), tok, model);
    }

    if (effx1 > 0.0)
        force->setXY(effx1 - effx1 / smalldistancefactor, 0.0);
    if (x2 < 0 || is_equal(x2, x1)) {
        // If X2 is blank or equal to X1, a concentrated load of value P1 will be applied at position X1.
        //effp1 = effp1 * 2 / (2 * effx1 / smalldistancefactor); // compunting equivalent triangle area to get the resultant force
        //force->setXY(effx1, effp1);
        //force->setXY(effx1 + effx1 / smalldistancefactor, 0.0);
        // TODO : this does not seem to work, should split beam and add one node, then apply nodal force
        handleParsingError(string("Concentrated force not yet handled."), tok, model);
    } else {
        // TODO LD: this won't do, really need an absolute space function (at least in the writer)
        force->setXY(effx1, effp1);
        force->setXY(effx2, effp2);
        force->setXY(effx2 + effx2 / smalldistancefactor, 0.0);
    }
    force->setInputContext(tok.getInputContext());
    model.add(force);
    const auto& loadSet = model.getOrCreateLoadSet(loadset_id, LoadSet::Type::LOAD);
    const auto& forceLine = make_shared<ForceLine>(model, loadSet, force, dof);
    forceLine->addCellId(eid);
    forceLine->setInputContext(tok.getInputContext());
    model.add(forceLine);
}

void NastranParser::parsePLOAD2(NastranTokenizer& tok, Model& model) {
    int loadset_id = tok.nextInt();
    double p = tok.nextDouble();
    const auto& loadSet = model.getOrCreateLoadSet(loadset_id, LoadSet::Type::LOAD);
    // https://knowledge.autodesk.com/support/nastran/learn-explore/caas/CloudHelp/cloudhelp/2019/ENU/NSTRN-Reference/files/GUID-DC59FE67-D314-4E68-B9BC-10790C755D32-htm.html
    // The direction of the pressure is computed according to the right-hand rule
    // using the grid point sequence specified on the element entry.
    const auto& normalPressionFace = make_shared<NormalPressionShell>(model, loadSet, p);
    normalPressionFace->addCellIds(tok.nextInts());
    normalPressionFace->setInputContext(tok.getInputContext());
    model.add(normalPressionFace);
}

void NastranParser::parsePLOAD4(NastranTokenizer& tok, Model& model) {
    int loadset_id = tok.nextInt();
    int eid1 = tok.nextInt();
    double p1 = tok.nextDouble();
    double p2 = tok.nextDouble(true, p1);
    double p3 = tok.nextDouble(true, p1);
    double p4 = tok.nextDouble(true, p1);
    if (!is_equal(p2, p1) || !is_equal(p3, p1) || !is_equal(p4, p1)) {
        handleParsingWarning("Non uniform pressure not implemented.", tok, model);
    }
    bool format1 = tok.isNextInt() || tok.isNextEmpty() || tok.isEmptyUntilNextKeyword();
    int g1 = Globals::UNAVAILABLE_INT;
    int g3_or_4 = Globals::UNAVAILABLE_INT;
    int eid2 = Globals::UNAVAILABLE_INT;
    if (format1) {
        //format 1
        g1 = tok.nextInt(true);
        g3_or_4 = tok.nextInt(true);
        eid2 = eid1;
    } else {
        if (tok.isNextTHRU()) {
            //format2
            tok.skip(1);
            eid2 = tok.nextInt();
        } else {
            //format not recognized
            handleParsingError("Format not recognized.", tok, model);
        }
    }
    int cid = tok.nextInt(true, CoordinateSystem::GLOBAL_COORDINATE_SYSTEM_ID);
    if (cid != CoordinateSystem::GLOBAL_COORDINATE_SYSTEM_ID) {
        handleParsingWarning("CoordinateSystem not supported.", tok, model);
    }
    double n1 = tok.nextDouble(true, 0.0);
    double n2 = tok.nextDouble(true, 0.0);
    double n3 = tok.nextDouble(true, 0.0);

    string sorl = tok.nextString(true, "SURF");
    if (sorl != "SURF"){
        handleParsingWarning("SORL field not supported: default (SURF) assumed.", tok, model);
    }
    string ldir = tok.nextString(true, "NORM");
    if (ldir != "NORM"){
        handleParsingWarning("LDIR field not supported: default (NORM) assumed.", tok, model);
    }

    const auto& loadSet = model.getOrCreateLoadSet(loadset_id, LoadSet::Type::LOAD);
    bool has_direction = not (is_equal(n1, 0.0) and is_equal(n2, 0.0) and is_equal(n3, 0.0)
                              and cid == CoordinateSystem::GLOBAL_COORDINATE_SYSTEM_ID);
    if (not has_direction and g1 == Globals::UNAVAILABLE_INT) {
        const auto& normalPressionFace = make_shared<NormalPressionFace>(model, loadSet, p1);
        for(int cellId = eid1; cellId <= eid2; cellId++) {
            normalPressionFace->addCellId(cellId);
        }
        normalPressionFace->setInputContext(tok.getInputContext());
        model.add(normalPressionFace);
    } else if (not has_direction and g1 != Globals::UNAVAILABLE_INT) {
        // For the faces of solid elements, the direction of positive pressure (defaulted continuation)
        // is inward
        shared_ptr<NormalPressionFaceTwoNodes> pressionFaceTwoNodes = nullptr;
        if (g3_or_4 != Globals::UNAVAILABLE_INT) {
            pressionFaceTwoNodes = make_shared<NormalPressionFaceTwoNodes>(model, loadSet, g1, g3_or_4,
                -p1);
        } else {
            pressionFaceTwoNodes = make_shared<NormalPressionFaceTwoNodes>(model, loadSet, g1,
                -p1);
        }
        for(int cellId = eid1; cellId <= eid2; cellId++) {
            pressionFaceTwoNodes->addCellId(cellId);
        }

        pressionFaceTwoNodes->setInputContext(tok.getInputContext());
        model.add(pressionFaceTwoNodes);
    } else if (has_direction and g1 != Globals::UNAVAILABLE_INT) {
        shared_ptr<ForceSurfaceTwoNodes> forceSurfaceTwoNodes = nullptr;
        if (g3_or_4 != Globals::UNAVAILABLE_INT) {
            forceSurfaceTwoNodes = make_shared<ForceSurfaceTwoNodes>(model, loadSet, g1, g3_or_4,
			VectorialValue(n1 * p1, n2 * p1, n3 * p1), VectorialValue(0, 0, 0));
        } else {
            forceSurfaceTwoNodes = make_shared<ForceSurfaceTwoNodes>(model, loadSet, g1,
			VectorialValue(n1 * p1, n2 * p1, n3 * p1), VectorialValue(0, 0, 0));
        }
        for(int cellId = eid1; cellId <= eid2; cellId++) {
            forceSurfaceTwoNodes->addCellId(cellId);
        }

        forceSurfaceTwoNodes->setInputContext(tok.getInputContext());
        model.add(forceSurfaceTwoNodes);
    } else {
        const auto& forceSurface = make_shared<ForceSurface>(model, loadSet, VectorialValue(n1 * p1, n2 * p1, n3 * p1),
                VectorialValue(0.0, 0.0, 0.0));
        for(int cellId = eid1; cellId <= eid2; cellId++) {
            forceSurface->addCellId(cellId);
        }

        forceSurface->setInputContext(tok.getInputContext());
        model.add(forceSurface);
    }
}

void NastranParser::parsePLSOLID(NastranTokenizer& tok, Model& model) {
    int pid = tok.nextInt();
    int mid = tok.nextInt();
    /*
     * Location selection for stress output.
     * Stress output may be requested at the Gauss points (STRESS = “GAUSS” or
     * 1) of CHEXA and CPENTA elements with no midside nodes. Gauss point
     * output is available for the CTETRA element with or without midside nodes.
     *
     */
    string stress = tok.nextString(true, "GRID");
    if (stress != "GRID") {
        handleParsingWarning("STRESS field " + stress + " not supported", tok, model);
    }
    // TODO LD: add large strain and large rotation somewhere, to be used in COMPORTEMENT
    const auto& continuum = make_shared<Continuum>(model, ModelType::TRIDIMENSIONAL, pid);
    continuum->assignMaterial(Reference<Material>(Material::Type::MATERIAL, mid));
    continuum->add(*getOrCreateCellGroup(pid, model, "PLSOLID"));
    continuum->setInputContext(tok.getInputContext());
    model.add(continuum);
}

void NastranParser::parsePROD(NastranTokenizer& tok, Model& model) {
    int propId = tok.nextInt();
    int mid = tok.nextInt();
    double a = tok.nextDouble();
    double j = tok.nextDouble(true, 0.0);
    double c = tok.nextDouble(true, 0.0);
    double nsm = tok.nextDouble(true, 0.0);
    if (!is_equal(c, 0)) {
        handleParsingWarning("Stress coefficient (C) not supported and dismissed.", tok, model);
    }
    if (!is_equal(nsm, 0)) {
        handleParsingWarning("Non Structural mass (NSM) not supported and dismissed.", tok, model);
    }
    double equivalent_inertia_moment = pow(a, 2) / 4 / boost::math::constants::pi<double>(); // Using circular beam formula
    const auto& genericSectionBeam = make_shared<GenericSectionBeam>(model, a, equivalent_inertia_moment, equivalent_inertia_moment, j, 1.0, 1.0, GenericSectionBeam::BeamModel::TRUSS, nsm,
            propId);
    genericSectionBeam->assignMaterial(Reference<Material>(Material::Type::MATERIAL, mid));
    genericSectionBeam->add(*getOrCreateCellGroup(propId, model, "PROD"));
    genericSectionBeam->setInputContext(tok.getInputContext());
    model.add(genericSectionBeam);
}

void NastranParser::parsePSHELL(NastranTokenizer& tok, Model& model) {
    int propId = tok.nextInt();
    int mid1 = tok.nextInt();
    double thickness = tok.nextDouble(true, 0.0);
    int mid2 = tok.nextInt(true);
    if (mid2 != Globals::UNAVAILABLE_INT && mid2 != mid1) {
        handleParsingWarning("Material 2 not yet supported and dismissed.", tok, model);
    }
    double bending_moment = tok.nextDouble(true, 1.0);
    if (!is_equal(bending_moment, 1.0)) {
        handleParsingWarning("Bending moment of inertia ratio != 1.0 not supported", tok,
                model);
    }
    int mid3 = tok.nextInt(true);
    if (mid3 != Globals::UNAVAILABLE_INT && mid3 != mid1) {
        handleParsingWarning("Material 3 not yet supported and dismissed.", tok, model);
    }
    double ts_t_ratio = tok.nextDouble(true, 0.833333);
    if (!is_equal(ts_t_ratio, 0.833333)) {
        handleParsingWarning("ts/t ratio !=  0.833333 not supported", tok, model);
    }
    double nsm = tok.nextDouble(true, 0);
    double z1 = tok.nextDouble(true);
    double z2 = tok.nextDouble(true);
    if (!is_equal(z1, Globals::UNAVAILABLE_DOUBLE)
            || !is_equal(z2, Globals::UNAVAILABLE_DOUBLE)) {
        handleParsingWarning("Fiber distances z1,z2 not supported", tok, model);
    }
    int mid4 = tok.nextInt(true);
    if (mid4 != Globals::UNAVAILABLE_INT && mid4 != mid1) {
        handleParsingWarning("Material 4 not yet supported and dismissed.", tok, model);
    }

    const auto& shell = make_shared<Shell>(model, thickness, nsm, 0.0, propId);
    shell->assignMaterial(Reference<Material>(Material::Type::MATERIAL, mid1));
    shell->add(*getOrCreateCellGroup(propId, model,"PSHELL"));
    shell->setInputContext(tok.getInputContext());
    model.add(shell);
}

void NastranParser::parsePSOLID(NastranTokenizer& tok, Model& model) {
    int elemId = tok.nextInt();
    int mid = tok.nextInt();
    int material_coordinate_system = tok.nextInt(true, CoordinateSystem::GLOBAL_COORDINATE_SYSTEM_ID);
    if (material_coordinate_system != CoordinateSystem::GLOBAL_COORDINATE_SYSTEM_ID) {
        handleParsingWarning("Material coordinate system!=0 not supported and dismissed.", tok, model);
    }
    string psolid_in = tok.nextString(true, "BUBBLE");
    /*
     * Location selection for stress output.
     * Stress output may be requested at the Gauss points (STRESS = “GAUSS” or
     * 1) of CHEXA and CPENTA elements with no midside nodes. Gauss point
     * output is available for the CTETRA element with or without midside nodes.
     *
     */
    string stress = tok.nextString(true, "GRID");
    if (stress != "GRID") {
        handleParsingWarning("STRESS field " + stress + " not supported", tok, model);
    }
    string isop = tok.nextString(true, "REDUCED");
    const ModelType * modelType;
    if (psolid_in == "BUBBLE" && isop == "REDUCED") {
        modelType = &ModelType::TRIDIMENSIONAL_SI;
    } else if ((psolid_in == "TWO" || psolid_in == "2") && (isop == "FULL" || isop == "1")) {
        modelType = &ModelType::TRIDIMENSIONAL;
    } else if ((psolid_in == "THREE" || psolid_in == "3") && (isop == "FULL" || isop == "1")) {
        modelType = &ModelType::TRIDIMENSIONAL;
    } else {
        modelType = &ModelType::TRIDIMENSIONAL_SI;
        handleParsingWarning("Integration IN " + psolid_in + " ISOP " + isop + " not implemented : default assumed",
                tok, model);
    }
    string fctn = tok.nextString(true, "SMECH");
    if (fctn != "SMECH") {
        handleParsingWarning("PSOLID fctn " + fctn + " Not implemented", tok, model);
    }
    const auto& continuum = make_shared<Continuum>(model, *modelType, elemId);
    continuum->assignMaterial(Reference<Material>(Material::Type::MATERIAL, mid));
    continuum->add(*getOrCreateCellGroup(elemId, model, "PSOLID"));
    continuum->setInputContext(tok.getInputContext());
    model.add(continuum);
}

void NastranParser::parseRBAR(NastranTokenizer& tok, Model& model) {
    int original_id=tok.nextInt();
    int ga = tok.nextInt();
    int gb = tok.nextInt();
    int cna = tok.nextInt(true, 0);
    int cnb = tok.nextInt(true, 0);
    int cma = tok.nextInt(true, 0);
    int cmb = tok.nextInt(true, 0);
    if (cna != 0 && cnb != 0) {
        // LD note: in this (rare) case both nodes are (partially) master and slave at the same time (depending on dofs)
        handleParsingError("cna & cnb both specified.", tok, model);
    } else if (cna == 0 && cnb == 0) {
        cna = 123456;
    }
    if (cna != 0) {
        const auto& qrc = make_shared<QuasiRigidConstraint>(model, DOFS::nastranCodeToDOFS(cna), ga, original_id);
        qrc->addSlave(gb);
        qrc->setInputContext(tok.getInputContext());
        model.add(qrc);
        model.addConstraintIntoConstraintSet(*qrc, *model.commonConstraintSet);
    } else if (cnb != 0) {
        const auto& qrc = make_shared<QuasiRigidConstraint>(model, DOFS::nastranCodeToDOFS(cnb), ga, original_id);
        qrc->addSlave(gb);
        qrc->setInputContext(tok.getInputContext());
        model.add(qrc);
        model.addConstraintIntoConstraintSet(*qrc, *model.commonConstraintSet);
    }
    if (cma != 0 || cmb != 0) {
        handleParsingWarning("cma or cmb not supported.", tok, model);
    }
    double alpha = tok.nextDouble(true);
    if (!is_equal(alpha, Globals::UNAVAILABLE_DOUBLE)) {
        handleParsingWarning("ALPHA field: " + to_string(alpha) + " not supported",
                tok, model);
    }

}

void NastranParser::parseRBAR1(NastranTokenizer& tok, Model& model) {
    int original_id=tok.nextInt();
    int ga = tok.nextInt();
    int gb = tok.nextInt();
    int cna = tok.nextInt(true, 0);

    const auto& qrc = make_shared<QuasiRigidConstraint>(model, DOFS::nastranCodeToDOFS(cna), MasterSlaveConstraint::UNAVAILABLE_MASTER, original_id);
    qrc->addSlave(ga);
    qrc->addSlave(gb);
    qrc->setInputContext(tok.getInputContext());
    model.add(qrc);
    model.addConstraintIntoConstraintSet(*qrc, *model.commonConstraintSet);

    double alpha = tok.nextDouble(true);
    if (!is_equal(alpha, Globals::UNAVAILABLE_DOUBLE)) {
        handleParsingError("RBAR1: ALPHA field: " + to_string(alpha) + " not supported",
                tok, model);
    }

}

void NastranParser::parseRBE2(NastranTokenizer& tok, Model& model) {
    int original_id = tok.nextInt();
    int masterId = tok.nextInt();
    int cm = tok.nextInt();
    const DOFS& dofs = DOFS::nastranCodeToDOFS(cm);
    auto constraint = make_shared<QuasiRigidConstraint>(model, dofs, masterId, original_id);
    constraint->addRotations = true;

    while (tok.isNextInt()) {
        constraint->addSlave(tok.nextInt());
    }
    constraint->setInputContext(tok.getInputContext());
    model.add(constraint);
    model.addConstraintIntoConstraintSet(*constraint, *model.commonConstraintSet);

    double alpha = tok.nextDouble(true);
    if (!is_equal(alpha, Globals::UNAVAILABLE_DOUBLE)) {
        handleParsingWarning("ALPHA field: " + to_string(alpha) + " not supported",
                tok, model);
    }
}

void NastranParser::parseRBE3(NastranTokenizer& tok, Model& model) {
    int original_id = tok.nextInt();
    tok.skip(1); // ignoring blank
    int masterId = tok.nextInt();
    int nastranDofs = tok.nextInt();
    const DOFS& dofs = DOFS::nastranCodeToDOFS(nastranDofs);
    const auto& rbe3 = make_shared<RBE3>(model, masterId, dofs, original_id);
    while (tok.isNextDouble()) {
        double coef = tok.nextDouble();
        int nastranSDofs = tok.nextInt();
        const DOFS& sdofs = DOFS::nastranCodeToDOFS(nastranSDofs);
        while (tok.isNextInt()) {
            int slaveId = tok.nextInt();
            rbe3->addRBE3Slave(slaveId, sdofs, coef);
        }
    }
    rbe3->setInputContext(tok.getInputContext());
    model.add(rbe3);
    model.addConstraintIntoConstraintSet(*rbe3, *model.commonConstraintSet);
}

void NastranParser::parseRFORCE(NastranTokenizer& tok, Model& model) {
    // RFORCE  2       1               200.    0.0     0.0     1.0     2
    int sid = tok.nextInt();
    int g = tok.nextInt();
    int csid = tok.nextInt(true, CoordinateSystem::GLOBAL_COORDINATE_SYSTEM_ID);
    if (csid != CoordinateSystem::GLOBAL_COORDINATE_SYSTEM_ID) {
        csid = CoordinateSystem::GLOBAL_COORDINATE_SYSTEM_ID;
        handleParsingWarning("CoordinateSystem not supported and taken as 0.", tok, model);
    }
    double a = tok.nextDouble();
    double r1 = tok.nextDouble();
    double r2 = tok.nextDouble();
    double r3 = tok.nextDouble();

    int method = tok.nextInt(true, 1);
    if (method != 1) {
        handleParsingWarning("METHOD not supported. Default (1) assumed.", tok, model);
    }

    double racc = tok.nextDouble(true, 0);
    if (!is_equal(racc, 0.0)) {
        handleParsingWarning("Scale factor of angular acceleration (RACC) not supported. Default (0.0) assumed.", tok, model);
    }

    int mb = tok.nextInt(true, 0);
    if (mb != 0) {
        handleParsingWarning("MB not supported. Default (0) assumed.", tok, model);
    }
    const auto& loadset = model.getOrCreateLoadSet(sid, LoadSet::Type::LOAD);
    const auto& rotation = make_shared<RotationNode>(model, loadset, a, g, r1, r2, r3);

    rotation->setInputContext(tok.getInputContext());
    model.add(rotation);
}

void NastranParser::parseRLOAD1(NastranTokenizer& tok, Model& model) {
    int loadset_id = tok.nextInt();
    int darea_set_id = tok.nextInt();

    // Delay
    int delay_id = 0;
    double delay = 0.0;
    if (tok.isNextInt())
        delay_id = tok.nextInt(true, 0);
    else
        delay = tok.nextDouble(true, 0.0);

    // DPhase
    int dphase_id = 0;
    double dphase = 0.0;
    if (tok.isNextInt())
        dphase_id = tok.nextInt(true, 0);
    else
        dphase = tok.nextDouble(true, 0.0);

    int functionTableC_original_id = tok.nextInt(); //TC
    int functionTableD_original_id = tok.nextInt(true, 0); //TD
    if (functionTableD_original_id != 0)
      handleParsingError("TYPE in RLOAD1 not supported. ", tok, model);

    // Type
    string type = tok.nextString(true, "LOAD").substr(0, 1);
    DynamicExcitation::DynamicExcitationType excitType;
    if (type == "L" or type == "0" or type == "LOAD") {
        excitType = DynamicExcitation::DynamicExcitationType::LOAD;
    } else if (type == "D" or type == "1" or type == "DISP") {
        excitType = DynamicExcitation::DynamicExcitationType::DISPLACEMENT;
    } else if (type == "V" or type == "2" or type == "VELO") {
        excitType = DynamicExcitation::DynamicExcitationType::VELOCITY;
    } else if (type == "A" or type == "3" or type == "ACCE") {
        excitType = DynamicExcitation::DynamicExcitationType::ACCELERATION;
    } else {
        handleParsingError("TYPE " + type + " not yet implemented in RLOAD2.", tok, model);
    }

    Reference<NamedValue> dynaDelay_ref = Reference<NamedValue>(Value::Type::DYNA_PHASE, delay_id);
    if (delay_id == 0) {
        const auto& dynadelay = make_shared<DynaPhase>(model, -2*M_PI*delay);
        dynadelay->setInputContext(tok.getInputContext());
        model.add(dynadelay);
        dynaDelay_ref = dynadelay->getReference();
    }
    Reference<NamedValue> dynaPhase_ref = Reference<NamedValue>(Value::Type::DYNA_PHASE, dphase_id);
    if (dphase_id == 0) {
        const auto& dynaphase = make_shared<DynaPhase>(model, dphase);
        dynaphase->setInputContext(tok.getInputContext());
        model.add(dynaphase);
        dynaPhase_ref = dynaphase->getReference();
    }
    Reference<NamedValue> functionTableC_ref(Value::Type::FUNCTION_TABLE, functionTableC_original_id);
    Reference<NamedValue> functionTableD_ref(Value::Type::FUNCTION_TABLE, functionTableD_original_id);

    // If needed, creates a LoadSet EXCITEID for the DynamicExcitation
    shared_ptr<LoadSet> darea = model.getOrCreateLoadSet(darea_set_id, LoadSet::Type::EXCITEID);

    // if loadSet DLOAD does not exist (was not declared in the bulk), loadset_id become the original id of DynamicExcitation
    // else DynamicExcitation is created without original_id and is mapped to this loadSet
    int original_id;
    Reference<LoadSet> loadSetReference(LoadSet::Type::DLOAD, loadset_id);
    if (model.find(loadSetReference) != nullptr)
        original_id = Loading::NO_ORIGINAL_ID;
    else
        original_id = loadset_id;

    const auto& loadSet = model.getOrCreateLoadSet(loadset_id, LoadSet::Type::DLOAD);
    const auto& dynamicExcitation = make_shared<DynamicExcitation>(model, loadSet, dynaDelay_ref, dynaPhase_ref, functionTableC_ref, functionTableD_ref, darea->getReference(),
            excitType, original_id);
    dynamicExcitation->setInputContext(tok.getInputContext());
    model.add(dynamicExcitation);

    // PlaceHolder to complete the Value attribute paraX of FunctionTable
    dynamicExcitation->getFunctionTableBPlaceHolder()->setInputContext(tok.getInputContext());
    model.add(dynamicExcitation->getFunctionTableBPlaceHolder());
    if (functionTableD_original_id > 0){
        dynamicExcitation->getFunctionTablePPlaceHolder()->setInputContext(tok.getInputContext());
        model.add(dynamicExcitation->getFunctionTablePPlaceHolder());
    }
}

void NastranParser::parseRLOAD2(NastranTokenizer& tok, Model& model) {
    int loadset_id = tok.nextInt();
    int darea_set_id = tok.nextInt();

    // Delay
    int delay_id = 0;
    double delay = 0.0;
    if (tok.isNextInt())
        delay_id = tok.nextInt(true, 0);
    else
        delay = tok.nextDouble(true, 0.0);

    // DPhase
    int dphase_id = 0;
    double dphase = 0.0;
    if (tok.isNextInt())
        dphase_id = tok.nextInt(true, 0);
    else
        dphase = tok.nextDouble(true, 0.0);

    int functionTableB_original_id = tok.nextInt(); //TB
    int functionTableP_original_id = tok.nextInt(true, 0); //TP

    // Type
    string type = tok.nextString(true, "LOAD").substr(0, 1);
    DynamicExcitation::DynamicExcitationType excitType;
    if (type == "L" or type == "0" or type == "LOAD") {
        excitType = DynamicExcitation::DynamicExcitationType::LOAD;
    } else if (type == "D" or type == "1" or type == "DISP") {
        excitType = DynamicExcitation::DynamicExcitationType::DISPLACEMENT;
    } else if (type == "V" or type == "2" or type == "VELO") {
        excitType = DynamicExcitation::DynamicExcitationType::VELOCITY;
    } else if (type == "A" or type == "3" or type == "ACCE") {
        excitType = DynamicExcitation::DynamicExcitationType::ACCELERATION;
    } else {
        handleParsingError("TYPE " + type + " not yet implemented in RLOAD2.", tok, model);
    }

    Reference<NamedValue> dynaDelay_ref = Reference<NamedValue>(Value::Type::DYNA_PHASE, delay_id);
    if (delay_id == 0) {
        const auto& dynadelay = make_shared<DynaPhase>(model, -2*M_PI*delay);
        dynadelay->setInputContext(tok.getInputContext());
        model.add(dynadelay);
        dynaDelay_ref = dynadelay->getReference();
    }
    Reference<NamedValue> dynaPhase_ref = Reference<NamedValue>(Value::Type::DYNA_PHASE, dphase_id);
    if (dphase_id == 0) {
        const auto& dynaphase = make_shared<DynaPhase>(model, dphase);
        dynaphase->setInputContext(tok.getInputContext());
        model.add(dynaphase);
        dynaPhase_ref = dynaphase->getReference();
    }
    Reference<NamedValue> functionTableB_ref(Value::Type::FUNCTION_TABLE, functionTableB_original_id);
    //TODO: should not create a ref when the P value is 0
    Reference<NamedValue> functionTableP_ref(Value::Type::FUNCTION_TABLE, functionTableP_original_id);



    Reference<LoadSet> excitRef{LoadSet::Type::EXCITEID, darea_set_id};
    if (model.loadSets.contains(LoadSet::Type::LOADSET) and model.find(excitRef) == nullptr) {
        // If there is a LOADSET request in the Case Control, then the model will reference static and thermal load set entries specified by the LID or TID field in the selected LSEQ entries corresponding to the EXCITEID.
        auto loadsets = model.loadSets.filter(LoadSet::Type::LOADSET); // Hack should look for the indicated LOADSEQ context parameter
        const auto& lseq = loadsets[0];
//        const Reference<LoadSet>& dareaSetId = lseq->embedded_loadsets[0].first; // What about .second ?
//        handleParsingWarning("Hack in resolving RLOAD2->LSEQ->(darea id)->LOAD : " + to_string(dareaSetId.original_id) + " versus " + to_string(darea_set_id),
//                tok, model); // Hack, should seek the corresponding darea
        Reference<LoadSet> loadId = lseq->embedded_loadsets[0].first; // What about .second ?
        excitRef = loadId; // HACK?
    } else if (model.find(Reference<LoadSet>{LoadSet::Type::LOAD, darea_set_id}) != nullptr) {
        excitRef = Reference<LoadSet>{LoadSet::Type::LOAD, darea_set_id};
    } else {
        // If there is no LOADSET request in the Case Control, then EXCITEID may directly reference DAREA, static, and thermal load set entries.
        //handleParsingWarning("RLOAD2 directly referencing EXCITEID not yet handled, seems to be working?", tok, model);
    }
    // If needed, creates a LoadSet EXCITEID for the DynamicExcitation
//    LoadSet darea(model, LoadSet::Type::EXCITEID, darea_set_id);
//    Reference<LoadSet> darea_ref(darea);
//    if (!model.find(darea_ref)){
//       model.add(darea);
//    }

    // if loadSet DLOAD does not exist (was not declared in the bulk), loadset_id become the original id of DynamicExcitation
    // else DynamicExcitation is created without original_id and is mapped to this loadSet
    int original_id;
    Reference<LoadSet> loadSetReference(LoadSet::Type::DLOAD, loadset_id);
    auto loadSet = model.find(loadSetReference);
    if (loadSet == nullptr) {
        loadSet = make_shared<LoadSet>(model, LoadSet::Type::DLOAD, loadset_id);
        loadSet->setInputContext(tok.getInputContext());
        model.add(loadSet);
        original_id = loadset_id;
    } else {
        original_id = Loading::NO_ORIGINAL_ID;
    }

    const auto& dynamicExcitation = make_shared<DynamicExcitation>(model, loadSet, dynaDelay_ref, dynaPhase_ref, functionTableB_ref, functionTableP_ref, excitRef,
            excitType, original_id);
    dynamicExcitation->setInputContext(tok.getInputContext());
    model.add(dynamicExcitation);

    // PlaceHolder to complete the Value attribute paraX of FunctionTable
    dynamicExcitation->getFunctionTableBPlaceHolder()->setInputContext(tok.getInputContext());
    model.add(dynamicExcitation->getFunctionTableBPlaceHolder());
    if (functionTableP_original_id>0) {
        dynamicExcitation->getFunctionTablePPlaceHolder()->setInputContext(tok.getInputContext());
        model.add(dynamicExcitation->getFunctionTablePPlaceHolder());
    }
}

void NastranParser::parseSET1(NastranTokenizer& tok, Model& model) {
    // page 2454 Set Definition Defines a list of structural grid points or element ID's.
    int sid = tok.nextInt();
    const auto& ids = tok.nextInts();
    const auto& setValue = make_shared<SetValue<int>>(model, set<int>{ids.begin(), ids.end()}, sid);
    setValue->setInputContext(tok.getInputContext());
    model.add(setValue);
}

void NastranParser::parseSET3(NastranTokenizer& tok, Model& model) {
    // page 2457 Labeled Set Definition, defines a list of grids, elements or points.
    int sid = tok.nextInt();
    string name = "SET3_" + to_string(sid);
    string des = tok.nextString();

    if (des == "GRID") {
        shared_ptr<NodeGroup> nodeGroup = model.mesh.findOrCreateNodeGroup(name,sid,"SET3");
        const auto& ids = tok.nextInts();
        nodeGroup->addNodeIds(ids);
        const auto& setValue = make_shared<SetValue<int>>(model, set<int>{ids.begin(), ids.end()}, sid);
        setValue->markAsWritten();
        setValue->setInputContext(tok.getInputContext());
        model.add(setValue);
    } else if (des == "ELEM") {
        shared_ptr<CellGroup> cellGroup = model.mesh.createCellGroup(name,sid,"SET3");
        const auto& ids = tok.nextInts();
        cellGroup->addCellIds(ids);
        const auto& setValue = make_shared<SetValue<int>>(model, set<int>{ids.begin(), ids.end()}, sid);
        setValue->markAsWritten();
        setValue->setInputContext(tok.getInputContext());
        model.add(setValue);
    } else {
        handleParsingError("Unsupported DES value in SET3 : " + des, tok, model);
    }

}

//FIXME: SLOAD uses the CID of the Grid point to determine X... Not sure it's done here.
// The "scalar" DOF is supposed to be DOF::DX
void NastranParser::parseSLOAD(NastranTokenizer& tok, Model& model) {
    int loadset_id = tok.nextInt();
    const auto& loadSet = model.getOrCreateLoadSet(loadset_id, LoadSet::Type::LOAD);

    while (tok.isNextInt()) {
        int grid_id = tok.nextInt();
        double magnitude = tok.nextDouble();
        const auto& force1 = make_shared<NodalForce>(model, loadSet, magnitude, 0., 0., 0., 0., 0., Loading::NO_ORIGINAL_ID);
        force1->addNodeId(grid_id);
        force1->setInputContext(tok.getInputContext());
        model.add(force1);
    }
}
void NastranParser::parseSPC(NastranTokenizer& tok, Model& model) {
    int spcSet_id = tok.nextInt();
    string name = "SPC_" + to_string(spcSet_id);
    shared_ptr<NodeGroup> spcNodeGroup = model.mesh.findOrCreateNodeGroup(name,NodeGroup::NO_ORIGINAL_ID,"SPC");

    while (tok.nextSymbolType == NastranTokenizer::SymbolType::SYMBOL_FIELD) {
        const int nodeId = tok.nextInt(true);
        if (nodeId == Globals::UNAVAILABLE_INT) {
            //space at the end of line found,consume it.
            continue;
        }
        const int gi = tok.nextInt(true, 123456);
        const double displacement = tok.nextDouble(true, 0.0);
        const auto& spc = make_shared<SinglePointConstraint>(model, DOFS::nastranCodeToDOFS(gi), displacement);
        spc->addNodeId(nodeId);
        spcNodeGroup->addNodeId(nodeId);

        spc->setInputContext(tok.getInputContext());
        model.add(spc);
        model.addConstraintIntoConstraintSet(*spc,
                Reference<ConstraintSet>(ConstraintSet::Type::SPC, spcSet_id));
    }
}

void NastranParser::parseSPC1(NastranTokenizer& tok, Model& model) {
    int set_id = tok.nextInt();
    const int dofInt = tok.nextInt();

    const auto& spc = make_shared<SinglePointConstraint>(model, DOFS::nastranCodeToDOFS(dofInt), 0.0);

    // Nodes are added to the constraint Node Group
    string name = "SPC1_" + to_string(set_id);
    //shared_ptr<NodeGroup> spcNodeGroup = model.mesh.findOrCreateNodeGroup(name,NodeGroup::NO_ORIGINAL_ID,"SPC1");

    // Parsing Nodes
    spc->addNodeIds(tok.nextInts());

    // Adding the constraint to the model
    spc->setInputContext(tok.getInputContext());
    model.add(spc);
    model.addConstraintIntoConstraintSet(*spc,
            Reference<ConstraintSet>(ConstraintSet::Type::SPC, set_id));

}

void NastranParser::parseSPCADD(NastranTokenizer& tok, Model& model) {
    int set_id = tok.nextInt();
    // retrieve the ConstraintSet that was created in the executive section
    shared_ptr<ConstraintSet> constraintSet_ptr = model.find(
            Reference<ConstraintSet>(ConstraintSet::Type::SPC, set_id));

    if (!constraintSet_ptr)
        handleParsingError("ConstraintSet "+ to_string(set_id)+ " does not exist in Executive section",
                tok, model);
    else {
        while (tok.isNextInt()) {
            int constraintSet_id = tok.nextInt();
            // Need to add referenced SPCs in the same constraintset, otherwise it will not be found
            // Example: case of SPC in local coordinate system later replaced by MPCs during finish()
            // Possible alternative: make sure that these SPCs are found also with methods like
            // Model.getConstraintsByConstraintSet() etc. which seems not to be the case now
            const Reference<ConstraintSet>& constraintSetReference{ConstraintSet::Type::SPC, constraintSet_id};
            auto constraintSet = model.constraintSets.find(constraintSetReference);
            if (constraintSet == nullptr) {
                constraintSet = make_shared<ConstraintSet>(model, ConstraintSet::Type::SPC, constraintSet_id);
                constraintSet->setInputContext(tok.getInputContext());
                model.add(constraintSet);
            }
            constraintSet_ptr->add(constraintSetReference);
        }
    }
}

void NastranParser::parseSPCD(NastranTokenizer& tok, Model& model) {
    int set_id = tok.nextInt();
    const int g1 = tok.nextInt();
    const int c1 = tok.nextInt();
    const double d1 = tok.nextDouble();
    //int g1pos = model.mesh.findNodePosition(g1);
    int g2 = -1;
    int c2 = -1;
    double d2 = -1;
    //int g2pos = -1;
    if (tok.isNextInt()) {
        g2 = tok.nextInt();
        c2 = tok.nextInt();
        d2 = tok.nextDouble();
        //g2pos = model.mesh.findNodePosition(g2);
    }
    const auto& loadSet = model.getOrCreateLoadSet(set_id, LoadSet::Type::LOAD);

    const auto& spcd = make_shared<ImposedDisplacement>(model, loadSet, DOFS::nastranCodeToDOFS(c1), d1);
    spcd->addNodeId(g1);
    spcd->setInputContext(tok.getInputContext());
    model.add(spcd);
    if (g2 != -1 and c2 != -1) {
        const auto& spcd2 = make_shared<ImposedDisplacement>(model, loadSet, DOFS::nastranCodeToDOFS(c2), d2);
        spcd2->addNodeId(g2);
        spcd2->setInputContext(tok.getInputContext());
        model.add(spcd2);
    }
}

void NastranParser::parseTABDMP1(NastranTokenizer& tok, Model& model) {
    int sid = tok.nextInt();
    const auto& objectiveSet = model.getOrCreateObjectiveSet(sid, ObjectiveSet::Type::SDAMP);
    string type = tok.nextString(true, "G");
    const auto& functionTable = make_shared<FunctionTable>(model, FunctionTable::Interpolation::LINEAR, FunctionTable::Interpolation::LINEAR,
            FunctionTable::Interpolation::NONE, FunctionTable::Interpolation::CONSTANT);

    // The next 6 fields are empty (but not in free syntax).
    // We do a special treatment for the second one, which can be filled in the OPTISTRUCT syntax
    if (tok.isNextEmpty()) {
        tok.skip(1); // skip first field
        if (tok.isNextInt()) {
            int flat = tok.nextInt(); UNUSEDV(flat);
            handleParsingWarning("FLAT field is dismissed (OPTISTRUCT syntax)", tok, model);
        }
        tok.skipToNotEmpty();
    }

    double x,y;
    string sField = "";
    while (sField != "ENDT") {
        if (tok.isNextDouble()){
            x = tok.nextDouble();
        } else {
            sField = tok.nextString();
            if (sField == "ENDT"){
                continue;
            }
            if (sField == "SKIP"){ // SKIP means the pair (x,y) is skipped
                tok.skip(1);
                continue;
            } else {
                handleParsingWarning("Invalid key ("+sField+") should be ENDT, SKIP or a real.", tok, model);
                break;
            }
        }
        if (tok.isNextDouble()){
            y = tok.nextDouble();
        }else{
            string sField2 = tok.nextString(); //Code_Aster convention : Nastran is coherent with factor 2 for sdamping : not so sure
            if (sField2 == "SKIP"){
                continue;
            } else {
                handleParsingWarning("Invalid key (" + sField2 + ") should be SKIP or a real.", tok, model);
                break;
            }
        }
        functionTable->setXY(x, y);
    }

    functionTable->setParaX(Function::ParaName::FREQ);
    functionTable->setParaY(Function::ParaName::AMOR);
    ModalDamping::DampingType dampingType;
    if (type == "CRIT") {
        dampingType = ModalDamping::DampingType::CRIT;
    } else if (type == "G") {
        dampingType = ModalDamping::DampingType::G;
    } else if (type == "Q") {
        dampingType = ModalDamping::DampingType::Q;
    } else {
        handleParsingError("Damping type not yet implemented : " + type, tok, model);
    }
    const auto& modalDamping = make_shared<ModalDamping>(model, objectiveSet, *functionTable, dampingType);

    functionTable->setInputContext(tok.getInputContext());
    model.add(functionTable);
    modalDamping->setInputContext(tok.getInputContext());
    model.add(modalDamping);
}

void NastranParser::parseTABLED1(NastranTokenizer& tok, Model& model) {
    int original_id = tok.nextInt();
    string interpolation = tok.nextString(true, "LINEAR");
    FunctionTable::Interpolation parameter =
            (interpolation == "LINEAR") ? FunctionTable::Interpolation::LINEAR : FunctionTable::Interpolation::LOGARITHMIC;
    interpolation = tok.nextString(true, "LINEAR");
    FunctionTable::Interpolation value =
            (interpolation == "LINEAR") ? FunctionTable::Interpolation::LINEAR : FunctionTable::Interpolation::LOGARITHMIC;
    // The next 5 fields are empty (but not in the free syntax).
    // We do a special treatment for the first one, which can be filled in the OPTISTRUCT syntax
    int flat = -1;
    if (tok.isNextInt())
        flat = tok.nextInt();
    tok.skipToNotEmpty();


    FunctionTable::Interpolation left = FunctionTable::Interpolation::LINEAR; // Remark 6 The table look-up is performed using linear interpolation within the table and linear extrapolation outside the table using the two starting or endpoints
    FunctionTable::Interpolation right;

    if (flat != -1) {
        right = (flat != 0) ? FunctionTable::Interpolation::CONSTANT : FunctionTable::Interpolation::NONE;
    } else {
        right = FunctionTable::Interpolation::LINEAR; // Remark 6 The table look-up is performed using linear interpolation within the table and linear extrapolation outside the table using the two starting or endpoints
    }


    const auto& functionTable = make_shared<FunctionTable>(model, parameter, value, left,
            right, original_id);

    double x,y;
    string sField = "";
    while (sField != "ENDT") {
        if (tok.isNextDouble()){
            x = tok.nextDouble();
        }else{
            sField = tok.nextString();
            if (sField == "ENDT"){
                break;
            }
            if (sField == "SKIP"){ // SKIP means the pair (x,y) is skipped
                tok.skip(1);
                continue;
            }else{
                handleParsingWarning("Invalid key (" + sField + ") should be ENDT, SKIP or a real.", tok, model);
                break;
            }
        }
        if (tok.isNextDouble()){
            y = tok.nextDouble();
        }else{
            string sField2 = tok.nextString(); //Code_Aster convention : Nastran is coherent with factor 2 for sdamping : not so sure
            if (sField2 == "ENDT"){
                break;
            }
            if (sField2 == "SKIP"){
                continue;
            }else{
                handleParsingWarning("Invalid key (" + sField2 + ") should be ENDT, SKIP or a real.", tok, model);
                break;
            }
        }
        functionTable->setXY(x, y);
    }

    functionTable->setInputContext(tok.getInputContext());
    model.add(functionTable);

}

void NastranParser::parseTABLES1(NastranTokenizer& tok, Model& model) {
    int original_id = tok.nextInt();

    const auto& functionTable = make_shared<FunctionTable>(model, FunctionTable::Interpolation::LINEAR, FunctionTable::Interpolation::LINEAR, FunctionTable::Interpolation::LINEAR,
            FunctionTable::Interpolation::LINEAR, original_id);
    functionTable->setParaX(Function::ParaName::STRAIN);
    functionTable->setParaY(Function::ParaName::STRESS);

    tok.skip(7); // The next 7 fields are empty.

    double x,y;
    string sField="";
    while (sField != "ENDT") {
        if (tok.isNextDouble()){
            x = tok.nextDouble();
        } else {
            sField = tok.nextString();
            if (sField == "ENDT"){
                break;
            }
            if (sField=="SKIP"){ // SKIP means the pair (x,y) is skipped
                tok.skip(1);
                continue;
            } else {
                handleParsingWarning("Invalid key ("+sField+") should be ENDT, SKIP or a real.", tok, model);
                break;
            }
        }
        if (tok.isNextDouble()){
            y = tok.nextDouble();
        } else {
            string sField2=tok.nextString(); //Code_Aster convention : Nastran is coherent with factor 2 for sdamping : not so sure
            if (sField2=="ENDT"){
                break;
            }
            if (sField2=="SKIP"){
                continue;
            } else {
                handleParsingWarning("Invalid key ("+sField2+") should be ENDT, SKIP or a real.", tok, model);
                break;
            }
        }
        functionTable->setXY(x, y);
    }

    functionTable->setInputContext(tok.getInputContext());
    model.add(functionTable);

}

void NastranParser::parseTEMP(NastranTokenizer& tok, Model& model) {
    int sid = tok.nextInt();
    const auto& loadSet = model.getOrCreateLoadSet(sid, LoadSet::Type::LOAD);

    int g1 = tok.nextInt();
    double t1 = tok.nextDouble();
    const auto& temp1 = make_shared<InitialTemperature>(model, loadSet, t1);
    temp1->addNodeId(g1);
    temp1->setInputContext(tok.getInputContext());
    model.add(temp1);

    if (tok.isNextInt()) {
        int g2 = tok.nextInt();
        double t2 = tok.nextDouble();
        const auto& temp2 = make_shared<InitialTemperature>(model, loadSet, t2);
        temp2->addNodeId(g2);
        temp2->setInputContext(tok.getInputContext());
        model.add(temp2);
    }

    if (tok.isNextInt()) {
        int g3 = tok.nextInt();
        double t3 = tok.nextDouble();
        const auto& temp3 = make_shared<InitialTemperature>(model, loadSet, t3);
        temp3->addNodeId(g3);
        temp3->setInputContext(tok.getInputContext());
        model.add(temp3);
    }
}

dof_int NastranParser::parseDOF(NastranTokenizer& tok, Model& model, bool returnDefaultIfNotFoundOrBlank, dof_int defaultValue){

    int dofread = tok.nextInt(returnDefaultIfNotFoundOrBlank, defaultValue);

    // Check for errors
    if ((dofread!=defaultValue) && ((dofread<0) || (dofread>6))){
        handleParsingWarning("Out of bound degrees of freedom : " + std::to_string(dofread), tok, model);
    }
    // Scalar point have a "0" Nastran DOF, translated as a "0" (DX) Vega DOF
    if (dofread==0){
        return 0;
    }
    // Nastran dofs goes from 1 to 6, VEGA from 0 to 5.
    return static_cast<dof_int>(dofread-1);

}



} //namespace nastran

} //namespace vega
