#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <malloc.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include "server.h"

#define TRUE 1
#define FALSE 0


static void bail(const char *on_what)
{
    fputs(strerror(errno),stderr);
    fputs(":",stderr);
    fputs(on_what,stderr);
    fputc('\n',stderr);
    exit(1);
}

extern pthread_mutex_t reg_lock;
extern pthread_mutex_t on_lock;
extern pthread_mutex_t file_lock;

pthread_mutex_t RDWR_LOCK = PTHREAD_MUTEX_INITIALIZER;

extern reglist *head_reg;

onlist *head_on = NULL;   //在线用户列表头指针

/*******************************************************************
         函数名：insert_onlist
       功能描述：插入在线用户链表
       输入参数：新登录用户的用户名
       输出参数：无
           时间：2016.06.27
         返回值：0
*******************************************************************/
int insert_onlist(struct User_sl *temp_node,void *sockfd)
{
     printf ("插入在线用户链表操作中...\n");
     onlist *temp = (onlist *)malloc(sizeof (onlist));
     strcpy(temp -> user.user_id, temp_node -> user.user_id);
     strcpy(temp -> user.user_name, temp_node -> user.user_name);
     strcpy(temp -> password, temp_node -> password);
     temp -> user.connfd = (int)sockfd;
     //temp -> user.user_id = ;W
     printf("user _id %s\n", temp -> user.user_id);
     printf("user_name %s\n", temp -> user.user_name);
     printf("user_passwd %s\n", temp -> password);
     printf("userfd %d\n", temp -> user.connfd);
     temp -> next = head_on;
     head_on = temp;

     printf ("插入在线用户链表成功\n");
     printf ("插入在线链表之后链表存储为\n");
     //print_onlist_iteror(head_on);
     return 0;
}
/*****************************************************************
         函数名：insert_reglist
       函数功能：插入注册用链表
       输入参数：新注册用户的用户名和密码
       输出参数：无
           时间：2016.06.27
         返回值：无
*****************************************************************/
int insert_reglist(struct User_sl *temp_node, void *sockfd)
{
    pthread_mutex_lock(&reg_lock);
    printf ("插入注册用户链表中...\n");

    reglist *temp =(reglist *)malloc (sizeof (reglist));
    strcpy (temp -> user.user_id, temp_node->user.user_id);
    strcpy (temp -> user.user_name, temp_node->user.user_name);
    strcpy (temp -> password, temp_node->password);
    
    temp -> user.connfd = (int)sockfd;

    temp -> next = head_reg;
    head_reg = temp;

    printf ("插入注册用户链表成功\n");
    pthread_mutex_unlock(&reg_lock);
    print_reg_iteror(head_reg);
    printf("-------------------------------------------\n");
    return 0;
}

/******************************************************************
       函数名：delete_onlist
     功能描述：将用户从在线用户链表中删除
     输入参数：要求删除的用户的用户名
     输出参数：无
         时间：2016.06.27
       返回值：无
******************************************************************/
int delete_online(char *name)
{
    pthread_mutex_lock(&on_lock);

    int flag = 1;
    onlist *temp = head_on;
    onlist *std = NULL;  //定义一个中间指针 
    for (temp; temp != NULL; temp = temp -> next)
    {
        std = temp -> next;
        if (strcmp(std -> user.user_name, name) == 0)  //从在线用户链表中找出该用户
	    {
            flag = 0;
	        temp -> next = std -> next;
	        free (std);
	    }
        if(0 == flag)
        {
            printf("已经删除用户：%s \n",name);
        }
    }

    pthread_mutex_unlock(&on_lock);

    printf ("从在线用户链表中删除成功\n");
    return 0;
}

/*******************************************************************
      函数名：seek_user
	功能描述：查找在某个在线用户
	输入参数：要求查找的用户的用户名
	输出参数：无
        时间：2016.06.27   
	  返回值：该用户数据节点的指针
*******************************************************************/
onlist *seek_user(char *name_id)
{
    printf ("查找用户中....\n");

    int flag = 1;
    onlist *temp = head_on;
    for (temp; temp != NULL; temp = temp -> next)
    {
        //printf("temp -> user.user_id%s\n", temp -> user.user_id);
        if (strcmp (temp -> user.user_id, name_id) == 0)
	    {
	        return temp;
	    }
    }
    return NULL;
}

