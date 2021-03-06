/* vim:set ts=4 sw=4 sts=4 et: */

#include <algorithm>
#include <ctime>
#include <iostream>
#include <string>

#include "cmd_arguments.h"

using namespace std;
using namespace SimpleOpt;

enum {
    HELP=30000, VERSION, VERBOSE, QUIET, USE_STDIN, OUT_FILE, MODEL,
    MODE, USE_EDGE, INPUT_FORMAT, OUTPUT_FORMAT
};

CommandLineArguments::CommandLineArguments(
        const std::string programName, const std::string version) :
    m_executableName(programName), m_versionNumber(version),
    m_options(),
    inputFile(), verbosity(1), outputFile(),
    modelType(SWITCHBOARD_MODEL), operationMode(MODE_DRIVER_NODES),
    useEdgeMeasure(false),
    inputFormat(GRAPH_FORMAT_AUTO), outputFormat(GRAPH_FORMAT_GML)
{

    addOption(USE_STDIN, "-", SO_NONE);

    addOption(HELP, "-?", SO_NONE);
    addOption(HELP, "-h", SO_NONE, "--help");

    addOption(VERSION, "-V", SO_NONE, "--version");
    addOption(VERBOSE, "-v", SO_NONE, "--verbose");
    addOption(QUIET,   "-q", SO_NONE, "--quiet");

    addOption(OUT_FILE, "-o", SO_REQ_SEP, "--output");
    addOption(MODEL,    "-m", SO_REQ_SEP, "--model");
    addOption(MODE,     "-M", SO_REQ_SEP, "--mode");

    addOption(INPUT_FORMAT,  "-f", SO_REQ_SEP, "--input-format");
    addOption(OUTPUT_FORMAT, "-F", SO_REQ_SEP, "--output-format");

    addOption(USE_EDGE, "-e", SO_NONE, "--edge");
}

void CommandLineArguments::addOption(int id, const char* option,
        SimpleOpt::ESOArgType type, const char* longOption) {
    SimpleOpt::CSimpleOpt::SOption opt;
    opt.nId = id;
    opt.pszArg = option;
    opt.nArgType = type;
    m_options.push_back(opt);

    if (longOption) {
        opt.pszArg = longOption;
        m_options.push_back(opt);
    }
}

int CommandLineArguments::handleFormatOption(const std::string& arg, GraphFormat* pFormat) {
    *pFormat = GraphUtil::formatFromString(arg);
    return *pFormat == GRAPH_FORMAT_UNKNOWN;
}

int CommandLineArguments::handleOption(int id, const std::string& arg) {
    return 0;
}

