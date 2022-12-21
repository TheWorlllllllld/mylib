#include "Buffer.h"

#include <unistd.h>
#include <sys/uio.h> 
#include <errno.h>

//从fd中读取数据，并写入到Buffer中
ssize_t Buffer::readFd(int fd, int* savedErrno){
    //创建一个临时缓冲区
    char extrabuf[65536] = {0};
    struct iovec vec[2];
    //vec的第一个缓冲区是当前buffer已有的剩余可写空间
    const size_t writable = writableBytes();  //获取缓冲区剩余空间大小
    vec[0].iov_base = begin() + write_index_; //获取缓冲区头指针
    vec[0].iov_len = writable; //获取缓冲区剩余空间大小
    //vec的第二个缓冲区是临时缓冲区extrabuf
    vec[1].iov_base = extrabuf; //获取临时缓冲区指针
    vec[1].iov_len = sizeof(extrabuf); //获取临时缓冲区大小

    const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1; //判断是否需要使用临时缓冲区,目的是为了将吞吐量限制到64KB
    const ssize_t n = ::readv(fd, vec, iovcnt); //读取数据
    if(n < 0){
        *savedErrno = errno;
    }
    else if(n<=writable){ //说明buffer本身的剩余空间就足够写入数据
        write_index_ += n;
    }
    else{
        write_index_ = buffer_.size();
        append(extrabuf, n - writable); //把临时缓冲区数据写入到buffer中
    }
    return n;
}

ssize_t Buffer::writeFd(int fd, int* savedErrno){
    ssize_t n = ::write(fd, begin() + write_index_, readableBytes()); //写入数据
    if(n < 0){
        *savedErrno = errno;
    }
    else{
        write_index_ += n;
    }
    return n;
}