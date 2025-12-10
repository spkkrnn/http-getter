#include "ReqHandling.h"

int main(int argc, char* argv[]) {
    using namespace std;
    // inits
    CURL *curl;
    CURLcode globalInit = curl_global_init(CURL_GLOBAL_SSL);
    curl = curl_easy_init();
    if (!curl || globalInit) {
        cerr << "init failed" << endl;
        return EXIT_FAILURE;
    }
    //set options
    string target_url = "http://localhost:8989/test";
    bool redirect = true; // follow redirects to webpage
    bool print = true; // print to stdout
    //curl_easy_setopt(curl, CURLOPT_URL, target_url.c_str());
    //curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_3);
    getPage(curl, target_url, print, redirect);

    //printHeaders(curl);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return EXIT_SUCCESS;
}