void CommandLineArguments::parse(int argc, char** argv) {
    CSimpleOpt::SOption* optionSpec = new CSimpleOpt::SOption[m_options.size()+1];
    std::copy(m_options.begin(), m_options.end(), optionSpec);

    optionSpec[m_options.size()].nId = -1;
    optionSpec[m_options.size()].pszArg = 0;
    optionSpec[m_options.size()].nArgType = SO_NONE;

    CSimpleOpt args(argc, argv, optionSpec);
    string arg;
    int ret = -1;

    while (ret == -1 && args.Next()) {
        if (args.LastError() != SO_SUCCESS) {
            cerr << "Invalid argument: " << args.OptionText() << "\n";
            ret = 1;
            break;
        }

        switch (args.OptionId()) {
            /* Processing - */
            case USE_STDIN:
                inputFile = "-";
                break;

            /* Processing --help and --version */
            case HELP:
                showHelp(cerr);
                ret = 0;
                break;

            case VERSION:
                cerr << m_executableName << ' ' << m_versionNumber << '\n';
                ret = 0;
                break;

            /* Log levels */
            case VERBOSE:
				verbosity = 2;
                break;
            case QUIET:
				verbosity = 0;
                break;

            /* Processing basic algorithm parameters */
            case OUT_FILE:
                outputFile = args.OptionArg();
                break;

            case MODEL:
                arg = args.OptionArg() ? args.OptionArg() : "";
                if (arg == "liu")
                    modelType = LIU_MODEL;
                else if (arg == "switchboard")
                    modelType = SWITCHBOARD_MODEL;
                else {
                    cerr << "Unknown model type: " << arg << '\n';
                    ret = 1;
                }
                break;

            case MODE:
                arg = args.OptionArg() ? args.OptionArg() : "";
                if (arg == "driver_nodes")
                    operationMode = MODE_DRIVER_NODES;
                else if (arg == "control_paths")
                    operationMode = MODE_CONTROL_PATHS;
                else if (arg == "graph")
                    operationMode = MODE_GRAPH;
                else if (arg == "statistics")
                    operationMode = MODE_STATISTICS;
                else if (arg == "significance")
                    operationMode = MODE_SIGNIFICANCE;
                else {
                    cerr << "Unknown operation mode: " << arg << '\n';
                    ret = 1;
                }
                break;

            /* Processing advanced algorithm parameters */
            case USE_EDGE:
                useEdgeMeasure = true;
                break;

            /* Processing format options parameters */
            case INPUT_FORMAT:
                arg = args.OptionArg() ? args.OptionArg() : "";
                ret = handleFormatOption(arg, &inputFormat);
                if (ret) {
                    cerr << "Unknown input format: " << arg << '\n';
                    ret = 1;
                } else {
                    ret = -1;
                }
                break;

            case OUTPUT_FORMAT:
                arg = args.OptionArg() ? args.OptionArg() : "";
                ret = handleFormatOption(arg, &outputFormat);
                if (ret || (outputFormat != GRAPH_FORMAT_GRAPHML &&
                            outputFormat != GRAPH_FORMAT_GML)) {
                    cerr << "Unknown output format: " << arg << '\n';
                    ret = 1;
                } else {
                    ret = -1;
                }
                break;

            default:
                arg = args.OptionArg() ? args.OptionArg() : "";
                ret = handleOption(args.OptionId(), arg);
                if (!ret)
                    ret = -1;
        }
    }

    if (ret != -1) {
        delete[] optionSpec;
        exit(ret);
    }

    /* If no input file was given, show help and exit */
    if (args.FileCount() == 0 && inputFile != "-") {
        delete[] optionSpec;
        showHelp(cerr);
        exit(1);
    }

    /* If the output file is empty, use a single dash, meaning stdout */
    if (outputFile == "")
        outputFile = "-";

    /* Set up the input file if it is not stdin */
    if (inputFile != "-")
        inputFile = args.Files()[0];

    delete[] optionSpec;
}

void CommandLineArguments::showHelp(ostream& os) const {
    os << "Usage:\n    " << m_executableName << " [options] inputfile\n"
          "\n";
    os << "    -h, --help          shows this help message\n"
          "    -V, --version       shows the version number\n"
          "    -v, --verbose       verbose mode (more output)\n"
          "    -q, --quiet         quiet mode (less output, only errors)\n"
          "\n"
          "Basic algorithm parameters:\n"
          "    -m, --model         selects the controllability model to use.\n"
          "                        Supported models: liu, switchboard.\n"
          "                        Default: switchboard.\n"
          "    -M, --mode          selects the mode in which the application will operate.\n"
          "                        Supported modes: driver_nodes, control_paths, graph,\n"
          "                        statistics, significance. Default: driver_nodes.\n"
          "    -o, --output        specifies the name of the output file where the results\n"
          "                        should be written.\n"
          "\n"
          "Advanced algorithm parameters:\n"
          "    -e, --edge          use the edge-based controllability measure for the\n"
          "                        switchboard model.\n"
          "\n"
          "Input/output format:\n"
          "    -f, --input-format  specifies the input format for reading graphs.\n"
          "                        Supported formats: auto, edgelist, gml, graphml, lgl, ncol\n"
          "                        Default: auto, except when the input file comes from\n"
          "                        stdin; in this case, edgelist is used.\n"
          "    -F, --output-format specifies the output format for writing graphs. Used only\n"
          "                        when mode = graph. Supported formats: gml, graphml.\n"
          "                        Default: gml.\n";

}
