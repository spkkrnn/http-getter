#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <exception>
#include <unordered_map>
#include <curl/curl.h>

namespace InfoVars {
    const std::unordered_map<std::string, CURLINFO> transferDetails = {
        {"connect", CURLINFO_CONNECT_TIME_T},
        {"appconnect", CURLINFO_APPCONNECT_TIME_T},
        {"pretransfer", CURLINFO_PRETRANSFER_TIME_T},
        {"total", CURLINFO_TOTAL_TIME_T},
        {"dlspeed", CURLINFO_SPEED_DOWNLOAD_T}
    }; //{"posttransfer", CURLINFO_POSTTRANSFER_TIME_T} not in current libcurl
}

enum type {
    HTML,
    IMAGE,
    SCRIPT,
    STYLE
};

class WebResource {
    private:
        enum type m_type;
        char *m_data;
        size_t m_size;
        std::unordered_map<std::string, curl_off_t> *m_info;
    public:
        WebResource(enum type rsrcType)
            : m_type(rsrcType)
            , m_data(nullptr)
            , m_size(0)
            {
                m_data = (char *)malloc(1);
                m_info = new std::unordered_map<std::string, curl_off_t>;
            }
        int append(char* , size_t );
        void addInfo(std::string , curl_off_t);
        void print();
        ~WebResource() {
            free(m_data);
            delete m_info;
        }
};

CURLcode getPage(CURL* , std::string& , bool=false , bool=true );
int printHeaders(CURL* );
