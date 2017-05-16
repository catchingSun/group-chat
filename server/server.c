//
//                       _oo0oo_
//                      o8888888o
//                      88" . "88
//                      (| -_- |)
//                      0\  =  /0
//                    ___/`---'\___
//                  .' \\|     |// '.
//                 / \\|||  :  |||// \
//                / _||||| -:- |||||- \
//               |   | \\\  -  /// |   |
//               | \_|  ''\---/''  |_/ |
//               \  .-\__  '-'  ___/-. /
//             ___'. .'  /--.--\  `. .'___
//          ."" '<  `.___\_<|>_/___.' >' "".
//         | | :  `- \`.;`\ _ /`;.`/ - ` : | |
//         \  \ `_.   \_ __\ /__ _/   .-` /  /
//     =====`-.____`.___ \_____/___.-`___.-'=====
//                       `=---='
//
//
//     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//               佛祖保佑         永无BUG
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h> 
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/select.h>
#include <arpa/inet.h>

#include "list.h"
#include "server.h"

//将注册的信息写入文件时加锁
pthread_mutex_t file_lock = PTHREAD_MUTEX_INITIALIZER;

//对在线用户链表加锁
pthread_mutex_t on_lock = PTHREAD_MUTEX_INITIALIZER;

//对注册用户链表加锁
pthread_mutex_t reg_lock = PTHREAD_MUTEX_INITIALIZER;

//声明一个pool全局变量
static CThread_pool *pool = NULL; 

int i = 0;
int max_i = -1;          
int client[MAX_CLIENT];  //当前在线用户列表
fd_set read_fds,old_fds;
int max_fd;
int num = 0; //链表中元素个数
reglist *head_reg  = NULL;  //注册用户列表的头指针

pthread_t tid,pt;

LIST_HEAD(fd_queue);



static void bail(const char *on_what){
    fputs(strerror(errno),stderr);
    fputs(":",stderr);
    fputs(on_what,stderr);
    fputc('\n',stderr);
    exit(1);
}

int main()
{
    P_SKOPT p_so;
    socklen_t optlen;
    int sr = 1; //防止服务器端关闭后再次开启，绑定出错

    struct timeval time_out;
    time_out.tv_sec = 0;
    time_out.tv_usec = 50000;
    struct sockaddr_in servaddr;  
      //客户端的地址和端口号

    int listenfd;      //用来表示监听的套接口 //
    int cliaddr_len;   //缓冲区cliaddr的长度 
   
    int ret;
    int n,i; //已准备好的文件描述符个数
    
    int err;
    int sock_statue;
 /*   for(i=0;i<101;++i){
        update_process(i,50);
        fflush(stdout);
        sleep(1);
    }*/

    printf ("\n********************************************\n");
    printf ("启动服务器...");
    printf( "            网络工程1301班课程设计小组版权所有\n" );
    //printf( "\n    服务器本次启动时间：" );
    //printf ("\n********************************************\n");
    for (i = 0; i < MAX_CLIENT; i++)  //将用户队列里的值初始化为 -1
    {
        client[i] = -1;
    }
  
 /**********************************************************************
    功能描述：将注册用户信息从文件加载到注册链表
 **********************************************************************/
    
    read_register_file(); //读取注册表

/*************************************************************************
    服务器端建立套接口并监听
*************************************************************************/
    if ((listenfd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf ("建立套接口失败!\n");
	    exit (1);
    }
    else
    {
        printf ("\n建立套接口成功!\n");
    }
    
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);

    optlen = sizeof(sr);
    sock_statue = setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&sr,optlen); //当服务器断开连接后，去掉2MSL
    if(sock_statue)
        bail("setsockopt()");


    // 绑定套接口
    if (bind (listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        printf ("套接口绑定失败！\n");
	    exit (1);
    }
    else 
    {
        printf ("绑定成功！\n");
    }

    if(listen (listenfd, LISTENQU) == -1)
        bail("listen()"); // 设置监听状态
    //pthread_create(&pt,NULL,&heart_check,NULL);
    printf ("欢迎来到聊天室！\n");

     max_fd = listenfd;
    if(init(listenfd))
        bail("listen()");
    for(;;)
    {
        read_fds = old_fds;

        n = select(max_fd + 1,&read_fds,NULL,NULL,NULL);
        for(;n > 0;n--){
            list_for_each_entry(p_so,&fd_queue,list)
            if(FD_ISSET(p_so->fd,&read_fds))
                p_so->do_task(p_so);
        }
    }
    printf("selset %d\n",n);
    close(listenfd);
    return 0;
/******************************************************************************/
}

