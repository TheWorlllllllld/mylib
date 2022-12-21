#include "httpresponse.h"

#include<iostream>

const std::unordered_map<std::string, std::string> httpresponse::SUFFIX_TYPE = {
        { ".html",  "text/html" },
        { ".xml",   "text/xml" },
        { ".xhtml", "application/xhtml+xml" },
        { ".txt",   "text/plain" },
        { ".rtf",   "application/rtf" },
        { ".pdf",   "application/pdf" },
        { ".word",  "application/nsword" },
        { ".png",   "image/png" },
        { ".gif",   "image/gif" },
        { ".jpg",   "image/jpeg" },
        { ".jpeg",  "image/jpeg" },
        { ".au",    "audio/basic" },
        { ".mpeg",  "video/mpeg" },
        { ".mpg",   "video/mpeg" },
        { ".avi",   "video/x-msvideo" },
        { ".gz",    "application/x-gzip" },
        { ".tar",   "application/x-tar" },
        { ".css",   "text/css "},
        { ".js",    "text/javascript "},
};

const std::unordered_map<int, std::string> httpresponse::CODE_STATUS = {
        { 200, "OK" },
        { 400, "Bad Request" },
        { 403, "Forbidden" },
        { 404, "Not Found" },
};

const std::unordered_map<int, std::string> httpresponse::CODE_PATH = {
        { 400, "/400.html" },
        { 403, "/403.html" },
        { 404, "/404.html" },
};

void httpresponse::init(bool isKeepAlive=false,
                            int code=-1,
                            std::string data_ = "")
{
    code_ = code;
    isKeepAlive_ = isKeepAlive;
    data = data_;
    mmFile_ = nullptr;
    mmFileStat_ = { 0 };
}


void httpresponse::makeResponse(std::string& buff){
    if(code_ == -1) {
        code_ = 200;
    }
    addStateLine_(buff);
    addResponseHeader_(buff);
    addBody(buff);

}


void httpresponse::addStateLine_(std::string& buff){
    std::string status;
    if(CODE_STATUS.find(code_) != CODE_STATUS.end()) {
        status = CODE_STATUS.at(code_);
    }
    else {
        status = CODE_STATUS.at(404);
    }
    buff.append("HTTP/1.1 " + std::to_string(code_) + " " + status + "\r\n");
}

void httpresponse::addResponseHeader_(std::string& buff){
    buff.append("Connection: ");
    if(isKeepAlive_) {
        buff.append("keep-alive\r\n");
        buff.append("keep-alive: max=6, timeout=120\r\n");
    } else{
        buff.append("close\r\n");
    }
    buff.append("Content-type: " + getFileType_() + "\r\n");
    buff.append("Access-Control-Allow-Origin: *\r\n");
}

void httpresponse::addBody(std::string& buff){
    buff.append("Content-length: " + std::to_string(data.size()) + "\r\n\r\n");
    buff.append(data + "\r\n");
}

std::string httpresponse::getFileType_(){
    return "text/plain";
}

void httpresponse::errorContent(std::string& buff,std::string message){
    std::string body;
    std::string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if(CODE_STATUS.count(code_) == 1) {
        status = CODE_STATUS.find(code_)->second;
    } else {
        status = "Bad Request";
    }
    body += std::to_string(code_) + " : " + status  + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>TinyWebServer</em></body></html>";

    buff.append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
    buff.append(body);
}