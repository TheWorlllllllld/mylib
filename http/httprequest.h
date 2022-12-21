#pragma once

#include <unordered_map>

class httprequest
{
private:
    //设置解析的状态
    enum PARSE_STATE{
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,
    };
    //存储请求头的数据
    std::unordered_map<std::string,std::string> header;
    PARSE_STATE state = REQUEST_LINE;
    std::string method = "";
    std::string URL = "";
    std::string path = "";
    std::string body = "";
    std::string version = "";

    //解析请求行
    bool parseRequestLine(const std::string);
    //解析请求数据
    void parseRequestBody(const std::string);
    //解析请求头
    void parseRequestHeader(const std::string);
    //解析POST请求
    void ParseFromUrlencoded();
public:
    //解析对外接口,使用时，只需将请求数据传入即可，有任何错误都返回false
    bool parse(std::string &buff);
    //返回请求行中的URL
    std::string GETpath();
    //返回请求行中的方法
    std::string GETmethod();
    //返回请求行中的版本
    std::string GETversion();
    //返回body的数据，是一个字符串
    std::string GETbody();
    //返回请求头中的数据,传入key即可返回value
    std::string GETheader(const std::string key);

};