/********************************************************************
      函数名：get_client 
        功能：处理该用户的的命令以及数据处理
    输入参数：accept函数的返回值
    输出参数：无
        时间：2016年6月27日
      返回值：无
********************************************************************/
int* get_client (void *sockfd)
{
    char user[MAX_NAME];  //当前用户

    char temp_buf[MAX_BUF_SIZE];
    char buf[MAXLINE];
    char *p;
    int rec = 1;
    int reg_rt ;   //注册处理函数返回值
    int load_rt ;  //登录处理函数返回值
    int statue;
    int read_error;

   // struct fd_set fds; 
    struct timeval timeout={3,0}; //select等待3秒，3秒轮询，要非阻塞就置0 
    char buffer[256]={0}; //256字节的接收缓冲区 
    
    char reg[] = "SIGN";   
    char load[] = "LOGIN";
    char quit[] = "QUIT";


    if ((int)sockfd < 0)
    {
        printf ("\n新用户进入聊天室失败!\n");
    }
    else 
    {
        printf ("\n-----------------------新用户进入聊天室成功!-------------------------------\n");
        //线程死循环
            memset(buf, 0, sizeof(buf));
	       if ((rec = read ((int)sockfd, buf, MAXLINE)) < 0 && errno != EINTR)       //读取登陆/注册命令信息
	       {
	            printf ("读取用户命令失败！,用户已退出\n");
                delete_online (user);
                read_error = USER_EXITED;
                return read_error;
            }
/*********************************    处理注册命令   *********************************************/	        
            //进入聊天室后选择 登录 或 注册
            if(rec < 0 && errno != ECONNRESET) //表示尝试再次建立连接
                bail("read()");
            }
            if (strstr(buf,reg) != NULL)
            {
		        printf ("进入注册模式.\n");
                memset(temp_buf, 0, sizeof(temp_buf));
                strcpy(temp_buf, buf);
                printf("temp_buf:%s\n",temp_buf);
		        //调用注册处理函数
                reg_rt = explain_reg ( (void *)sockfd, temp_buf);
                if (TRUE == reg_rt)
		        {
                    printf("欢迎新用户注册成功！请登录......\n");
		        }
            }
/*******************************   处理登录命令   **************************************************/
            else if (strstr(buf,load) != NULL)
	        {
		        printf ("进入登录模式\n");
                memset(temp_buf, 0, sizeof(temp_buf));
                strcpy(temp_buf, buf);
                printf("传入temp_buf:%s\n",temp_buf);
                load_rt = explain_load((void *)sockfd, temp_buf);
                printf ("%s\n", user);

                if (TRUE == load_rt)
		        {
		            printf ("开始聊天.\n");
		            chat_mode ((void *)sockfd);
		        }
		        else 
		        {
		        }
            }
/**********************************    处理退出命令    *************************************************/
	        else if (strncmp (quit, buf, sizeof (quit)) == 0)  // 处理退出命令
            {
		         printf ("%s用户已退出\n", buf);
	        }
}

