#include <iostream>
using namespace std;

#include "../httpresponse.h"

//发送文件的案例:srcDir为工作目录，path为要当前目录下要要发送的文件，
//当要发送文件时，init的最后一个值为空即可
// void test1()
// {
//     httpresponse response;
//     string srcDir = "/home/mylib/http/";
//     string path = "/123.txt";
//     response.init(srcDir,path,true,200,"");
//     string buffer;
//     response.makeResponse(buffer);
//     cout<<buffer<<endl;
// }

//发送数据的案例:srcDir为工作目录，path为要当前目录下要要发送的文件
//当要发送数据时，init的最后一个值为要发的数据，不可为空
void test2()
{
    httpresponse response;
    string srcDir = "";
    string path = "";
    response.init(true,200,"123");
    string buffer;
    response.makeResponse(buffer);
    cout<<buffer<<endl;
}



int main(){
    // test1();
    test2();
    return 0;
}