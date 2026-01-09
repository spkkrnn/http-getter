#include "ReqHandling.h"

int main(int argc, char* argv[]) {
    using namespace std;
    // input arguments
    string target; // webpage URL
    if (argc == 2) {
        target = argv[1];
    }
    else {
        target = "http://localhost:8989/test";
    }
    // inits
    CURL *curl;
    CURLcode globalInit = curl_global_init(CURL_GLOBAL_SSL);
    curl = curl_easy_init();
    if (!curl || globalInit) {
        cerr << "init failed" << endl;
        return EXIT_FAILURE;
    }
    //set options
    bool redirect = true; // follow redirects to webpage
    bool print = false; // print to stdout

    getPage(curl, target, print, redirect);

    printHeaders(curl);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return EXIT_SUCCESS;
}