/*******************************************************************
       函数名：explain_reg
     功能描述：处理用户的注册命令
     输入参数：缓存的首地址和端口描述符
     输出参数：无
       返回值：若成功返回0
*******************************************************************/
int explain_reg(void *sockfd, char *temp_buf)
{
    int rec; //读取用户端数据的文件描述符
    char buf[MAXLINE];  //缓存
    int name_exist = 0;

    char name[MAX_USER_NAME_SIZE];
    char passwd[MAX_USER_PASSWORD_SEZE];

    char exist[] = "sign_name_exist";         //注册名已存在标志
    char no_exist[] = "sys_no_exit";    //注册名不存在

    reglist temp_node;

    printf ("注册处理开始\n");
    memset (buf, 0, sizeof (buf));  //清空缓存
    memset(name, 0, sizeof(name));
    memset(passwd, 0, sizeof(passwd));
   

    if(reg_load_handler(temp_buf,name,passwd) == TRUE)
    {
        reglist *pi = head_reg;

        memset(temp_node.user.user_name, 0, sizeof(temp_node.user.user_name));
        memset(temp_node.password, 0, sizeof(temp_node.password));
        memset(temp_node.user.user_id, 0, sizeof(temp_node.user.user_id));

        strcpy(temp_node.user.user_name,name);

        strcpy(temp_node.password,passwd);

        printf("user name ：%s\nuser passwd is :%s\n", temp_node.user.user_name,temp_node.password);
        
        //printf("写入注册列表文件\n");
        for (; pi != NULL; pi = pi -> next)
        {
            //if (strcmp (temp_node -> user.user_name, pi -> user.user_name) == 0) //新用户名存在
            int er = strcmp(temp_node.user.user_name, pi -> user.user_name);
            printf("er = %d\n",er);
            if ( 0 == strcmp (temp_node.user.user_name, pi -> user.user_name)) //新用户名存在
            {
                printf ("用户名已存在.\n");

                //输出已注册用户
                printf("输出已注册用户\n");
                print_reg_iteror(head_reg);
                memset(buf, 0, sizeof (buf));
                strcpy(buf, exist);
                name_exist = 1;
                
                //通知用户端用户名已存在
                if (write((int)sockfd, buf, strlen (buf)) < 0)
                {
                }
                else 
                {
                    break;
                }
            }
            //else if (pi -> next == NULL)
        }
        if(pi == NULL){
             printf("准备进入注册模式\n");
             reg_ready((void *)sockfd, &temp_node);
             return TRUE;
        }
    }
}
/*******************************************************************
       函数名：reg_ready
     功能描述：处理用户的注册命令
     输入参数：缓存的首地址和端口描述符
     输出参数：无
         时间：2016.06.28
       返回值：若成功返回0
*******************************************************************/  
int reg_ready(void *sockfd, struct User_sl *temp_node)
{
    char success[] = "sys_success";     //注册成功
    char no_exist[] = "sys_no_exist";    //注册名不存在

    char name_buf[MAX_NAME];
    char pw_buf[MAX_PASSWORD];
    char buffer[MAXLINE];
    int rec,i;
    int usr_id;
    printf ("可以注册!\n");
    // 通知用户可以注册
    memset(buffer, 0, sizeof (buffer));
    memset(pw_buf, 0, sizeof (pw_buf));
    memset(name_buf, 0, sizeof(name_buf));

    printf ("注册中...\n");  
	printf("%s\n", temp_node-> user.user_name);

    //写入注册用户列表文件
//-----------------------------------------------------------------    manage_reg(temp_node);
    printf ("注册成功！\n");
    memset(temp_node -> user.user_id, 0, sizeof (temp_node -> user.user_id));
    memset(buffer, 0, sizeof (buffer));
    printf("user_id is %s\n",temp_node -> user.user_id);
    rand_string(buffer,6);
    //strcat(buf,"\n");
    strcpy(temp_node -> user.user_id,buffer);
    printf("user_id is %s\n",temp_node -> user.user_id);
    manage_reg(temp_node);
    if((rec = write((int)sockfd, buffer, strlen (buffer))) > 0)
    {
        printf("返回ID成功！\n");
    }
    //插入注册用户链表
    insert_reglist(temp_node, (void *)sockfd);
    //print_reg_iteror(head_reg);
    return TRUE;
}

/************************************************************************
      函数名：explain_load
    功能描述：处理用户的登录操作
    输入参数：缓存的首地址及端口描述符
    输出参数：无
        时间：2016.06.28
      返回值：若成功返回0
************************************************************************/
int explain_load(void *sockfd, char *temp_buf)
{
    printf ("现在开始登录处理.\n");
    int rec; //读端口的返回值 
    char user_id_buf[MAX_NAME];
    //char user_name_buf[MAX_NAME];
    char pw_buf[MAX_PASSWORD];
   
    char right[] = "login_success";    //登陆成功
    char wrong[] = "login_failed";    //登录失败
   
    onlist *temp = (onlist *)malloc(sizeof (onlist));
 
    reg_load_handler(temp_buf,user_id_buf,pw_buf);

    strcpy(temp -> user.user_id, user_id_buf);
    strcpy(temp -> password, pw_buf);

    reglist *pi = head_reg;
    if (( seek_user(temp -> user.user_id)) != NULL)      //查询用户是否在线
    {
        //printf ("用户已登录!\n");
        //通知用户登录失败
        //write ((int)sockfd, wrong, strlen(wrong));
        //continue;
        //return FALSE;
    }
    else
    {
        printf("开始登陆信息验证**********************************：\n");
        printf("temp -> user.user_id is %s\n", temp -> user.user_id);
        printf("temp -> passwordis %s\n", temp -> password);
        for (pi; pi != NULL; pi = pi -> next)
        {          
            if ((strcmp (pi -> password, temp -> password) == 0) &&        //登录用户名，密码验证
                (strcmp (pi -> user.user_id, temp -> user.user_id) == 0))
            {
                printf ("登录数据操作中...\n");
                printf("temp -> user.user_name is %s\n", pi -> user.user_name);
                strcpy( temp -> user.user_name,pi -> user.user_name);
   
                // 插入在线用户链表
                insert_onlist(temp, (void *)sockfd);
                sleep(1);
                printf ("用户%s录成功.\n",temp -> user.user_name);
                //通知用户端成功
                write ((int)sockfd, right, strlen(right)); 
                return TRUE;
            }
        }
        if (pi == NULL)
        {
            write ((int)sockfd, wrong, strlen(wrong));  //通知用户失败

            printf ("登录失败,请检查用户名和密码\n");
            //continue;
            return FALSE;
        }  
    }
}

