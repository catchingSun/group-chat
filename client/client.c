#include <stdlib.h>
#include "chat.h"

char cmd[80];
struct hostent *host;
int login_count;
CHATCMD chat_cmd[] = {
	{"sign","SIGN",NULL,do_sign},
	{"login","LOGIN",NULL,do_login},
	{"create","CREATE",NULL,do_create},
	{"join","JOIN","group_name",do_join},
	{"invite","INVITE",NULL,do_invite},
	{"delete","DELETE","group_name",do_delete},
	{"chat","CHAT",NULL,do_chat},
	{"group","GROUP",NULL,do_group}
};
int login_statues = -1;
char user_number[USER_NUMBER_LENGTH];
int qu;
char mode[10];
int main()
{
	int socket_fd;
	struct sockaddr_in server_addr;
	int is_sign = -1;
	int port = SERVER_PORT;
	int z;
	int cmd_found;
	fd_set rfds,orfds;
	int ret,maxfd = -1;
	char *ip = SERVER_IP;
	char sign[10];
	host = gethostbyname(ip);
	if((socket_fd = socket(PF_INET,SOCK_STREAM,0)) == -1){
		printf("create socket error");
	}
	
	memset(&server_addr,0,sizeof server_addr);
	server_addr.sin_family = PF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr = *((struct in_addr *)host->h_addr);

	if(connect(socket_fd,(struct sockaddr *)(&server_addr),sizeof server_addr) == -1){
		fprintf(stderr,"connect error:%s \n\a",strerror(errno));
		exit(1);
	}
	while(1){
		memset(sign,0,sizeof(sign));
		printf("Signed ? yes or no?");

		fgets(sign,sizeof(sign),stdin);
		if(sign[strlen(sign) - 1] == '\n'){
			sign[strlen(sign) - 1] = 0;
		}


		if( !strncmp(sign,"yes",3)){
			is_sign = SIGN_Y;
			break;
		}
		else if( !strncmp(sign,"no",2)){
			is_sign = SIGN_N;
			break;
		}
	}

	if(is_sign == SIGN_Y){
		printf("Please input your user name and pasword to login:\n");
		printf("-------------------------------------------------------------------------------\n");
		do_login(socket_fd,"LOGIN");
	}

	else if(is_sign == SIGN_N){
		printf("Please input your user name and pasword to register:\n");
		printf("-------------------------------------------------------------------------------\n");
		do_sign(socket_fd,"SIGN");
	}

	while(1){
		input_cmd(cmd,sizeof(cmd));
		
		cmd_found = check_chat_cmd(cmd);
		chat_cmd[cmd_found].handler(socket_fd,chat_cmd[cmd_found].name);
	}
	shutdown(socket_fd,1);
	close(socket_fd);
	return 0;
}
int check_chat_cmd(char *cmd){
	int i = 0;
	int j = 0;
	int found = -1;
	while(*(cmd + j) == 32){
		j++;
	}
	for(i = 0; i < CMD_NUM; i++ ){
		if(strcmp(cmd,chat_cmd[i].alias) == 0)
			found = i;
	}

	return found;
}


int input_cmd(char *cmd,int size){
	int len;
	memset(cmd,0,size);

	printf("---->");
	fgets(cmd,size,stdin);

	len = strlen(cmd);
	if(cmd[strlen(cmd) - 1 ] == '\n'){
		cmd[strlen(cmd) - 1 ] = 0;
	}
	return len;
}


