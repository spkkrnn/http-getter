#include "ReqHandling.h"

inline int WebResource::append(char *newBytes, size_t numberOfBytes) { // overwrite data
    if (this->m_size < numberOfBytes) {
        char *tmpData = (char *)realloc(this->m_data, numberOfBytes + 1);
        this->m_size = numberOfBytes;
        if (!tmpData) {
        std::cout << "alloc error" << std::endl;
            return -1;
        }
        this->m_data = tmpData;
    }
    memcpy(this->m_data, newBytes, numberOfBytes);
    this->m_data[this->m_size] = 0; // null terminator
    return 0;
}

inline int WebPage::append(char *newBytes, size_t numberOfBytes) { // actually append
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
        if (name == "version") {
            value -= 1;
        }
        this->m_info->insert({name, value});
    }
    catch (std::bad_alloc& e) {
        std::cerr << "Allocation failed for " << name << ": " << e.what() << std::endl;
    }
}

void WebResource::printTransferInfo() const {
    std::cout << "Resource " << this->m_id << ": " << this->m_url << "\n";
    for (const auto &infoPair : *(this->m_info)) { // terrible way to do this?
        std::cout << infoPair.first << ": " << infoPair.second << "\n";
    }
    std::cout << std::endl;
}

void WebPage::printAllTransferInfo() const {
    this->printTransferInfo();
    for (const auto &resourcePair : *(this->m_content)) {
        resourcePair.second.printTransferInfo();
    }
}

void WebResource::print() const {
    printf("%.*s\n", (int) this->m_size, this->m_data);
}

void WebPage::addLink(std::string link, enum linkpath pathType) {
    try {
        this->m_links->insert({link, pathType});
    }
    catch (std::bad_alloc& e) {
        std::cerr << "Allocation failed for " << link << ": " << e.what() << std::endl;
    }
}

void WebPage::addContent(int orderNum, WebResource resource) {
    try {
        this->m_content->insert({orderNum, resource});
    }
    catch (std::bad_alloc& e) {
        std::cerr << "Allocation failed adding resource number " << orderNum << ": " << e.what() << std::endl;
    }
}

void WebPage::removeHandles(CURLM *mCurl) {
    for (const auto &contentPair : *(this->m_content)) {
        CURL *currentCurl = contentPair.second.getConnection();
        curl_multi_remove_handle(mCurl, currentCurl);
    }
}

void WebPage::cleanUpContents() {
    for (auto &contentPair : *(this->m_content)) {
        contentPair.second.cleanUp();
    }
}

void WebPage::printLinks() const {
    for (const auto &linkPair : *(this->m_links)) { // terrible way to do this?
        std::cout << linkPair.first << "\n";
    }
    std::cout << std::endl;
}

bool WebPage::containsLink(const std::string link) const {
    if (this->m_links->count(link) != 0) {
        return true;
    }
    else {
        return false;
    }
}

static void dumpTrace(const char *text, const unsigned char *ptr, size_t size, char nohex) {  // from libcurl examples
    size_t i;
    size_t c;
    unsigned int width = 0x10;
    if(nohex)
        width = 0x40;
    fprintf(stderr, "%s, %lu bytes (0x%lx)\n", text, (unsigned long)size, (unsigned long)size);
    for(i = 0; i < size; i += width) {
        fprintf(stderr, "%4.4lx: ", (unsigned long)i);
    if(!nohex) {
      for(c = 0; c < width; c++)
        if(i + c < size)
          fprintf(stderr, "%02x ", ptr[i + c]);
        else
          fputs("   ", stderr);
    }
    for(c = 0; (c < width) && (i + c < size); c++) {
        if(nohex && (i + c + 1 < size) && ptr[i + c] == 0x0D && ptr[i + c + 1] == 0x0A) {
            i += (c + 2 - width);
            break;
        }
        fprintf(stderr, "%c", (ptr[i + c] >= 0x20) && (ptr[i + c] < 0x80) ? ptr[i + c] : '.');
        if(nohex && (i + c + 2 < size) && ptr[i + c + 1] == 0x0D &&
            ptr[i + c + 2] == 0x0A) {
            i += (c + 3 - width);
            break;
        }
    }
    fputc('\n', stderr);
  }
}
 
