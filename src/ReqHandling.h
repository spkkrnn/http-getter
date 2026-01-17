#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <exception>
#include <unordered_map>
#include <curl/curl.h>
#include <libxml/xpath.h>
#include <libxml/HTMLparser.h>

#define MAXNODELEN 512

static std::string ftypes[] = {"ico", "jpg", "jpeg", "png", "svg", "css", "js"};

namespace InfoVars {
    const std::unordered_map<std::string, CURLINFO> transferDetails = {
        {"connect", CURLINFO_CONNECT_TIME_T},
        {"appconnect", CURLINFO_APPCONNECT_TIME_T},
        {"pretransfer", CURLINFO_PRETRANSFER_TIME_T},
        {"total", CURLINFO_TOTAL_TIME_T},
        {"dlspeed", CURLINFO_SPEED_DOWNLOAD_T}
    }; //{"posttransfer", CURLINFO_POSTTRANSFER_TIME_T} not in current libcurl
}

enum linkpath {
    NONE,
    ABSOLUTE,
    RELATIVE
};

using link_map = std::unordered_map<std::string, enum linkpath>;

class WebResource {
    protected:
        std::string m_url;
        char *m_data;
        size_t m_size;
        std::unordered_map<std::string, curl_off_t> *m_info; //transfer info
    public:
        WebResource(std::string url)
            : m_url{ url }
            , m_data{ (char *)malloc(1) }
            , m_size{ 0 }
            , m_info{ new std::unordered_map<std::string, curl_off_t>()} {
                //m_data = (char *)malloc(1);
                //m_info = std::unordered_map<std::string, curl_off_t>();
            }
        int append(char* , size_t );
        void addInfo(std::string , curl_off_t );
        void print();
        char* getHtml() { return m_data; }
        size_t getHtmlLen() { return m_size; }
        virtual ~WebResource() {
            if (m_data) {
                free(m_data);
            }
            delete m_info;
        }
};

class WebPage : public WebResource {
    private:
        link_map *m_links;
    public:
        WebPage(std::string url)
            : WebResource{ url }
            , m_links{ new std::unordered_map<std::string, enum linkpath>() } {
                //m_links = new std::unordered_map<std::string, enum linkpath>();
            }
        ~WebPage() {
            delete m_links;
        }
        void addLink(std::string , enum linkpath );
        void printLinks() const;
        bool containsLink(const std::string ) const;
};

CURLcode getPage(CURL* , std::string& , bool=false , bool=true );
int printHeaders(CURL* );