void do_sign(int sockfd,char *name){
	char user_name[MAX_USER_NAME_SIZE];
	char user_password[MAX_USER_PASSWORD_SEZE];
	char user_password1[MAX_USER_PASSWORD_SEZE];
	char sign_buff[BUFSIZE];
	char read_message[MAX_BUF_SIZE];

	memset(sign_buff,0,sizeof(sign_buff));
	memset(read_message,0,sizeof(read_message));
	memset(user_password,0,sizeof(user_password));
	memset(user_password1,0,sizeof(user_password1));
	memset(user_name,0,sizeof(user_name));
	memset(user_number,0,sizeof(user_number));

	printf("user name:");

	fflush(stdin);
	fflush(stdout);
	fgets(user_name,sizeof(user_name),stdin);
	user_name[strlen(user_name) - 1 ] = 0;


	printf("password:");
	
	terminal_echo_off(STDIN_FILENO);
	fgets(user_password,sizeof(user_password),stdin);
	terminal_echo_on(STDOUT_FILENO);
	printf("\n");

	printf("please input password again:");
	
	terminal_echo_off(STDIN_FILENO);
	fgets(user_password1,sizeof(user_password1),stdin);
	terminal_echo_on(STDOUT_FILENO);
	printf("\n");
	if(user_password[strlen(user_password) - 1 ] == '\n'){
		user_password[strlen(user_password) - 1 ] = 0;
	}
	if(user_password1[strlen(user_password1) - 1 ] == '\n'){
		user_password1[strlen(user_password1) - 1 ] = 0;
	}
	

	while(strcmp(user_password,user_password1) != 0){
		
		printf("please input password again:");
		terminal_echo_off(STDIN_FILENO);
		fgets(user_password1,sizeof(user_password1),stdin);
		user_password1[strlen(user_password1) - 1 ] = 0;
		terminal_echo_on(STDIN_FILENO);
		printf("\n");
	}

	strncpy(sign_buff,name,strlen(name));


	strcat(sign_buff,DELIMITER); 
	strcat(sign_buff,user_name);
	strcat(sign_buff,DELIMITER); //添加分界符
	strcat(sign_buff,user_password);
	
	printf("%s\n",sign_buff);
	send_message(sockfd,sign_buff);
	
	read_message_status(sockfd,read_message);
	
	if(!strncmp(read_message,SIGN_NAME_EXISTED,strlen(SIGN_NAME_EXISTED))){
		printf("Sign error, name is exist.\n");
		do_sign(sockfd,"SIGN");
	}
	if(strlen(read_message) > 6){
		printf("服务器错误\n");
	}else{
		strncpy(user_number,read_message,strlen(read_message));
		printf("It is your BiuBiu number: %s\n",user_number);
	}
}
void do_login(int sockfd,char *name){
	char user_password[MAX_USER_PASSWORD_SEZE];
	char login_buff[CMD_MESSAGE_LENGTH];

	char read_message[BUFSIZE];
	memset(read_message,0,sizeof(read_message));
	memset(login_buff,0,sizeof(login_buff));
	memset(user_password,0,sizeof(user_password));
	login_count++;
	printf("user number:");
	fgets(user_number,sizeof(user_number),stdin);
	if(user_number[strlen(user_number) - 1] == '\n'){
		user_number[strlen(user_number) - 1] = 0;
	}

	printf("password:");
	
	terminal_echo_off(STDIN_FILENO);
	fgets(user_password,sizeof(user_password),stdin);
	terminal_echo_on(STDOUT_FILENO);
	printf("\n");
	if(user_password[strlen(user_password) - 1] == '\n'){
		user_password[strlen(user_password) - 1] = 0;
	}
	

	strncpy(read_message,name,strlen(name));
	strcat(read_message,DELIMITER);
	strcat(read_message,user_number);
	strcat(read_message,DELIMITER);
	strcat(read_message,user_password);
	send_message(sockfd,read_message);

	read_message_status(sockfd,read_message);
	fflush(stdout);
	if(strncmp(read_message,LOGIN_SUCCESS,strlen(LOGIN_SUCCESS)) == 0){
		printf("Login success.\n");
		login_statues = LOGIN_ON;
	}
	if(strncmp(read_message,LOGIN_FAILURE,strlen(LOGIN_FAILURE)) == 0){
		printf("Login failed , please login again.\n");
		login_statues = LOGIN_OFF;
		do_login(sockfd,name);
	}

	if(login_count >= 3){
		printf("Login failed three times ， you have exist BiuBiu.\n");
		close(sockfd);
		exit(1);
	}
}


void do_create(int sockfd, char *name){
	char cmd[CMD_MESSAGE_LENGTH];
	char read_message[MAX_BUF_SIZE];
	is_login(sockfd);
	strncpy(cmd,name,strlen(name));
	send_message(sockfd,cmd);

	read_message_status(sockfd,read_message);
}
void is_login(int sockfd){
	while(!login_statues){
		printf("请先登录:");
		do_login(sockfd,"LOGIN");
	}
}

