#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <errno.h>
#include "list.h"
//#include "server.h"

#define MAXLINE 1024      //
#define MAX_CLIENT 20     // 最多的在线用户数目 
#define LISTENQU 20       // 最大的监听数目
#define SERV_PORT 5000    // 端口号

#define TIME_SIZE 100
#define MAX_BUF_SIZE 1024
#define MAX_USER_NAME_SIZE 16
#define MAX_USER_PASSWORD_SEZE 16
#define MAX_NAME 25
#define MAX_PASSWORD 30
#define MAX_USER_ID 8

#define BUFSIZE 512

#define USER_EXITED 1

/*char chat_cmd[] = {
	{"SIGN",NULL,do_sign},
	{"LOGIN",NULL,do_login},
	{"CREATE",NULL,do_create},
	{"JOIN","group_name",do_join},
	{"INVITE",NULL,do_invite},
	{"DELETE","group_name",do_delete},
	{"CHAT",NULL,do_chat},
	{"GROUP",NULL,do_group}
};
*/
// 处理每个socket描述符的结构体
struct sockfd_opt{
	int fd;
	int (*do_task)(struct sockfd_opt *p_so);
	struct list_head list;
};

typedef struct sockfd_opt SKOPT,*P_SKOPT;
/*************************************************************
struct msg
{
    char name[MAX_NAME];
    char password[MAX_PASSWORD];
};

typedef struct link  
{
    int connfd;
    struct msg content;
    struct link *next;
}onlist，reglist;                               //注册用户

*******************************************************************/
struct User{
	int connfd;
	char user_id[MAX_NAME];
	char user_name[MAX_USER_NAME_SIZE];
};
struct User_sl{
	struct User user;
	char password[MAX_USER_PASSWORD_SEZE];
	struct User_sl *next;
};

typedef struct User_sl onlist,reglist;



//在线用户
struct UserOnline{
	struct User user;
	struct UserOnline *next;
};
//所有用户
struct UserList{
	struct User user;
	struct UserList *next;
};
//群
struct Group{
	struct UserList user;
	int group_id;
};

typedef struct Group group;

//消息内容
struct MessageContent{
	char time[TIME_SIZE];
	int length;
	char message[MAX_BUF_SIZE];
};


//私聊收、发
struct ReceiveChatPerson{
	int type;
	char user_id1[MAX_USER_NAME_SIZE];
	char user_id2[MAX_USER_NAME_SIZE];
	//struct MessageContent message;
	char message[MAXLINE];
};

typedef struct ReceiveChatPerson chat_mess,group_mess;     //私聊对象

//群消息内容
struct GroupMessage{
	int user_id;
	struct MessageContent message;
	struct GroupMessage *next;
};
//收群消息
struct ReceiveChatGroup{
	int group_id;
	int user_id;
	struct GroupMessage message;
};
//发群消息
struct SendMessageGroup{
	int user_id;
	int group_id;
	struct MessageContent message;
};
//聊天命令
struct chatcmd{
	char *alias;
	char *name;
	char *args;
	int(*handler)(int fd,char *cmd,char *args);
};

typedef struct chatcmd CHATCMD;
/*---------------------------------------------------------------------------------------------------
基本思路：
    ①客户端定时给服务器发送心跳包（案例中定时时间为3秒）；
    ②服务器创建一个心跳检测的线程，线程中每隔3秒对用户在线会话记录中的计数器进行加1操作（初始值为0）；
    ③服务器每次收到客户端的心跳包后，都将其在线会话记录中的计数器清零；
    ④当心跳检测线程中检测到某用户计数器已经累加到数值为5时
    （说明已经有15秒未收到该用户心跳包），就判定该用户已经断线，并将其从会话记录中清除出去。
*/
//心跳包节点********************************************************************
typedef struct session{
	char peerip[16];
	char name[10];
	int sockfd;
	int count;
	struct session *next;
}s_t;
/********************************************************************************/
//线程池节点定义
typedef struct worker 
{ 
     
    void *(*process) (void *arg); 
    void *arg; 
    struct worker *next; 
 
} CThread_worker; 
 
 
typedef struct 
{ 
    pthread_mutex_t queue_lock; 
    pthread_cond_t queue_ready; 
     
    CThread_worker *queue_head; 
 
     
    int shutdown; 
    pthread_t *threadid; 
     
    int max_thread_num; 
     
    int cur_queue_size; 
 
} CThread_pool;

int create_connect(struct sockfd_opt *p_so);
int send_reply(struct sockfd_opt *p_so);

//函数向线程池的任务链表中加入一个任务，
//加入后通过调用pthread_cond_signal (&(pool->queue_ready))唤醒一个出于阻塞状态的线程(如果有的话)。
int pool_add_worker (void *(*process) (void *arg), void *arg); 
//
void *thread_routine (void *arg);
void pool_init (int max_thread_num);//预先创建好max_thread_num个线程，每个线程执thread_routine ()函数
//用于销毁线程池，线程池任务链表中的任务不会再被执行，但是正在运行的线程会一直把任务运行完后再退出。
int pool_destroy ();
void* thread_routine (void *arg);    //线程执行函数
void* myprocess(void *arg);

int* get_client(void *);       //用于处理用户请求

int explain_reg(void *, char *);        //注册命令处理函数
int explain_load(void *, char *);       //登录命令处理函数
int chat_mode(void *);          //聊天模式处理函数
int get_online(void *);         //获取在先用户列表处理函数
int explain_sys(char *);         //解析服务器特殊命令
int manage_reg(struct User_sl*);
int chat_history(char * buf);
int reg_ready(void *, struct User_sl *);   //注册函数

int insert_onlist(struct User_sl*, void *);          //插入在线用户链表
int delete_online(char *);                  //从在线用户链表中删除

int insert_reglist(struct User_sl *,void *);  //插入注册用户链表

onlist *seek_user (char *);                 //查找在线的某个用户
int print_reg_iteror(struct User_sl *head_reg);  //遍历输出注册链表
int print_onlist_iteror(struct User_sl *head_on); //遍历输出在线链表
int rand_string(char *buf,int num);   //生成随机字符串

int char_date_handler(char *buf,struct ReceiveChatPerson *chat_mess);   //私聊命令处理函数
int group_date_handler(char *buf,struct Group *group);    //群消息处理函数

void update_process(int percent,int barlen);


/*void heart_handler(int sockfd,DATA_PACK *pd);
void *heart_check();
void check_handler();*/
#endif 
