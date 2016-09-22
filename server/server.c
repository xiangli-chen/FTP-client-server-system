/******************************************************************************* 
name:xiangli chen 
UIN:668102693
**********************************************************************************************/
#include <sys/types.h>//server
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <pthread.h>
#define BUF_SIZE 4096                   /* block transfer size */
#define QUEUE_SIZE 10
#define LEN sizeof(struct FNode)

struct FNode* head;//record files' information	

typedef struct FNode{
	char fileName[50];
	char owner[50];
	char uploadTime[50];
	int  dTimes;
	int  fileSize;
	struct FNode* next;
}FNode;
typedef struct Infor{
	int sa;
	char owner[40];
}Infor;
void CreatFileLink(FNode** head){
	(*head)=(FNode*)malloc(LEN);
	(*head)->next=NULL;
}
void AddNode(FNode* head,FNode* p){
	p->next=head->next;
	head->next=p;
}
FNode* FindNode(FNode* head,char name[20]){
    FNode*p;
	for(p=head->next;p!=NULL;p=p->next){
		if(strcmp(p->fileName,name)==0){
			return p;
		}
	}
	return NULL;
}
itoa(int i, char* string)//transfer int i to char* string
{
    int power, j ;   
    j=i;  
    for(power=1; j >= 10; j/=10)   
        power*=10;  
    for(; power > 0 ; power/=10){
		*string++ = '0'+i/power;
		i%=power;
	}
    *string = '\0';
}