void do_join(int sockfd,char *name){
	char cmd[CMD_MESSAGE_LENGTH];
	char group_number[GROUP_NUMBER_LENGTH];
	char read_message[MAX_BUF_SIZE];
	is_login(sockfd);
	strncpy(cmd,name,strlen(name));
	fgets(group_number,sizeof(group_number),stdin);
	strcat(cmd,group_number);
	send_message(sockfd,cmd);

	read_message_status(sockfd,read_message);
}

void do_invite(int sockfd,char *name){
	is_login(sockfd);
	char cmd[CMD_MESSAGE_LENGTH];
	char group_number[GROUP_NUMBER_LENGTH];
	char user_number1[GROUP_NUMBER_LENGTH];
	char read_message[MAX_BUF_SIZE];
	strncpy(cmd,name,strlen(name));
	fgets(user_number1,sizeof(user_number),stdin);
	fgets(group_number,sizeof(group_number),stdin);
	strcat(cmd,user_number);
	strcat(cmd,DELIMITER);
	strcat(cmd,group_number);
	strcat(cmd,DELIMITER);
	strcat(cmd,user_number1);

	send_message(sockfd,cmd);

	read_message_status(sockfd,read_message);


}


void do_delete(int sockfd,char *name){
	is_login(sockfd);
	char cmd[CMD_MESSAGE_LENGTH];
	char group_number[GROUP_NUMBER_LENGTH];
	char read_message[MAX_BUF_SIZE];
	printf("请输入要删除的群号码：");
	fgets(group_number,sizeof(group_number),stdin);

	strncpy(cmd,name,strlen(name));
	strcat(cmd,group_number);

	send_message(sockfd,cmd);

	read_message_status(sockfd,read_message);
}


void do_chat(int sockfd,char *name){
	char cmd[CMD_MESSAGE_LENGTH];

	char read_message[MAX_BUF_SIZE];

	char user_number1[USER_NUMBER_LENGTH];
	pthread_t ntid;
	Parameter arg;
	memset(mode,0,sizeof(mode));
	strcpy(mode,"CHAT");

	ChatMessage chat;
	is_login(sockfd);
	
	strncpy(cmd,name,strlen(name));
	send_message(sockfd,cmd);
	memset(read_message,0,sizeof(read_message));
	
	//获取在线用户列表
	get_online_list(sockfd,read_message);
	printf("Please input other BiuBiu number to chat.\n");
	
	fgets(user_number1,sizeof(user_number1),stdin);
	if(user_number1[strlen(user_number1) - 1] = '\n'){
		user_number1[strlen(user_number1) - 1] = 0;
	}
	printf("You can chat with %s now.Stop chat with %s, Please input 'quit'.\n",user_number1,user_number1);
	printf("-------------------------------------------------------------------------------\n");

	arg.sockfd = sockfd;
	arg.read_message = read_message;
	int err = pthread_create(&ntid,NULL,thread_read,(void *)(&arg));
	while(1){

		qu = set_message_to_send(sockfd,user_number,user_number1);
		if(qu)
		{
			send_message(sockfd,"QUIT");
			break;
		}
	}


}

void *thread_read(void *arg){
	Parameter *m;
	m = (Parameter *)arg;
	while(1){
		if(qu == 1){
			pthread_exit(NULL);
		}
		memset(m->read_message ,0,sizeof(m->read_message));
		get_chat_message(m->sockfd,m->read_message);
	}

}
int set_message_to_send(int sockfd,char *number1,char *number2){
	char chat_buff[MAX_BUF_SIZE];
	char chat_message[BUFSIZE];
//	char user_number1[USER_NUMBER_LENGTH];
	char time_buff[TIME_SIZE];
	char chat_message_length[10];

	memset(chat_buff,0,sizeof(chat_buff));
		
	memset(chat_message,0,sizeof(chat_message));
	memset(time_buff,0,sizeof(time_buff));

	get_local_time(time_buff);
	if(strcmp(mode,"CHAT") == 0){
		printf("Time: %s",time_buff);
		printf(" User: %s\n",number1);
	}else if(strcmp(mode,"GROUP") == 0){
		printf("Time: %s",time_buff);
		printf(" User: %s\n",number2);
	}
	

	fflush(stdin);
	fgets(chat_message,sizeof(chat_message),stdin);


	if(!strncasecmp(chat_message,"QUIT",4)){

		printf("Quited the chat.\n");
		return QUIT_CHAT;
	}

	strncpy(chat_buff,number1,strlen(number1));
	strcat(chat_buff,DELIMITER);
	strcat(chat_buff,number2);
	strcat(chat_buff,DELIMITER_MESSAGE);
	strcat(chat_buff,time_buff);
	strcat(chat_buff,DELIMITER);
	itoa(strlen(chat_message) - 1,chat_message_length,10);
	strcat(chat_buff,chat_message_length);
	strcat(chat_buff,DELIMITER);
	chat_message[strlen(chat_message) - 1] = 0;
	strcat(chat_buff,chat_message);
	
//	chat.cmd = name;
/*	if(chat){
		chat.user_id1 = number1;
		chat.user_id2 = number2;
		chat.message.time = time_buff;
		chat.message.length = chat_message_length;
		chat.message.message = chat_message;
	}
*/	
	
	send_message(sockfd,chat_buff);

	return 0;
}


