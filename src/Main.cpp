#include "ReqHandling.h"

void printHelp() {
    std::cout << "Usage: Give target URL, and optionally some of the following flags.\n";
    std::cout << "FLAGS:\n\t--http\n\tSet HTTP version. Should be followed by a value.\n\tAccepted values: 1 / 1.1 / 2 / 3\n\tDefault: libcurl decides\n";
    std::cout << "\t--agent\n\tSet user agent. Should be followed by a value (string).\n\tDefault: libcurl-agent/1.0\n";
    std::cout << "\t--nofollow\n\tDo not follow redirects.\n";
    std::cout << "\t--noprint\n\tDo not print transfer information to STDOUT.\n";
    std::cout << "\t--notrace\n\tDo not print verbose debugging details to STDERR.\n";
    std::cout << "\t--help\n\tPrint this.\n";
    std::cout << "Example: ./program https://example.com\nExample: ./program --http 1.1 --notrace --agent my-own-agent/0.1 http://example.com/" << std::endl;
}

void missingArgument(std::string& flag) {
    std::cout << "Error: " << flag << " expected an argument." << std::endl;
}

void removeQuotes(std::string& text) {
    if (text.length() < 1) return;
    char c = text.back();
    if (text[0] == '\"' || text[0] == '\'') text.erase(0);
    if (c == '\"' || c == '\'') text.pop_back();
}

int main(int argc, char* argv[]) {
    using namespace std;
    // input arguments
    if (argc < 2) {
        printHelp();
        return EXIT_SUCCESS;
    }
    string target = "https://"; // webpage URL
    string agent = "libcurl-agent/1.0"; // user agent
    long version = CURL_HTTP_VERSION_NONE;
    bool redirect = true;
    bool print = true;
    bool trace = true;
    if (argc == 2) {
        string arg1(argv[1]);
        if (arg1 == "--help") {
            printHelp();
            return EXIT_SUCCESS;
        }
        target = ((arg1.find("http")) != 0) ? (target + arg1) : arg1;
    }
    else {
        bool expecting = false; // expecting an argument to follow a flag
        string prevFlag;
        const unordered_map<string, long> httpVersions {
            {"1", CURL_HTTP_VERSION_1_0},
            {"1.0", CURL_HTTP_VERSION_1_0},
            {"1.1", CURL_HTTP_VERSION_1_1},
            {"2", CURL_HTTP_VERSION_2_0},
            {"3", CURL_HTTP_VERSION_3} };
        for (int i = 1; i < argc; i++) {
            string argument(argv[i]);
            if (argument[0] == '-') { // check if flag
                if (expecting) {
                    missingArgument(prevFlag);
                    return EXIT_FAILURE;
                }
                if (argument == "--http" || argument == "--agent") {
                    expecting = true;
                    prevFlag = argument;
                }
                else if (argument == "--nofollow") redirect = false;
                else if (argument == "--noprint") print = false;
                else if (argument == "--notrace") trace = false;
                else if (argument == "--help") {
                    printHelp();
                    return EXIT_SUCCESS;
                }
            }
            else {
                if (!expecting) target = ((argument.find("http")) != 0) ? (target + argument) : argument;
                else {
                    if (prevFlag == "--http") {
                        try {
                            version = httpVersions.at(argument);
                        }
                        catch (const out_of_range &notfound) {
                            cout << "Unexpected HTTP version." << endl;
                            cerr << notfound.what() << endl;
                            return EXIT_FAILURE;
                        }
                    }
                    else if (prevFlag == "--agent") {
                        removeQuotes(argument);
                        agent = argument;
                    }
                    expecting = false;
                }
            }
        }
    }
    // inits
    CURL *curl;
    CURLcode rcode = curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (!curl || rcode) {
        cerr << "init failed" << endl;
        return EXIT_FAILURE;
    }
    //set options
    rcode = curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, version);
    if (rcode != CURLE_OK) {
        cerr << "Error setting HTTP version." << endl;
    }
    rcode = curl_easy_setopt(curl, CURLOPT_USERAGENT, agent.c_str());
    if (rcode != CURLE_OK) {
        cerr << "Error setting user agent." << endl;
    }
    else if (redirect) {
        rcode = curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // 1L = CURLFOLLOW_ALL
    }
    if (rcode == CURLE_OK) {
        getPage(curl, target, print, trace);
    }
    //printHeaders(curl);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return EXIT_SUCCESS;
}