void* connect_client(Infor* inf){//to process interation with client
	
	int sa=0,i=0,j=0,fd=0,bytes=0,f_size=0;
	char buf[BUF_SIZE];
	char a[50]="./s_file/",owner[50]={'\0'};
	char l_filename[50]={'\0'},r_filename[50]={'\0'};
	char t[50];
	struct FNode* p;
	time_t now;
	strcpy(owner,inf->owner);
	sa=inf->sa;	
	
				while(1){
					puts("wait for instruction:");
					bzero(buf,BUF_SIZE);
					if(recv(sa, buf, BUF_SIZE,0)==-1){/* read instruction from socket */					
						close(sa);
						break;   
					}
				
					puts(buf);
					if(strncmp(buf,"put ",4)==0 && strlen(buf)>4){//upload file
						for(j=0,i=4;buf[i]!='\0' && buf[i]!=' ';j++,i++){
							l_filename[j]=buf[i];
						}
						l_filename[j]='\0';
						if(buf[i]==' '){//get l_filename r_filename
							for(j=0,i=i+1;buf[i]!='\0';i++,j++){
								r_filename[j]=buf[i];               
							}
							r_filename[j]='\0';
						}
						else if(buf[i]=='\0'){//get l_filename
							strcpy(r_filename,l_filename);
                            puts(r_filename);
						}
						else{
							write(sa,"illegle input!",15);
							continue;
						}
						strcpy(a,"./s_file/");
						strcat(a,r_filename);						
						fd=open(a,O_RDWR|O_CREAT|O_EXCL,S_IRWXU|S_IRWXG|S_IRWXO);
						if(fd==-1){//if r_filename exists
							write(sa,"not:conflict file name",23);//conflict filename
							continue;
						}
						else{
							write(sa,"successful",11);
							while(1){
								bytes=read(sa,buf,BUF_SIZE);
								f_size=f_size+bytes;
								if(buf[strlen(buf)-2]=='@' && buf[strlen(buf)-1]=='#'){
									write(fd,buf,bytes-3);
									break;	/* check for end of file */	
								}//if
								write(fd,buf,bytes);
							}
							close(fd);
							bzero(buf,BUF_SIZE);
							f_size=f_size-2*sizeof(char)-1;                         
							printf("file:%s has been uploaded\n",r_filename);
							p=(FNode*)malloc(LEN);//new file information need to record
							p->dTimes=0;
							p->fileSize=f_size;
							f_size=0;
							strcpy(p->fileName,r_filename);        
							strcpy(p->owner,owner);
							time(&now);
							strcpy(p->uploadTime,ctime(&now));
							AddNode(head,p);
							
						} 
					}//end upload file
					
					else if(strncmp(buf,"lsFile",6)==0)//lsFile
					{
						if(buf[6]=='\0'){//lsFile
							bzero(buf,BUF_SIZE);
							for(p=head->next;p!=NULL;p=p->next){
								strcat(strcat(buf,p->fileName),"  ");
								strcat(strcat(buf,p->owner),"  ");
								strcat(strcat(buf,p->uploadTime),"  ");
								bzero(t,50);
								itoa(p->dTimes,t);
								strcat(strcat(buf,t),"\n");												
							}
						}
						else if(buf[6]==' ' && buf[7]!='\0'){//lsFile filename
							bzero(t,50);
							for(i=0;buf[i+7]!='\0';i++){
								t[i]=buf[i+7];
							}
							t[i]='\0';
							p=FindNode(head,t);
							bzero(buf,BUF_SIZE);
							if(p==NULL){
								strcpy(buf," non-existence file!\n");
							}
							else{
								strcat(strcat(buf,p->fileName),"  ");
								strcat(strcat(buf,p->owner),"  ");
								strcat(strcat(buf,p->uploadTime),"  ");
								bzero(t,50);
								itoa(p->dTimes,t);
								strcat(strcat(buf,t),"\n");
							}
							
						}
						else{
							bzero(buf,BUF_SIZE);
							strcpy(buf,"illegal lsFile");
						}
						send(sa, buf, strlen(buf)+1,0);					
					}//end lsFile
					else if(strcmp(buf,"exit")==0){//client exit
						printf("The client %s requries to stop session\n",inf->owner);
						close(sa);
						puts("session has been stopped\n");
						break;
					}//end client exit
					else {//send file
						printf("received file name:%s\n",buf);
						strcpy(a,"./s_file/");
						strcat(a,buf);
						fd = open(a,O_RDONLY);/* open the file to be sent back */
						if (fd < 0) {
							puts("open failed");  
							write(sa,"non",3);
							bzero(buf,BUF_SIZE);
							continue;				
						}
                        p=FindNode(head,buf);						
                        write(sa,p->owner,strlen(p->owner)+1);//sent the file's owner
                        read(sa,buf,BUF_SIZE);
						if(strcmp(buf,"failed")==0){
							continue;
						}						
						bzero(buf,BUF_SIZE);
						while (1) {
							bytes = read(fd, buf, BUF_SIZE); /* read from file */
							if (bytes <= 0) {close(fd);break;}/* check for end of file */
							write(sa, buf, bytes);           /* write bytes to socket */
						}
						write(sa,"@#",3);
						bzero(buf,BUF_SIZE);
						p->dTimes=p->dTimes+1;//dowload success
					}//end send file
				}				
	}
	
	void* server_exit(void* arg){//get exit 
		char buf[BUF_SIZE];
		char t[50];
		char a[40];
		FNode* p;
		gets(a);//wait for exit instruction
		if(strcmp(a,"exit")==0){
			bzero(buf,BUF_SIZE);
			for(p=head->next;p!=NULL;p=p->next){
				strcat(strcat(buf,p->fileName),"  ");
				strcat(strcat(buf,p->owner),"  ");
				strcat(strcat(buf,p->uploadTime),"  ");
				bzero(t,50);
				itoa(p->fileSize,t);
				strcat(strcat(buf,t),"  ");
				bzero(t,50);
				itoa(p->dTimes,t);
				strcat(strcat(buf,t),"\n");												
			}
			puts(buf);
			exit(0);
		}
	}
	
	
	int main(int argc, char *argv[])
	{
		int s, b, l, sa,on=1;
		struct sockaddr_in channel;           /* hold's IP address */
		struct sockaddr_in client;
		int SERVER_PORT=atoi(argv[1]);
		int sin_size=sizeof(struct sockaddr_in);
		pthread_t c_c,s_e;
		Infor* inf;//relative information should pass to thread
		
		/* Build address structure to bind to socket. */
		memset(&channel, 0, sizeof(channel)); /* zero channel */
		channel.sin_family = AF_INET;
		channel.sin_addr.s_addr = htonl(INADDR_ANY);
		channel.sin_port = htons(SERVER_PORT);
		
		
		if(mkdir("s_file",0777)==-1){//create new directory
			puts("create a new directory failed!\n");
			exit(0);
		}
		
		CreatFileLink(&head);//create linklist		
		printf("new directory:s_file has been created!\n");
		/* Passive open. Wait for connection. */
		s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); /* create socket */
		if (s < 0) fatal("socket failed");
		setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on));
		b = bind(s, (struct sockaddr *) &channel, sizeof(channel));
		if (b < 0) fatal("bind failed");
		//
		l = listen(s, QUEUE_SIZE);            /* specify queue size */
		if (l < 0) fatal("listen failed");
		/* Socket is now set up and bound. Wait for connection and process it. */
		
		pthread_create(&s_e, NULL, (void*)server_exit, NULL);//thread in order to get exit instruction
		
		while(1){
			sa = accept(s,(void *)&client, (int *)&sin_size);//block for connection			
			if (sa < 0) fatal("accept failed");		
			else {
				printf("The client's ip: %s\n",(char*)inet_ntoa(client.sin_addr));
				printf("The client's port numver: %s\n",argv[1]);
			}
			inf=(struct Infor*)malloc(sizeof(struct Infor));
			strcpy(inf->owner,(char*)inet_ntoa(client.sin_addr));
			strcpy(inf->owner,strcat(strcat(inf->owner," "),argv[1]));
			inf->sa=sa;
			
			pthread_create(&c_c, NULL, (void*)connect_client,inf);//thread to process task with connected client
			
		}//endwhile
	}//end main
	
	fatal(char *string)
	{
		printf("%s\n", string);
		exit(1);
	}
