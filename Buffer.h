#pragma once

#include <vector>
#include <string>

// 类如其名《Buffer》
class Buffer
{
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize)
        , read_index_(kCheapPrepend)
        , write_index_(kCheapPrepend)
    {}
    ~Buffer() = default;

    size_t readableBytes() const { return write_index_ - read_index_; }

    size_t writableBytes() const { return buffer_.size() - write_index_; }

    //返回可读位置的首地址（头部到读指针之间的位置是有可能留下空余空间的，比如读走一部分数据后空余出来的那部分）
    size_t prependableBytes() const { return read_index_; }

    //返回数据可读数据区域的起始地址
    const char* peek() const { return begin() + read_index_; }

    //
    void retrieve(size_t len)
    {
        if(len < readableBytes()) //只读了一部分
        {
            read_index_ += len;
        }
        else // len >= readableBytes()，说明全都读完了
        {
            retrieveAll();
        }
    }

    void retrieveAll(){
        read_index_ = kCheapPrepend;
        write_index_ = kCheapPrepend;
    }

    //把接受的数据转换为string数据返回出来
    std::string retrieveAllAsString(){
        return retrieveAsString(readableBytes());//只返回已有的数据：长度为readableBytes()
    }

    std::string retrieveAsString(size_t len){
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }

    //判断将要写入的数据是否大于可写区域，判断是否需要扩容
    void ensureWritableBytes(size_t len){
        if(writableBytes() < len){
            makeSpace(len);
        }
    }
    //返回可写区域的起始地址
    char* beginWrite(){
        return begin() + write_index_;
    }

    const char* beginWrite() const{
        return begin() + write_index_;
    }

    //有数据写入了，所以要讲写指针后移
    void hasWritten(size_t len){
        write_index_ += len;
    }

    void append(const char* data, size_t len){
        ensureWritableBytes(len);
        std::copy(data, data + len, beginWrite());
        hasWritten(len);
    }
    void append(const std::string &str){
        append(str.data(), str.size());
    }
    void append(const Buffer &buf){
        append(buf.peek(), buf.readableBytes());
    }
    void append(const void* data, size_t len){
        append(static_cast<const char*>(data), len);
    }
    
    //从fd中读数据
    ssize_t readFd(int fd, int* savedErrno);
    //写数据到fd中
    ssize_t writeFd(int fd, int* savedErrno);

private:
    //返回缓冲区头指针
    char *begin() { return &*buffer_.begin(); }

    const char *begin() const { return &*buffer_.begin(); }

    //扩容缓冲区
    void makeSpace(size_t len){
        if(writableBytes() + prependableBytes() < len + kCheapPrepend){ //剩余空间不足，需要重新分配空间
            buffer_.resize(write_index_ + len);
        }
        else{ //算上读取数据后遗留的那点空间就足够了，那就不用扩容直接给len腾位置就行
            size_t readable = readableBytes();
            std::copy(begin()+read_index_ , begin()+write_index_ , begin()+kCheapPrepend);
            read_index_ = kCheapPrepend;
            write_index_ = write_index_ + readable;
        }
    }

    std::vector<char> buffer_;
    size_t read_index_;
    size_t write_index_;
};
     