static int curlTrace(CURL *curl, curl_infotype infoType, char *data, size_t size, void *ptr) { // from libcurl examples
  const char *text;
  //WebResource *tmpResource = (WebResource *)resPtr;
  //int num = tmpResource->getID();
  char *hexChar = (char *)ptr;
  (void)curl;
  switch(infoType) {
  case CURLINFO_TEXT:
    fprintf(stderr, "== Info: %s", data);
    return 0;
  case CURLINFO_HEADER_OUT:
    text = "=> Send header";
    break;
  case CURLINFO_DATA_OUT:
    text = "=> Send data";
    break;
  case CURLINFO_SSL_DATA_OUT:
    text = "=> Send SSL data";
    break;
  case CURLINFO_HEADER_IN:
    text = "<= Recv header";
    break;
  case CURLINFO_DATA_IN:
    text = "<= Recv data";
    break;
  case CURLINFO_SSL_DATA_IN:
    text = "<= Recv SSL data";
    break;
  default:
    return 0;
  }
  dumpTrace(text, (const unsigned char *)data, size, *hexChar);
  return 0;
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

CURLcode httpGet(CURL *curl, const std::string &url) {
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    CURLcode perfResult = curl_easy_perform(curl);
    if (perfResult != CURLE_OK) {
        std::cerr << "problem fetching resource:" << curl_easy_strerror(perfResult) << std::endl; 
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

static int setCurlOptions(CURL *curl, WebResource &resource, int httpVersion, bool multi=true, bool trace=true) {
    const std::string url = resource.getUrl();
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    switch (httpVersion) {
        case 1:
            curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
            break;
        case 2:
            curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
            break;
        case 3:
            curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_3);
            break;
    }
    if (multi) {
        curl_easy_setopt(curl, CURLOPT_PIPEWAIT, 1L);
        curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 100000L);
    }
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, receiveData);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&resource);
    if (trace) {
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, curlTrace);
        curl_easy_setopt(curl, CURLOPT_DEBUGDATA, (void *)&resource);
    }
    return 0;
}

void cleanUpMulti(CURLM *multi, WebPage &webPage, bool cleanCurls=true) {
    if (multi) {
        webPage.removeHandles(multi);
        if (cleanCurls) webPage.cleanUpContents();
    }
    curl_multi_cleanup(multi);
}

std::string stripToBase(const std::string &url) {
    std::size_t start = url.find("//");
    start = (start != std::string::npos) ? (start + 2) : 0;
    std::size_t end = url.find("/", start);
    if (end > url.length()) {
        end = url.length();
    }
    return url.substr(start, end - start);
}

std::string fixLink(char *urlPtr, const std::string &baseUrl) {
    std::string url(urlPtr);
    if (url.length() > 1) {
        switch (url.find("//")) {
            case 0:
                url = "https:" + url; // possibly add option for HTTP or HTTPS
                break;
            case 5:
            case 6:
                break;
            default:
                if (url[0] == '/') { // relative link
                    url = "https://" + baseUrl + url;
                }
                else {
                    std::cout << "fixLink: " << url << std::endl; // TEST
                    url.clear();
                }
                break;
        }
    }
    return url;
}

std::string findObj(char *nodeText, const std::string &baseUrl) {
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
                objLink = fixLink(nodeText, baseUrl);
            }
        }
    }
    return objLink;
}

