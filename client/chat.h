#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <termios.h>
#include <pwd.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>


#define MAX_BUF_SIZE 1024
#define MAX_USER_NAME_SIZE 18
#define MAX_USER_PASSWORD_SEZE 18
#define LOGIN_ON 1
#define LOGIN_OFF 0 

#define BUFSIZE 512
#define CMD_NUM 8 
//#define SERVER_IP "127.0.0.1"
#define SERVER_IP "192.168.191.6"
//#define SERVER_IP "192.168.191.1"
//#define SERVER_IP "192.168.1.101" 
//#define SERVER_IP "10.100.10.48"
#define SERVER_PORT 5000
#define CMD_MESSAGE_LENGTH 80
#define SIGN_Y 1
#define SIGN_N 0
#define QUIT_CHAT 1
#define GROUP_NUMBER_LENGTH 8
#define USER_NUMBER_LENGTH 8
#define DELIMITER "%%"
#define DELIMITER_MESSAGE "^^"


#define TIME_SIZE 30

#define LOGIN_SUCCESS "login_success"
#define LOGIN_FAILURE "login_failed"
#define SIGN_NAME_EXISTED "sign_name_exist"
#define ONLINE_LIST_END "online_end"
//time_length =19

void do_sign(int sockfd,char *name);
void do_login(int sockfd,char *name);
void do_create(int sockfd,char *name);
void do_join(int sockfd,char *name);
void do_invite(int sockfd,char *name);
void do_delete(int sockfd,char *name);
void do_chat(int sockfd,char *name);
void do_group(int sockfd,char *name);
void is_login(int sockfd);
int send_message(int sockfd,char *sign_message);
void read_message_status(int sockfd,char *read_message);
int check_chat_cmd(char *cmd);
int input_cmd(char *cmd,int size);
void get_local_time(char *time_buff);
void get_online_list(int sockfd,char *read_message);
void get_chat_message(int sockfd,char *read_message);
void *thread_read(void *arg);

void bail(const char *on_what){
	fputs(strerror(errno),stderr);
	fputs(":",stderr);
	fputs(on_what,stderr);
	fputc('\n',stderr);
	exit(1);
}

void terminal_echo_off(int fd){
	struct termios old_terminal;
	tcgetattr(fd,&old_terminal);
	old_terminal.c_lflag &= ~ECHO;
	tcsetattr(fd,TCSAFLUSH,&old_terminal);
}

void terminal_echo_on(int fd){
	struct termios old_terminal;
	tcgetattr(fd,&old_terminal);
	old_terminal.c_lflag |= ECHO;
	tcsetattr(fd,TCSAFLUSH,&old_terminal);
}




struct parameter{
	int sockfd;
	char *read_message;
};

struct User{
	int user_id;
	char user_name[MAX_USER_NAME_SIZE];
	struct User *next;
};



/*
struct User_sl{
	struct User user;
	char password[MAX_USER_PASSWORD_SEZE];
};


//在线用户
struct UserOnline{
	struct User user;
	struct UserOnline *next;
};

*/

//所有用户
struct UserList{
	struct User user;
//	int statues;
	struct UserList *next;
};


//群
struct Group{
	struct UserList user;
	int group_id;
};

//消息内容
struct MessageContent{
	char *time;
	char *length;
	char *message;
};
//私聊收、发
struct ReceiveChatPerson{
//	char *cmd;
	char *user_id1;
	char *user_id2;
	struct MessageContent message;
};
//群消息内容

//群消息
struct ReceiveChatGroup{
	char *group_id;
	char *user_id;
	struct MessageContent message;
	struct ReceiveChatGroup *next;
};
//聊天命令
struct chatcmd{
	char *alias;
	char *name;
	char *args;
	int(*handler)(int fd,char *cmd);
};

typedef struct User UserList;
typedef struct chatcmd CHATCMD;
typedef struct ReceiveChatPerson ChatMessage;
typedef struct ReceiveChatGroup GroupMessage;
//int set_message_to_send(int sockfd,char *number1,char *number2,ChatMessage chat,GroupMessage group);
typedef struct parameter Parameter;