int send_reply(struct sockfd_opt *p_so)
{
    char reqBuf[BUFSIZE];
    char dtfmt[BUFSIZE];
    time_t td;
    struct tm tv;
    int z;
    int get_action;
    get_action = get_client((void *)p_so->fd);
    if(get_action == USER_EXITED){
        FD_CLR(p_so->fd,&old_fds);
        list_del(&p_so->list);
        free(p_so);
    }
    return 0;
}



int create_connect(struct sockfd_opt *p_so){
    struct sockaddr_in cliaddr;
    int connfd;
    socklen_t sin_size;

    sin_size = sizeof(struct sockaddr_in);

    if((connfd = accept(p_so->fd,(struct sockaddr *)(&cliaddr),&sin_size)) == -1){
        fprintf(stderr,"Accept error: %s\a\n",strerror(errno));
        exit(0);
    }
    printf("------------进入create_connect\n");
    fprintf(stdout, "Server got connection from %s : %d\n",inet_ntoa(cliaddr.sin_addr),ntohs(cliaddr.sin_port));
    printf("进入create_connect\n");
    if(num++==FD_SETSIZE){
        fprintf(stderr,"too many clients!\n");
        close(connfd);
        return -1;
    }

    if((p_so=(P_SKOPT)malloc(sizeof(SKOPT)))==NULL)
    {
        perror("malloc");
        return -1;
    }

    p_so->fd=connfd;

    p_so->do_task=send_reply;

    list_add_tail(&p_so->list,&fd_queue);


    FD_SET(connfd,&old_fds);
    if(connfd>max_fd)
        max_fd=connfd;

    if( pthread_create(&tid, NULL, &get_client, (void *)connfd) !=0)
    {
        printf ("\n创建会话线程失败.\n");
    }
    else
    {
        printf ("\n创建会话线程成功.ID:%u\n",pthread_self());
    }
    return 0;

}

int init(int listenfd){
    P_SKOPT p_so;

    num = 0;

    if((p_so = (P_SKOPT)malloc (sizeof(SKOPT))) == NULL)
    {
        perror("malloc");
        return -1;
    }

    assert(list_empty(&fd_queue));
    p_so->do_task = create_connect;
    p_so->fd = listenfd;
    list_add_tail(&p_so->list,&fd_queue);
    num++;

    FD_ZERO(&old_fds);
    FD_SET(listenfd,&old_fds);

    return 0;
}


void read_register_file(){
    int fd; 
    int ret;
    printf ("\n信息初始化...\n");
    reglist *temp = (reglist *)malloc (sizeof (reglist));
    //reglist temp_NULL;
    //打开注册文件
    if((fd = open ("./reglist.c", O_RDONLY|0666 )) < 0)
    {
        printf ("打开文件失败(1)\n");
    }
    printf("%d\n", fd);
    //将注册文件内的用户信息读入在线用户链表
    printf ("\n加载注册用户信息...\n");


    while (((read (fd, temp->user.user_id , sizeof (temp->user.user_id)) )
        &&(read (fd, temp->user.user_name , sizeof (temp->user.user_name)))
        && (read (fd, temp -> password , sizeof (temp -> password))))> 0)
    {

        temp -> next = head_reg;
        head_reg = temp;

        temp = (reglist *)malloc (sizeof (reglist));
    }
    if(0 == ret)
    {
        printf("ret = 0\n");
    }
    if (ret < 0)
    {
        perror ("read fial!\n");
    }
    close (fd);
    printf ("\n信息初始化完毕.\n");
    
    print_reg_iteror(head_reg);
}


void update_process(int percent,int barlen){
int i;
putchar('[');
for(i=1;i<=barlen;++i)
putchar(i*100<=percent*barlen?'>':' ');
putchar(']');
printf("%3d%%",percent);
for(i=0;i!=barlen+6;++i)
putchar('\b');
}





void pool_init (int max_thread_num) 
{ 
    pool = (CThread_pool *) malloc (sizeof (CThread_pool)); 
    pthread_mutex_init (&(pool->queue_lock), NULL); 
    pthread_cond_init (&(pool->queue_ready), NULL); 
    pool->queue_head = NULL; 
    pool->max_thread_num = max_thread_num; 
    pool->cur_queue_size = 0; 
    pool->shutdown = 0;                    //0代表不销毁
    pool->threadid = (pthread_t *) malloc (max_thread_num * sizeof (pthread_t)); 
    int i = 0; 
    for (i = 0; i < max_thread_num; i++) 
    {  
        pthread_create (&(pool->threadid[i]), NULL, thread_routine,NULL); 
    } 
} 