int scrapeLinks(WebPage &webPage) { // TODO: add error checks
    const std::string xCmd("//@href");
    const std::string pageUrl = webPage.getUrl();
    const std::string baseUrl = stripToBase(pageUrl);
    htmlDocPtr doc = htmlReadMemory(webPage.getHtml(), webPage.getDataLen(), NULL, NULL, HTML_PARSE_NOERROR);
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
        char *content = (char *)xmlNodeGetContent(result->nodesetval->nodeTab[i]); // I guess we'll trust this is null-terminated
        std::string link = findObj(content, baseUrl);
        xmlFree(content);
        if (!link.empty() && !webPage.containsLink(link)) {
            enum linkpath linkLocation = (link.find(baseUrl) != std::string::npos) ? BASE_CONTENT : EXTERNAL_CONTENT;
            webPage.addLink(link, linkLocation);
        }
    }
    xmlXPathFreeObject(result);
    xmlXPathFreeContext(context);
    xmlFreeDoc(doc);
    xmlCleanupParser();
    webPage.printLinks(); // test
    return 0;
}

int fetchContent(CURL *curl, WebPage &webPage, bool baseOnly=false) {
    link_map *linkList = webPage.getLinks();
    if (linkList->empty()) return 0;
    CURLcode getResult = CURLE_OK;
    int index = 0;
    for (const auto &linkPair : *linkList) {
        if (linkPair.second != BASE_CONTENT) {
            continue;
        }
        ++index;
        WebResource resource(curl, linkPair.first, index);
        /*if (debugTrace) {
            curl_easy_setopt(curl, CURLOPT_DEBUGDATA, (void *)&resource);
        }*/
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&resource);
        getResult = httpGet(curl, linkPair.first);
        if (getResult != CURLE_OK) {
            std::cout << "Failed to fetch " << linkPair.first << std::endl;
            continue;
        }
        saveTransferInfo(curl, resource);
        webPage.addContent(index, resource);
    }
    if (baseOnly) return 0;
    for (const auto &linkPair : *linkList) {
        if (linkPair.second != EXTERNAL_CONTENT) {
            continue;
        }
        ++index;
        WebResource resource(curl, linkPair.first, index);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&resource);
        getResult = httpGet(curl, linkPair.first);
        if (getResult != CURLE_OK) {
            std::cout << "Failed to fetch " << linkPair.first << std::endl;
            continue;
        }
        saveTransferInfo(curl, resource);
        webPage.addContent(index, resource);
    }
    return 0;
}

int multiFetch(WebPage &webPage, int httpVersion) { // not functional
    CURLM *multi = nullptr;
    link_map *linkList = webPage.getLinks();
    if (linkList->empty()) return 0;
    CURLcode getResult = CURLE_OK;
    multi = curl_multi_init();
    int handlesRunning = 0;
    int index = 0; // becomes the ID of the download
    if (!multi) {
        cleanUpMulti(multi, webPage);
        return -1;
    }
    for (const auto &linkPair : *linkList) {
        if (linkPair.second != BASE_CONTENT) {
            continue;
        }
        CURL *objCurl = nullptr;
        ++index;
        WebResource resource(objCurl, linkPair.first, index);
        setCurlOptions(objCurl, resource, httpVersion);
        curl_multi_add_handle(multi, objCurl);
        webPage.addContent(index, resource);
    }
    curl_multi_setopt(multi, CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX);
    do {
        CURLMcode mResult = curl_multi_perform(multi, &handlesRunning);
        if (handlesRunning) {
            mResult = curl_multi_poll(multi, NULL, 0, 1000, NULL);
        }
        if (mResult) break;
    } while (handlesRunning);
    cleanUpMulti(multi, webPage, false);
    return 0;
}

CURLcode getPage(CURL *curl, std::string &url, bool print, bool trace) {
    CURLcode getResult = CURLE_OK;
    char yesHex = 1;
    if (trace) {
        curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, curlTrace);
        curl_easy_setopt(curl, CURLOPT_DEBUGDATA, &yesHex);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    }
    WebPage page(curl, url);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, receiveData);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&page);
    getResult = httpGet(curl, url);
    if (getResult != CURLE_OK) {
        std::cout << "Problem loading page." << std::endl;
        return getResult;
    }
    scrapeLinks(page);
    saveTransferInfo(curl, page);
    //page.printTransferInfo();
    fetchContent(curl, page);
    if (print) {
        page.printAllTransferInfo();
    }
    return getResult;
}