/********************************************************************
       函数名：chat_mode
     功能描述：提供过户不同的聊天模式
     输入参数：套接口描述符及该用户的名字
     输出参数：无
       返回值：0
********************************************************************/
int chat_mode (void *sockfd)
{
    char buf[MAXLINE];

    char privite[] = "CHAT";    //私聊命令
    char public[] = "GROUP";      //群聊命令
    char end[] = "DELETE";        //退出私聊模式命令
    char end_pub[] = "QUIT";    //退出群聊模式命令
    char online[] = "ONLINE";      //获取在线用户列表
    //char creat[] = "CREATE";
    //char invite[] = "INVITE";
    char write_message[MAXLINE];
    char object[MAXLINE];        //私聊对象名
    onlist *temp = NULL;

    char user[MAX_NAME];         //在线用户链表找到的用户
    char content[MAXLINE];       //发送的内容
    int ob_len;                  //对象名长度

    int j = 0,ret;
    int client_n[MAX_CLIENT] = {0};
    

    while(1)
    {
        memset (buf, 0, sizeof (buf));
        if ((ret = read ((int)sockfd, buf, MAXLINE)) < 0)
        {
            printf ("获取用户聊天命令失败！\n");
        }
        else if(ret > 0)
        {
            printf ("用户聊天命令：%s\n", buf);
        }
		else
		{
			sleep(1);
			continue;
		}
        
        //  用户选择群聊
        if (strstr(buf,public) != NULL)
        {
            printf("进入群聊天模式\n");
            group_mess group;
            while (1)
	        {
	            memset(buf, 0, sizeof(buf));
                if ((read ((int)sockfd, buf, MAXLINE)) < 0)
                {
	                printf ("获取用户聊天内容失败！\n");
                    sleep(1);
                    continue;
	            } 
                memset(write_message,0,sizeof(write_message));
                strcpy(write_message,buf);
                printf("收到群处理消息%s\n", buf);
                date_handler(buf,&group);
               
                printf ("群ID %s,发送用户id %s\n",group.user_id1,group.user_id2);
	            if (strstr (buf, end_pub) != NULL)
	            {
	                printf ("结束群聊！\n");
		            continue;                    
	            }
	            else
                {
		           onlist *on = head_on;
                   printf("开始群发。。。。。。。。\n");
	               for (on; on != NULL; on = on -> next)
	               {
                        if(strcmp(group.user_id1,on -> user.user_id) == 0)
                        {
                            continue;
                        }
		                printf ("user_id:%s\n", on -> user.user_id);
                        printf("群消息%s\n",buf);
		                write (on -> user.connfd, write_message,strlen(write_message));
                        //sleep(1);
	                }   
                }
    	    }
        }
        // 用户选择私聊
        else if (strstr(buf, privite) != NULL )
        {
            printf ("私聊模式.\n");
            //发送用户列表
            get_online((void *)sockfd);
            printf("发送用户列表\n");
            chat_mess chat;
            while(1)
            {
	            memset(content, 0, sizeof (content));
                printf("等待用户聊天信息\n");
	            if (read((int)sockfd, content, sizeof (content)) <= 0)
	            {
	                printf ("获取私聊信息失败!\n"); 
                    sleep(1);
                    continue;
	            }
                else
	            {   
                    memset(write_message,0,sizeof(write_message));
                    strcpy(write_message,content);
					if(strstr(content,"QUIT") != NULL)
						break;
                    printf("读到用户聊天信息%s\n",content);
                    //分割消息字段
                    date_handler(content,&chat);

	                printf ("用户id1 %s目的id %s\n",chat.user_id1,chat.user_id2);

                    memset(buf, 0, sizeof(buf));
                    strcpy(buf,chat.user_id2);
                    strcat(buf,"%%");
                    strcat(buf,chat.user_id1);
                    strcat(buf,"^^");
                    strcat(buf,chat.message);

	                temp = seek_user(chat.user_id2);    //从在线用户链表找到该用户节点
                    printf("找到对应ID%s\n", temp -> user.user_id);
	                if (temp == NULL)
	                {
	                    printf ("用户不在线,写入日志文件\n");
                        chat_history( buf); 
                        sleep(1);
	                    continue;
	                }

                    if(write (temp -> user.connfd, write_message,strlen(write_message)) > 0)
                    {
                        printf("服务器转发成功\n");
                        printf("发送:%s\n",buf);
                    }
                    else
                    {
                        printf("转发失败！\n");
                        sleep(1);
                        continue;
                    }
	            }
            }
        }
        // 获取在线用户列表
        else if (strstr(buf, online) != NULL)
        {
            printf ("获取在线用户\n");
            get_online((void *)sockfd);
            sleep(1);
	        continue;
        }
    }
}

