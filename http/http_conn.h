#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include<sys/uio.h>
#include<map>
#include "locker.h"

class http_conn
{
public:
    //设置读取文件名称的大小
    static const int FILENAME_LEN = 200;
    //设置读取缓冲区的大小
    static const int READ_BUFFER_SIZE = 2048;
    //设置写缓冲区的大小
    static const int WRITE_BUFFER_SIZE = 1024;
    //报文请求方法，本项目只用GET和POST
    enum METHOD { GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT, PATCH };
    //主状态机的状态，分别表示：当前正在分析请求行、当前正在分析请求头、当前正在分析请求内容
    enum CHECK_STATE { CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER, CHECK_STATE_CONTENT };
    /*服务器处理HTTP请求的结果:NO REQUEST表示请求不完整，需要继续渎取客户数据;
    GET_REQUEST表示获得了一个完整的客户请求;BAD_REQUEST表示客户请求有语法错误；
    FORBIDDEN_REQUEST表示客户对资源没有足够的访问权限﹔INTERNAL_ERROR表示服务器内部错误﹔
    CLOSED CONNECTTON表示客户端已经关闭连接了*/
    enum HTTP_CODE { NO_REQUEST, GET_REQUEST, BAD_REQUEST, NO_RESOURCE, FORBIDDEN_REQUEST, FILE_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION };
    /*从状态机的三种可能状态，即行的读取状态，分别表示﹔读取到一个完整的行、行出错和行数据尚且不完整*/
    enum LINE_STATUS { LINE_OK = 0, LINE_BAD, LINE_OPEN };

public:
    http_conn(){}
    ~http_conn(){}

public:
    //初始化新接受的连接
    void init( int sockfd, const sockaddr_in& addr );
    //关闭连接
    void close_conn( bool real_close = true );
    //处理客户请求
    void process();
    //非阻塞读操作
    bool read();
    //非阻塞写操作
    bool write();

private:
    //初始化连接
    void init();
    //解析HTTP请求
    HTTP_CODE process_read();
    //填充HTTP应答
    bool process_write( HTTP_CODE ret );

    /*被process_read调用以分析HTTP请求*/
    HTTP_CODE parse_request_line( char* text );
    HTTP_CODE parse_headers( char* text );
    HTTP_CODE parse_content( char* text );
    HTTP_CODE do_request();
    char* get_line() { return m_read_buf + m_start_line; }
    LINE_STATUS parse_line();
    /*被process_write调用以填充HTTP应答*/
    void unmap();
    bool add_response( const char* format, ... );
    bool add_content( const char* content );
    bool add_status_line( int status, const char* title );
    bool add_headers( int content_length );
    bool add_content_length( int content_length );
    bool add_linger();
    bool add_blank_line();

public:
    //所有socket上的事件被注册到同一个epoll内核事件表中，所以将epoll文件内描述符设置为静态的
    static int m_epollfd;
    //统计用户数量
    static int m_user_count;

private:
    /* 该HTTP连接的socket和对方的socket地址*/
    int m_sockfd;
    sockaddr_in m_address;

    //读缓冲区
    char m_read_buf[ READ_BUFFER_SIZE ];
    //标记读缓冲区中已经读入的客户数据的最后一个字节的下一个位置
    int m_read_idx;
    //当前正在分析的字符在缓冲区的位置
    int m_checked_idx;
    //当前正在解析的行的起始位置
    int m_start_line;
    //写缓冲区
    char m_write_buf[ WRITE_BUFFER_SIZE ];
    //写缓冲区待发送的字节数
    int m_write_idx;

    //主状态机当前所处状态
    CHECK_STATE m_check_state;
    //请求方法
    METHOD m_method;

    //客户请求的目标文件的完整路径，内容等于doc_root)(网站根目录)+m_url
    char m_real_file[ FILENAME_LEN ];
    //客户请求的目标文件的文件名
    char* m_url;
    //HTTP协议版本号，我们仅支持HTTP1.1
    char* m_version;
    //主机名
    char* m_host;
    //HTTP请求的消息体的长度
    int m_content_length;
    //HTTP请求是否要求保持连接
    bool m_linger;

    //客户请求的目标文件被mmap到内存中的起始位置
    char* m_file_address;
    /*目标文件的状态，通过它可以判断文件是否存在、是否为根目录、是否可读，并获取文件大小等信息 */
    struct stat m_file_stat;
    
    struct iovec m_iv[2];
    //写内存块的数量
    int m_iv_count;

    int cgi;        //是否启用的POST
    char *m_string; //存储请求头数据
    int bytes_to_send;//剩余发送字节数
    int bytes_have_send; //已发送字节数
};

#endif
