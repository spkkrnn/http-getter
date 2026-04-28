#include "ReqHandling.h"

int main(int argc, char* argv[]) {
    using namespace std;
    // input arguments
    string target = "https://"; // webpage URL
    if (argc == 2) {
        std::string arg1(argv[1]);
        target = ((arg1.find("http")) != 0) ? (target + arg1) : arg1;
    }
    else {
        target = "http://localhost:8989/test";
    }
    // inits
    CURL *curl;
    CURLcode globalInit = curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (!curl || globalInit) {
        cerr << "init failed" << endl;
        return EXIT_FAILURE;
    }
    //set options
    int version = 2;
    bool redirect = true; // follow redirects to webpage
    bool print = false; // print to stdout

    getPage(curl, target, version, print, redirect);

    //printHeaders(curl);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return EXIT_SUCCESS;
}