/********************************************************************
       函数名： manage_reg.c
     功能描述： 处理注册用户列表,并将其写入文件
     输入参数： 用户的有关信息
     输出参数： 无
       返回值： 0
********************************************************************/
int manage_reg(struct User_sl *temp)
{
    int fd ;  //打开文件的文件描述符
    int ret;  //写入文件的文件描述符  
    
    pthread_mutex_lock(&file_lock);
    fd = open ("reglist.c", O_RDWR | O_APPEND);
    if (fd > 0)
    {
        printf ("打开文件成功!\n");
    }
    ret = write(fd,&temp -> user.user_id,sizeof (temp -> user.user_id));
    ret = write(fd,&temp -> user.user_name,sizeof (temp -> user.user_name));
    ret = write(fd,&temp -> password,sizeof (temp -> password));
    printf("注册用户ID %s\n", temp -> user.user_id);
    printf("注册用户名 %s\n", temp -> user.user_name);
    printf("注册用户密码 %s\n", temp -> password);
    if (ret < 0)
    {
        printf ("写入文件失败\n");
    }
    close (fd);
    pthread_mutex_unlock(&file_lock);

    return 0;
}

/***************************************************************************
        函数名：get_online.c
      功能描述：获取在线用户
      输入参数：用换套接口描述符
      输出参数：无
        返回值：0
***************************************************************************/
int get_online (void *sockfd)
{
    char buf[MAXLINE];
    char end_online[MAXLINE] = "online_end";
    onlist *temp = head_on;

    for (temp ; temp != NULL; temp = temp -> next)
    {
        memset(buf, 0, sizeof (buf));
        strcpy(buf,temp -> user.user_id);
        strcat(buf,"%%");
        strcat(buf, temp -> user.user_name);
        if (write((int)sockfd, buf, strlen (buf)) < 0)
        {
            printf ("发送在线用户链表%s失败\n",temp -> user.user_id);
        }
        sleep(1);
    }
    memset(buf, 0, sizeof (buf));
    strcpy (buf, end_online);
    if (write((int)sockfd, buf, strlen (buf)) < 0)
    {
        printf ("发送在线用户链表失败\n");
    }
    printf ("反馈用户链表：%s \n",buf);
    return 0;
}

