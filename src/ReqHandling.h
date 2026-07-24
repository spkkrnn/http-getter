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

static std::string ftypes[] = {"avif", "ico", "jpg", "jpeg", "png", "svg", "css", "js"};

namespace InfoVars {
    const std::unordered_map<std::string, CURLINFO> transferDetails = {
        {"version", CURLINFO_HTTP_VERSION},
        {"connect", CURLINFO_CONNECT_TIME_T},
        {"appconnect", CURLINFO_APPCONNECT_TIME_T},
        {"pretransfer", CURLINFO_PRETRANSFER_TIME_T},
        {"total", CURLINFO_TOTAL_TIME_T},
        {"dlspeed", CURLINFO_SPEED_DOWNLOAD_T},
        {"bytes", CURLINFO_SIZE_DOWNLOAD_T}
    }; //{"posttransfer", CURLINFO_POSTTRANSFER_TIME_T} not in currently installed libcurl
}

enum linkpath {
    NONE,
    BASE_CONTENT,
    EXTERNAL_CONTENT
}; 

class WebResource {
    protected:
        CURL *m_curl;
        std::string m_url;
        char *m_data;
        size_t m_size;
        int m_id;
        std::unordered_map<std::string, curl_off_t> *m_info; //transfer details specified in InfoVars
    public:
        WebResource(CURL *curl, std::string url, int id)
            : m_curl{ curl }
            , m_url{ url }
            , m_data{ (char *)malloc(1) }
            , m_size{ 0 }
            , m_id{ id }
            , m_info{ new std::unordered_map<std::string, curl_off_t>() } {
            }
        WebResource(const WebResource &rsc) {
            m_curl = rsc.getConnection();
            m_url = rsc.getUrl();
            m_data = (char *)malloc(1); // discard data
            m_size = 0;
            m_info = new std::unordered_map<std::string, curl_off_t>(rsc.getInfo());
        }
        virtual int append(char* , size_t );
        void addInfo(std::string , curl_off_t );
        void printTransferInfo() const;
        void print() const;
        CURL* getConnection() const { return m_curl; }
        size_t getDataLen() const { return m_size; }
        int getID() const { return m_id; }
        std::string getUrl() const { return m_url; }
        std::unordered_map<std::string, curl_off_t> getInfo() const { return *m_info; }
        void cleanUp() { curl_easy_cleanup(m_curl); }
        virtual ~WebResource() {
            if (m_data) {
                free(m_data);
            }
            delete m_info;
        }
};

using link_map = std::unordered_map<std::string, enum linkpath>;
using resource_map = std::unordered_map<int, WebResource>;

class WebPage : public WebResource {
    private:
        link_map *m_links;
        resource_map *m_content;
    public:
        WebPage(CURL* curl, std::string url)
            : WebResource{ curl, url, 0 }
            , m_links{ new std::unordered_map<std::string, enum linkpath>() }
            , m_content{ new std::unordered_map<int, WebResource>() } {
            }
        ~WebPage() {
            delete m_links;
            delete m_content;
        }
        int append(char* , size_t ) override;
        void addLink(std::string , enum linkpath );
        void addContent(int , WebResource );
        resource_map *getContents() { return m_content; }
        void removeHandles(CURLM* );
        void cleanUpContents();
        char* getHtml() { return m_data; }
        link_map *getLinks() { return m_links; };
        void printLinks() const;
        void printAllTransferInfo() const;
        bool containsLink(const std::string ) const;
};

CURLcode getPage(CURL* , std::string& , bool , bool );
int printHeaders(CURL* );
