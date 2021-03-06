/*
 * CommandLineUtils.cpp
 *
 *  Created on: Nov 25, 2014
 *      Author: devel
 */

#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_IGNORE_NON_ZERO_CHILD_CODE

#include "CommandLineUtils.h"
#include "../../Commandline/VegaCommandLine.h"
#include <array>
#include <cstddef>
#include <string>
#include <fstream>
#include "build_properties.h"
//dirty hack to circumvent a boost test dynamic link bug
#include <boost/test/unit_test.hpp>
#if defined(__unix__) && defined(VDEBUG) && defined __GNUC__  && !defined(_WIN32)
#include <valgrind/memcheck.h>
#endif
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/test/unit_test.hpp>

namespace vega {
namespace tests {

using namespace boost::algorithm;
using namespace std;

void CommandLineUtils::run(string inputFname, SolverName inputSolver, SolverName outputSolver,
        bool runSolver, bool strict, double tolerance, bool isNastranCosmic) {
    cout << "-------------------------------------------------------------------------------" << endl;
    cout << "----- Starting : " << boost::unit_test::framework::current_test_case().p_name << " -----" << endl;
    cout << "-------------------------------------------------------------------------------" << endl;
    string inputSolverString;
    switch (inputSolver) {
    case SolverName::NASTRAN:
        inputSolverString = "NASTRAN";
        break;
    case SolverName::OPTISTRUCT:
        inputSolverString = "OPTISTRUCT";
        break;
    default:
        BOOST_FAIL("InputSolver not recognized");
    }
    string outputSolverString;
    switch (outputSolver) {
    case SolverName::CODE_ASTER:
        outputSolverString = "ASTER";
        break;
    case SolverName::SYSTUS:
        outputSolverString = "SYSTUS";
        break;
    case SolverName::NASTRAN:
        outputSolverString = "NASTRAN";
        break;
    default:
        BOOST_FAIL("OutputSolver not recognized");
    }

    string folderPrefix = to_lower_copy(inputSolverString);
    string folderSuffix = to_lower_copy(outputSolverString);
    //code can be moved in a fixture
    string testOutputBase = string(PROJECT_BINARY_DIR "/Testing/") + folderPrefix + "2"
            + folderSuffix;
    fs::path sourceFname(string(PROJECT_BASE_DIR) + "/testdata/" + folderPrefix + inputFname);
    fs::path stem = sourceFname.stem();
    fs::path inputRelativePath = fs::path(inputFname).parent_path();
    if (not fs::exists(sourceFname)) {
        // trying to interpret input as absolute path
        sourceFname = fs::path(inputFname);
        inputRelativePath = sourceFname.parent_path().filename();
    }
    string message = "Processing : " + sourceFname.string();
    BOOST_TEST_MESSAGE(message);

    //outputPath

    fs::path outputPath = (fs::path(testOutputBase.c_str()) / inputRelativePath).make_preferred();
    const string outputPathEscaped = "\"" + outputPath.string() + "\"";

    //tests
    bool hasTests;
    string test_file;
    if (inputSolver == SolverName::NASTRAN or inputSolver == SolverName::OPTISTRUCT) {
        fs::path nastranTests = sourceFname.parent_path() / (stem.string() + ".f06");
        fs::path csvTests = sourceFname.parent_path() / (stem.string() + ".csv");
        hasTests = fs::exists(nastranTests) || fs::exists(csvTests);
        if (fs::exists(nastranTests)) {
			test_file = "\"" + nastranTests.string() + "\"";
        } else {
        	test_file = "\"" + csvTests.string() + "\"";
        }
    } else {
        hasTests = false;
    }

    // prepare command
    vector<const char*> argv1;
    argv1.push_back("vega");
    argv1.push_back("-o");
    argv1.push_back(outputPathEscaped.c_str());
    //G.C. debug output kills CDash server in large studies
    if ("ON" != TESTS_NIGHTLY_BUILD) {
        argv1.push_back("-d");
    }

    argv1.push_back("-g"); // Create graph

#if defined __unix__ && defined VDEBUG
    //disable running Aster if running under Valgrind on Linux
    runSolver = runSolver && (RUNNING_ON_VALGRIND == 0);
#endif
    if (runSolver) {
        argv1.push_back("-R"); //Run Solver
    }
    if (strict) {
        argv1.push_back("-s"); //Strict mode
    }

    string toleranceString = to_string(tolerance);
    if (hasTests) {
        argv1.push_back("-t");
        argv1.push_back(test_file.c_str());
        argv1.push_back("--tolerance");
        argv1.push_back(toleranceString.c_str());
    }
    if (not isNastranCosmic) {
        argv1.push_back("--nastran.OutputDialect=modern");
    }

    if (outputSolver == SolverName::SYSTUS) {
        argv1.push_back("--systus.RBE2TranslationMode=lagrangian");
        // cannot use RESU format when running tests with Topaze
        //argv1.push_back("--systus.OutputProduct=topaze");
    }

    string quotedSource = "\"" + sourceFname.make_preferred().string() + "\"";
    argv1.push_back(quotedSource.c_str());
    argv1.push_back(inputSolverString.c_str());
    argv1.push_back(outputSolverString.c_str());
    argv1.push_back(nullptr); // finish argv with zero

    // cleaning
    if (fs::exists(outputPath)) {
        switch (outputSolver) {
        case SolverName::CODE_ASTER:
            fs::remove(outputPath / (stem.string() + ".export"));
            fs::remove(outputPath / (stem.string() + ".comm"));
            fs::remove(outputPath / (stem.string() + ".med"));
            break;
        case SolverName::SYSTUS:
            for (fs::directory_iterator it(outputPath); it != fs::directory_iterator(); it++) {
                if (fs::is_regular_file(it->status())) {
                    string filename = it->path().filename().string();
                    if (filename.rfind(".DAT") + 4 == filename.size())
                        fs::remove(it->path());
                }
            }
            fs::remove(outputPath / (stem.string() + "_DATA1.ASC"));
            break;
        case SolverName::NASTRAN:
            fs::remove(outputPath / (stem.string() + "_vg.nas"));
            fs::remove(outputPath / (stem.string() + "_vg.nas.out"));
            fs::remove(outputPath / (stem.string() + "_vg.nas.log"));
            fs::remove(outputPath / (stem.string() + ".6"));
            fs::remove(outputPath / (stem.string() + ".f04"));
            fs::remove(outputPath / (stem.string() + ".f06"));
            fs::remove(outputPath / (stem.string() + ".log"));
            fs::remove(outputPath / (stem.string() + ".MASTER"));
            fs::remove(outputPath / (stem.string() + ".DBALL"));
            fs::remove(outputPath / (stem.string() + ".IFPDAT"));
            break;
        default:
            BOOST_FAIL("OutputSolver not recognized");
        }
    }

    // run command
    VegaCommandLine::ExitCode result;
    BOOST_TEST_CHECKPOINT(message + " about to launch vega++");
    try {
        VegaCommandLine vcl;
        result = vcl.process(static_cast<int>(argv1.size() - 1), &argv1[0]);
    } catch (exception &e) {
        result = VegaCommandLine::ExitCode::GENERIC_EXCEPTION;
        BOOST_FAIL(string("Exception threw ") + e.what() + " " + sourceFname.string());
    }
    BOOST_TEST_CHECKPOINT(
            message + " Vega++ terminated with code: " + VegaCommandLine::exitCodeToString(result));
    BOOST_CHECK_MESSAGE(result == VegaCommandLine::ExitCode::OK, VegaCommandLine::exitCodeToString(result));

    if (result == VegaCommandLine::ExitCode::OK) {
        // check results
        switch (outputSolver) {
        case SolverName::CODE_ASTER: {
            BOOST_CHECK(fs::exists(outputPath / (stem.string() + ".export")));
            BOOST_CHECK(fs::exists(outputPath / (stem.string() + ".comm")));
            BOOST_CHECK(fs::exists(outputPath / (stem.string() + ".med")));
            //comm file not cointains skipping, error, warn keywords
            string outCommFileStr = (outputPath / (stem.string() + ".comm")).string();
            vector<string> wordList = {
                    "skipping", //0
                    "warn", //1
                    "error", //2
                    "DEBUT", //3
                    "FIN", //4
                    "TEST_RESU" //5
                    };
            vector<bool> contents = containWords(outCommFileStr, wordList);
            if (strict) {
                BOOST_CHECK_MESSAGE(!contents[0], "Comm file contains the word 'skipping' ");
                BOOST_CHECK_MESSAGE(!contents[1], "Comm file contains the word 'warn'");
                BOOST_CHECK_MESSAGE(!contents[2], "Comm file contains the word 'error'");
            }
            BOOST_CHECK_MESSAGE(contents[3], "Comm file doesn't contain 'DEBUT'");
            BOOST_CHECK_MESSAGE(contents[4], "Comm file doesn't contain 'FIN'");
            if (hasTests) {
                BOOST_CHECK_MESSAGE(contents[5],
                        "Tests are enabled but no TEST_RESU found in .comm file");
            }
            break;
        }
        case SolverName::SYSTUS:
            // _ALL does not exist for topaze
            //BOOST_CHECK(fs::exists(outputPath / (stem.string() + "_ALL.DAT")));
            BOOST_CHECK(fs::exists(outputPath / (stem.string() + "_SC1_DATA1.ASC")));
            if (hasTests) {
                for (fs::directory_iterator it(outputPath); it != fs::directory_iterator(); it++) {
                    if (fs::is_regular_file(it->status())) {
                        string filename = it->path().filename().string();
                        if (filename.find(stem.string() + "_") == 0
                                && filename.rfind(".DAT") + 4 == filename.size()) {
                            string datFileStr = (outputPath / fs::path(filename)).string();                        }
                    }
                }
            }
            break;
        case SolverName::NASTRAN:
            BOOST_CHECK(fs::exists(outputPath / (stem.string() + "_vg.nas")));
            if (hasTests) {
            }
            break;
        default:
            BOOST_FAIL("OutputSolver not recognized");
        }
    }
    cout << "Leaving test: " << boost::unit_test::framework::current_test_case().p_name << endl;
}

void CommandLineUtils::nastranStudy2Aster(string fname, bool runSolver, bool strict,
        double tolerance) {
    run(fname, SolverName::NASTRAN, SolverName::CODE_ASTER, runSolver, strict, tolerance);
}

void CommandLineUtils::nastranStudy2Systus(string fname, bool runSystus, bool strict,
        double tolerance) {
    run(fname, SolverName::NASTRAN, SolverName::SYSTUS, runSystus, strict, tolerance);
}

void CommandLineUtils::nastranStudy2Nastran(string fname, bool runSolver, bool strict,
        double tolerance) {
    run(fname, SolverName::NASTRAN, SolverName::NASTRAN, runSolver, strict, tolerance);
}

void CommandLineUtils::optistructStudy2Aster(string fname, bool runSolver, bool strict,
        double tolerance) {
    run(fname, SolverName::OPTISTRUCT, SolverName::CODE_ASTER, runSolver, strict, tolerance);
}

void CommandLineUtils::optistructStudy2Nastran(string fname, bool runSolver, bool strict,
        double tolerance) {
    run(fname, SolverName::OPTISTRUCT, SolverName::NASTRAN, runSolver, strict, tolerance);
}

void CommandLineUtils::optistructStudy2Systus(string fname, bool runSolver, bool strict,
        double tolerance) {
    run(fname, SolverName::OPTISTRUCT, SolverName::SYSTUS, runSolver, strict, tolerance);
}

vector<bool> CommandLineUtils::containWords(const string &fname, const vector<string> &words) {
    ifstream file(fname);
    vector<string> lower_search;
    vector<bool> found;
    for (string word : words) {
        lower_search.push_back(to_lower_copy(word));
        found.push_back(false);
    }
    string buffer;
    while (file.good()) {
        file >> buffer;
        //string normalized = trim_copy(buffer);
        if (file.good()) {
            to_lower(buffer);
            for (unsigned int i = 0; i < words.size(); i++) {
                if (!found[i]) {
                    found[i] = (buffer.find(lower_search[i]) != string::npos);
                }
            }
        }
    }
    return found;
}

bool CommandLineUtils::containsWord(const string &fname, const string &word) {
    ifstream file(fname);
    string buffer;
    bool found = false;
    string lower_search = to_lower_copy(word);
    while (file.good()) {
        file >> buffer;
        to_lower(buffer);
        if (file.good()) {
            auto found1 = buffer.find(lower_search);
            found |= (found1 != string::npos);
            if (found) {
                break;
            }
        }
    }
    return found;
}

} /* namespace tests */
} /* namespace vega */
