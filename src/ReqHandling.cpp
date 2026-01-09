#include "ReqHandling.h"

inline int WebResource::append(char *newBytes, size_t numberOfBytes) {
    char *tmpData = (char *)realloc(this->m_data, this->m_size + numberOfBytes + 1);
    if (!tmpData) {
        std::cout << "alloc error" << std::endl;
        return -1;
    }
    this->m_data = tmpData;
    memcpy(&(this->m_data[this->m_size]), newBytes, numberOfBytes);
    this->m_size += numberOfBytes;
    this->m_data[this->m_size] = 0; // null terminator
    return 0;
}

void WebResource::addInfo(std::string name, curl_off_t value) {
    try {
        this->m_info->insert({name, value});
    }
    catch (std::bad_alloc& e) {
        std::cerr << "Allocation failed for " << name << ": " << e.what() << std::endl;
    }
}

void WebResource::print() {
    printf("%.*s\n", (int) this->m_size, this->m_data);
}

void printLinks(std::unordered_map<std::string, enum linkpath> &links) {
    for (const auto &linkPair : links) {
        std::cout << linkPair.first << "\n";
    }
    std::cout << std::endl;
}

static size_t receiveData(char *buffer, size_t itemsize, size_t nmemb, void *dest) {
    size_t chunkSize = itemsize * nmemb;
    WebResource *tmpResource = (WebResource *)dest;
    if (tmpResource->append(buffer, chunkSize) < 0) {
        std::cout << "realloc failed" << std::endl;
        return 0;
    }
    return chunkSize;
}

int saveTransferInfo(CURL *curl, WebResource &resource) {
    //const std::unordered_map<std::string, CURLINFO> *vars = &(InfoVars::transferDetails);
    CURLcode result;
    curl_off_t tmp;
    for (const auto &infoPair : InfoVars::transferDetails) {
        result = curl_easy_getinfo(curl, infoPair.second, &tmp);
        if ((result == CURLE_OK) && tmp) {
            resource.addInfo(infoPair.first, tmp);
        }
    }
    return 0;
}

CURLcode httpGet(CURL *curl, std::string &url) {
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    CURLcode perfResult = curl_easy_perform(curl);
    if (perfResult != CURLE_OK) {
        std::cerr << "problem loading page:" << curl_easy_strerror(perfResult) << std::endl; 
    }
    return perfResult;
}

int printHeaders(CURL *curl) {
    struct curl_header *prev = NULL;
    struct curl_header *h;
    do {
        h = curl_easy_nextheader(curl, CURLH_HEADER, -1, prev);
        if(h) {
            //printf(" %s: %s (%u)\n", h->name, h->value, (unsigned int) h->amount);
            std::cout << h->name << ": " << h->value << " (" << h->amount << ")" << std::endl;
        }
        prev = h;
    } while(h);
    return 0;
}

std::string findObj(char *nodeText) {
    char *ptr = nodeText;
    char *dotPtr = nullptr;
    std::string objLink;
    for (int i = 0; i < MAXNODELEN; ++i) {
        ptr++;
        if (isspace(*ptr) != 0 || *ptr == '\0') {
        *ptr = '\0';
        break;
        }
        if (*ptr == '.') dotPtr = ptr; // eventually the last dot in the string
    }
    if (dotPtr) {
        std::string suffix(dotPtr);
        for (auto ftype : ftypes) {
        if (suffix.find(ftype) != std::string::npos) {
            objLink = std::string(nodeText);
        }
        }
    }
    return objLink;
}

int scrapeLinks(WebResource &webPage) { // TODO: add error checks
    std::string xCmd("//@href");
    std::unordered_map<std::string, enum linkpath> links;
    htmlDocPtr doc = htmlReadMemory(webPage.getHtml(), webPage.getHtmlLen(), NULL, NULL, HTML_PARSE_NOERROR);
    xmlXPathContextPtr context = xmlXPathNewContext(doc);
    xmlXPathObjectPtr result = xmlXPathEvalExpression((xmlChar *) xCmd.c_str(), context);
    if (result->nodesetval == NULL) {
        std::cout << "No links found." << std::endl;
        xmlXPathFreeObject(result);
        xmlXPathFreeContext(context);
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return 1;
    }
    for (int i = 0; i < result->nodesetval->nodeNr; ++i) {
        char *content = (char *)xmlNodeGetContent(result->nodesetval->nodeTab[i]);
        std::string added = findObj(content); // I guess we'll trust this is null-terminated
        xmlFree(content);
        if (!added.empty() && links.count(added) == 0) {
            enum linkpath pathType = NONE;
            if (added[0] == 'h') pathType = ABSOLUTE;
            else if (added[0] == '/') pathType = RELATIVE;
            if (pathType == NONE) {
                std::cout << "Invalid link: " << added << std::endl;
            }
            links.insert(std::make_pair(added, pathType));
        }
    }
    xmlXPathFreeObject(result);
    xmlXPathFreeContext(context);
    xmlFreeDoc(doc);
    xmlCleanupParser();
    printLinks(links);
    return 0;
}

CURLcode getPage(CURL *curl, std::string &url, bool print, bool redirect) {
    CURLcode getResult = CURLE_OK;
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    if (redirect) {
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // 1L = CURLFOLLOW_ALL
    }
    if (print) {
        getResult = httpGet(curl, url);
        return getResult;
    }
    WebResource page(HTML);
    //std::unordered_map<std::string, enum linkpath> contentLinks;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, receiveData);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&page);
    getResult = httpGet(curl, url);
    scrapeLinks(page);
    return getResult;
}