/*************************************************************************
      函数名：pool_add_worker
    功能描述：添加任务 
        参数：void *(*process)(void *args), void *args 
        作者：王黎明
        时间：2016.07.03
      返回值：若成功返回0  
***************************************************************************/ 
int pool_add_worker (void *(*process) (void *arg), void *arg) 
{   
    CThread_worker *newworker = (CThread_worker *) malloc (sizeof (CThread_worker)); 
    newworker->process = process; 
    newworker->arg = arg; 
    newworker->next = NULL; 
    /*将任务加入到等待队列中*/  
    pthread_mutex_lock (&(pool->queue_lock)); 
     
    CThread_worker *member = pool->queue_head; 
    if (member != NULL) 
    { 
        while (member->next != NULL) 
            member = member->next; 
        member->next = newworker; 
    } 
    else 
    { 
        pool->queue_head = newworker; 
    } 
 
    assert (pool->queue_head != NULL); 
 
    pool->cur_queue_size++; 
    pthread_mutex_unlock (&(pool->queue_lock)); 
    /*好了，等待队列中有任务了，唤醒一个等待线程；*/  
    pthread_cond_signal (&(pool->queue_ready)); 
    return 0; 
} 


/*************************************************************************
      函数名：pool_destroy
    功能描述：销毁线程池
        参数：无 
        作者：王黎明
        时间：2016.07.03
      返回值：若成功返回0  
***************************************************************************/ 
int pool_destroy () 
{ 
    int i; 
    if (pool->shutdown) 
        return -1; 
    pool->shutdown = 1; 
         /*唤醒所有等待线程，线程池要销毁了*/
    pthread_cond_broadcast (&(pool->queue_ready)); 
     
    for (i = 0; i < pool->max_thread_num; i++) 
    {
        pthread_join (pool->threadid[i], NULL); 
    }
    /*销毁各种变量*/
    free (pool->threadid); 
     
    CThread_worker *head = NULL; 
    while (pool->queue_head != NULL) 
    { 
        head = pool->queue_head; 
        pool->queue_head = pool->queue_head->next; 
        free (head); 
    } 
     
    pthread_mutex_destroy(&(pool->queue_lock)); 
    pthread_cond_destroy(&(pool->queue_ready)); 
     
    free (pool); 
     
    pool=NULL; 
    return 0; 
} 
 
/*************************************************************************
      函数名：thread_routine
    功能描述：线程执行函数
        参数：void *arg
        作者：王黎明
        时间：2016.07.03
      返回值：若成功返回0  
***************************************************************************/ 
void* thread_routine (void *arg) 
{ 
    printf ("starting thread 0x%x\n", pthread_self ()); 
    while (1) 
    { 
        /*如果等待队列为0并且不销毁线程池，则处于阻塞状态; 注意 
         pthread_cond_wait是一个原子操作，等待前会解锁，唤醒后会加锁*/
        pthread_mutex_lock (&(pool->queue_lock)); 
         
        while (pool->cur_queue_size == 0 && !pool->shutdown) 
        { 
            printf ("thread 0x%u is waiting\n", pthread_self ()); 
            pthread_cond_wait (&(pool->queue_ready), &(pool->queue_lock)); 
        } 
 
        /*线程池要销毁了*/  
        if (pool->shutdown) 
        { 
             
            pthread_mutex_unlock (&(pool->queue_lock)); 
            printf ("thread 0x%x will exit\n", pthread_self ()); 
            pthread_exit (NULL); 
        } 
 
        printf ("thread 0x%x is starting to work\n", pthread_self ()); 
 
         
        assert (pool->cur_queue_size != 0); 
        assert (pool->queue_head != NULL); 
         
         
        pool->cur_queue_size--; 
        CThread_worker *worker = pool->queue_head; 
        pool->queue_head = worker->next; 
        pthread_mutex_unlock (&(pool->queue_lock)); 
 
          /*调用回调函数，执行任务*/  
        (*(worker->process)) (worker->arg); 
        free (worker); 
        worker = NULL; 
    } 
     
    pthread_exit (NULL); 
} 


void *  myprocess (void *arg) 
{ 
    printf ("threadid is 0x%x, working on task %d\n", pthread_self (),*(int *) arg); 
    sleep (1); 
    return NULL; 
} 

// 当前系统时间打印函数
void print_time( void )
{
    char      *wday[] = { "星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六" };
    time_t    timep;
    struct    tm *p;
    
    time( &timep );
    p = localtime( &timep );  //取得当地时间
    printf( "%02d/%02d/%02d  ", ( 1900 + p->tm_year ), ( 1 + p->tm_mon ), ( p->tm_mday ) );
    printf( "%s  %02d:%02d:%02d", wday[p->tm_wday], p->tm_hour, p->tm_min, p->tm_sec );
}
