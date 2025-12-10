#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <exception>
#include <unordered_set>
#include <curl/curl.h>

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
    public:
        WebResource(enum type rsrcType)
            : m_type(rsrcType)
            , m_data(nullptr)
            , m_size(0)
            {
                m_data = (char *)malloc(1);
            }
        int append(char* , size_t );
        void print();
        ~WebResource() {
            free(m_data);
        }
};

CURLcode getPage(CURL* , std::string& , bool=false , bool=true );
