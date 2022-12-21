#include "httprequest.h"

#include <regex>

//解析对外接口
bool httprequest::parse(std::string &buff){
    if(buff.size()<=0){
        return false;
    }
    const char RN[]="\r\n";
    while(buff.size()&&state!=FINISH){
        const char* lineEnd = std::search(buff.c_str(),buff.c_str()+buff.size(),RN,RN+2);
        std::string line(buff.c_str(),lineEnd);
        switch(state){
            case  REQUEST_LINE:
                if(!parseRequestLine(line)) {
                    return false;
                }
                break;
            case  HEADERS:
                parseRequestHeader(line);
                if(buff.size()<=2){
                    state=FINISH;
                }
                break;
            case  BODY:
                parseRequestBody(line);
                break;
            default:
                break;
        }
        if(lineEnd == buff.c_str()+buff.size()) { break; }
        buff.erase(0,lineEnd+2-buff.c_str());
    }
}

//解析请求行
bool httprequest::parseRequestLine(const std::string line){
    std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch subMatch;
    if(std::regex_match(line, subMatch, patten)) {
        method = subMatch[1];
        path = subMatch[2];
        version = subMatch[3];
        state = HEADERS;
        return true;
    }
    return false;
}
//解析请求数据
void httprequest::parseRequestBody(const std::string line){
    body = line;
    state = FINISH;
}
//解析请求头
void httprequest::parseRequestHeader(const std::string line){
    std::regex patten("^([^:]*): ?(.*)$");
    std::smatch subMatch;
    if(std::regex_match(line, subMatch, patten)) {
        header[subMatch[1]] = subMatch[2];
    }
    else {
        state = BODY;
    }
}

std::string httprequest::GETpath(){
    return path;
}

std::string httprequest::GETmethod(){
    return method;
}

std::string httprequest::GETversion(){
    return version;
}

std::string httprequest::GETbody(){
    return body;
}

std::string httprequest::GETheader(const std::string key){
    if(header.find(key) != header.end()){
        return header[key];
    }
    return "";
}