void get_chat_message(int sockfd,char *read_message){
	char *user_number1;
	char *user_number2;
	char *user_number_str;
	char *user_message;
	char *time_buff;
	char *chat_message_length;
	char *message;
	char *delim1 = "^^";
	char *delim2 = "%%";
	char chat_mode[10];
	memset(chat_mode,0,sizeof(chat_mode));
	
	read_chat_message(sockfd,read_message);
	if(strlen(read_message) > 0){
		user_number_str = strtok(read_message,delim1);
		user_message = strtok(NULL,delim1);
		user_number1 = strtok(user_number_str,delim2);
		user_number2 = strtok(NULL,delim2);

		time_buff = strtok(user_message,delim2);
		chat_message_length = strtok(NULL,delim2);
		message = strtok(NULL,delim2);

		if(strcmp(mode,"CHAT") == 0){
			printf("%s\n",read_message);
			if(strncmp(user_number,user_number2,strlen(user_number2)) == 0){
				printf("\t\t\t\t\tTime: %s",time_buff);
				printf(" User: %s\n",user_number1);
				printf("\t\t\t\t\t\t%s\n",message);
			}
		}else if(strcmp(mode,"GROUP") == 0){
			if(strncmp(user_number,user_number2,strlen(user_number2)) != 0){
				printf("\t\t\t\t\tTime: %s",time_buff);
				printf(" User: %s\n",user_number2);
				printf("\t\t\t\t\t\t%s\n",message);
			}
			
		}

	}
}

void get_online_list(int sockfd,char *read_message){
	char *unline_user_number;
	char *unline_user_name;
	char *delim = DELIMITER;
	memset(read_message,0,sizeof(read_message));
	printf("----------------------------------Online user----------------------------------\n");

	read_message_status(sockfd,read_message);
	
	while( strncmp(read_message,ONLINE_LIST_END, strlen(ONLINE_LIST_END) != 0) ){
		unline_user_number = strtok(read_message,delim);
		unline_user_name = strtok(NULL,delim);
		printf("    User number : %s     User name : %s\n",unline_user_number,unline_user_name);
		read_message_status(sockfd,read_message);
	}
		
	printf("-------------------------------------------------------------------------------\n");
}