/*******************************************************************
       函数名：reg_load_handler
     功能描述：解析用户登录注册命令
     输入参数：接受缓存
     输出参数：结构体对象指针
         时间：2016.06.29
       返回值：若成功返回0
*******************************************************************/ 
int reg_load_handler(char *buf,char *name,char *passwd)
{
    int i = 1;

    char temp[MAX_BUF_SIZE];
    int id[4];
    char *p;
    char *delim = "%%";

    printf("开始提取用户名信息。。。\n");

    strcpy(temp,buf);
    strcat(temp,"\n");
    printf("%s ",strtok(temp,delim));            //将%%用NULL(/0)替换

    while((p = strtok(NULL,delim)))
    {
        i++;
        if(2 == i)
        {
            strcpy(name,p);
            //strcpy(temp_node -> user.user_name,name);
        }
        if(3 == i)
        {
            strcpy(passwd,p);
            //strcpy(temp_node -> password,passwd);
        }
    }
    printf("\n");
    printf("name is :%s\n",name);
    printf("passwd is :%s\n",passwd);
    return TRUE;
}
/*******************************************************************
       函数名：print_reg_iteror
     功能描述：遍历输出注册用户链表
     输入参数：注册链表头指针
     输出参数：无
         时间：2016.07.01
       返回值：若成功返回0
*******************************************************************/ 
int print_reg_iteror(struct User_sl *head_reg)
{
    reglist *pt= head_reg;
    for (pt; pt != NULL; pt = pt -> next)
    {
        printf(" id %s;name: %s ;passwd:%s\n",pt -> user.user_id,pt -> user.user_name,pt -> password);
        //printf(" %s \n",pt -> password);
    }
    return TRUE;
}
/*******************************************************************
       函数名：rand_string
     功能描述：生成随机ID值
     输入参数：ID存储区
     输出参数：无
         时间：2016.07.01
       返回值：若成功返回0
*******************************************************************/ 
int rand_string(char *buf,int num)
{
    //char buffer[256+1];
    int i,leng;
    memset(buf, 0, sizeof (buf));
    buf[strlen(buf)-1] = '\0';
    if(buf[0] == '\0')
    {
    }
    else
    {
        leng = atoi(buf);
    }
    srand((int)time(0));
    //bzero(buf, num);
    for (i = 0; i < num; i++)
    {
        //buf[i] = 'a' + ( 0+ (int)(26.0 *rand()/(RAND_MAX + 1.0)));
        buf[i] = '0' + rand()%10;
    }
    printf("rand is %s \n",buf);
    return TRUE;
}

/*******************************************************************
       函数名：print_onlist_iteror
     功能描述：遍历输出在线用户链表
     输入参数：在线链表头指针
     输出参数：无
         时间：2016.07.01
       返回值：若成功返回0
*******************************************************************/ 
int print_onlist_iteror(struct User_sl *head_on)
{
    onlist *po= head_on;
    for (po; po != NULL; po = po -> next)
    {
        printf("print_onlist_iteror...\n");
        printf("%s\n",po -> user.user_id);
        printf(" %s \n",po -> user.user_name);
        printf(" %s \n",po -> password);
    }
    return TRUE;
}

/*******************************************************************
         函数名：date_handler
       功能描述：解析聊天字段
       输入参数：接受缓冲，聊天结构体对象
       输出参数：无
           时间：2016.06.27
         返回值：0
*******************************************************************/
int date_handler(char *buf,struct ReceiveChatPerson *chat)
{
    int i = 1,j = 0;
    char temp[MAX_BUF_SIZE];
    char *p,*q;
    char *delim1 = "%%";
    char *delim2 = "^^";

    printf("开始提取用户名信息。。。\n");
    //提取message
    q = strtok(buf,delim2);
    strcpy(temp,q);
    p = strtok(NULL,delim2);
    strcpy(chat -> message,p);
    //printf("chat -> message:%s\n", chat -> message);

    //提取userid1
    p = strtok(temp,delim1);
    strcpy(chat -> user_id1,temp);
    //printf("chat -> user_id1:%s\n", chat -> user_id1);

    p = strtok(NULL,delim1);
    strcpy(chat -> user_id2,p);
    //printf("chat -> user_id2:%s\n", chat -> user_id2);
}

/*******************************************************************
         函数名：chat_history
       功能描述：聊天日志缓存
       输入参数：聊天协议字段
       输出参数：无
           时间：2016.06.27
         返回值：0
*******************************************************************/
int chat_history(char * buf)
{
    int fd1 ;  //打开文件的文件描述符
    int ret;  //写入文件的文件描述符  
    chat_mess chat_history;
    pthread_mutex_lock(&file_lock);
    fd1 = open ("history.txt", O_RDWR | O_APPEND );
    if (fd1 > 0)
    {
        printf ("打开文件成功!\n");
    }
    date_handler(buf,&chat_history);
    ret = write(fd1,&chat_history. user_id1,sizeof (chat_history . user_id1));
    ret = write(fd1,&chat_history.user_id2,sizeof (chat_history . user_id2));
    ret = write(fd1,&chat_history . message,sizeof (chat_history .message));
    printf("存入目的ID %s\n", chat_history .user_id1);
    printf("存入发送方ID %s\n", chat_history .user_id2);
    printf("存入消息内容 %s\n", chat_history . message);
    if (ret < 0)
    {
        printf ("写入文件失败\n");
    }
    close (fd1);
    pthread_mutex_unlock(&file_lock);

    return 0;
}