#pragma once

#include <unordered_map>
#include <string>
#include <fcntl.h>  
#include <unistd.h> 
#include <sys/stat.h> 
#include <sys/mman.h> 

class httpresponse
{
private:
    void addStateLine_(std::string& buffer);
    void addResponseHeader_(std::string& buffer);
    void addResponseContent_(std::string& buffer);
    void addBody(std::string& buffer);
    void errorHTML_();
    std::string getFileType_();

    int code_ = -1;
    bool isKeepAlive_;
    std::string data = "";
    char* mmFile_ = nullptr;
    struct  stat mmFileStat_ = {0};

    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    static const std::unordered_map<int, std::string> CODE_STATUS;
    static const std::unordered_map<int, std::string> CODE_PATH;
public:
    httpresponse() = default;
    ~httpresponse() = default;

    void init(bool isKeepAlive,
                        int code,
                        std::string data);
    void makeResponse(std::string& buffer);
    char* file();
    size_t fileLen() const;
    void errorContent(std::string& buffer,std::string message);
    int code() const {return code_;}
    
};