void do_group(int sockfd,char *name){

	char cmd[CMD_MESSAGE_LENGTH];
	char chat_buff[MAX_BUF_SIZE];
	char read_message[MAX_BUF_SIZE];
	char chat_message[MAX_BUF_SIZE];
	char group_number[GROUP_NUMBER_LENGTH];
	char time_buff[TIME_SIZE];
	char chat_message_length[10];
	pthread_t ntid;
	GroupMessage group;
	Parameter arg;

	memset(mode,0,sizeof(mode));
	strcpy(mode,"GROUP");

	is_login(sockfd);
	strncpy(cmd,name,strlen(name));
	send_message(sockfd,cmd);
	printf("Please input group number to chat.\n");
	fflush(stdin);
	fgets(group_number,sizeof(group_number),stdin);
	if(group_number[strlen(group_number) - 1] == '\n'){
		group_number[strlen(group_number) - 1] == 0;
	}
	printf("--------------------------GROUP ID:%s-----------------------------\n",group_number);
	arg.sockfd = sockfd;
	arg.read_message = read_message;
	int err = pthread_create(&ntid,NULL,thread_read,(void *)(&arg));
	while(1){
		fflush(stdin);

		qu = set_message_to_send(sockfd ,group_number ,user_number);
		if(qu){
		//	send_message(sockfd,"QUIT");
			break;
		}
	}
}
void get_local_time(char *time_buff){
	
	time_t td;
	struct tm tm;
	time(&td);
	char local_time[10]; 
	tm = *localtime(&td);

	memset(local_time,0,sizeof(local_time));
	itoa(1900 + tm.tm_year,local_time,10);
//	

	strncpy(time_buff,local_time,strlen(local_time));
	itoa(1 + tm.tm_mon ,local_time,10);
	strcat(time_buff,"-");
	if(strlen(local_time) < 2){
		strcat(time_buff,"0");
	}
	strcat(time_buff,local_time);



	itoa(tm.tm_mday,local_time,10);
	strcat(time_buff,"-");
	if(strlen(local_time) < 2){
		strcat(time_buff,"0");
	}
	
	strcat(time_buff,local_time);
	strcat(time_buff," ");



	itoa(tm.tm_hour,local_time,10);
	
	if(strlen(local_time) < 2){
		strcat(time_buff,"0");
	}

	strcat(time_buff,local_time);
	strcat(time_buff,":");



	itoa(tm.tm_min,local_time,10);
	if(strlen(local_time) < 2){
		strcat(time_buff,"0");
	}
	strcat(time_buff,local_time);
	strcat(time_buff,":");



	itoa(tm.tm_sec,local_time,10);
	if(strlen(local_time) < 2){
		strcat(time_buff,"0");
	}
	strcat(time_buff,local_time);


}


int send_message(int sockfd,char *sign_message){
	char send_buff[MAX_BUF_SIZE];
	int z;

	memset(send_buff,0,sizeof send_buff);
	strncpy(send_buff,sign_message,strlen(sign_message));
	
	z = write(sockfd,send_buff,strlen(send_buff));


	if(z == -1){

		exit(1);
	}
	else if(z == 0){

	}
	else{		

	}
	return 1;
}


void itoa(int num,char *str,int radix)
{/*索引表*/
	char index[]="0123456789ABCDEF";
	unsigned unum;/*中间变量*/
	int i=0,j,k;
/*确定unum的值*/
	if(radix==10&&num<0)/*十进制负数*/
	{
		unum=(unsigned)-num;
		str[i++]='-';
	}
	else unum=(unsigned)num;/*其他情况*/
/*转换*/
	do{
		str[i++]=index[unum%(unsigned)radix];
		unum/=radix;
	}while(unum);
	str[i]='\0';
	/*逆序*/
	if(str[0]=='-')
		k=1;/*十进制负数*/
	else k=0;
		char temp;
	for(j=k;j<=(i-1)/2;j++)
	{
		temp=str[j];
		str[j]=str[i-1+k-j];
		str[i-1+k-j]=temp;
	}
}

void read_message_status(int sockfd,char *read_message){
	char read_buf[MAX_BUF_SIZE];
	int z;
	
	memset(read_buf,0,sizeof(read_buf));
	z = read(sockfd,read_buf,sizeof(read_buf));
	if(z < 0 && errno == EINTR){
		read_message_status(sockfd,read_message);
	}
	if(z > 0){
		strncpy(read_message,read_buf,strlen(read_buf));
	}

}

void read_chat_message(int sockfd,char *read_message){
	char read_buf[MAX_BUF_SIZE];
	int z;
	/*int flags;
	if((flags = fcntl(sockfd,F_GETFL,0) ) < 0){
		bail("f_cntl(F_GETFL)");
	}
	flags |= O_NONBLOCK;
	//在聊天时将fd设置为非阻塞模式
	if(fcntl(sockfd,F_SETFL,flags) < 0){
		bail("f_cntl(F_GETFL)");
	}*/
	int flags = fcntl(sockfd,F_GETFL,0);
	fcntl(sockfd,F_SETFL,flags | O_NONBLOCK);
	memset(read_buf,0,sizeof(read_buf));
	z = read(sockfd,read_buf,sizeof(read_buf));
	if(z < 0 && errno == EINTR){
		memset(read_buf,0,sizeof(read_buf));
		read_chat_message(sockfd,read_message);
	}
	if(z > 0){
		strncpy(read_message,read_buf,strlen(read_buf));
	}
}