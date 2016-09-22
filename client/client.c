/* This page contains the client program. The following one contains the
* server program. Once the server has been compiled and started, clients
* anywhere on the Internet can send commands (file names) to the server.
* The server responds by opening and returning the entire file requested.
*/
/*********************************************************************************************
name:xiangli chen
UIN:668102693
**********************************************************************************************/
#include <sys/types.h>//client
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
//#define SERVER_PORT 12345             /* arbitrary, but client and server must agree */
#define BUF_SIZE 4096                   /* block transfer size */
#define LEN sizeof(struct FNode)
typedef struct FNode{//to record files' information
	char fileName[50];
	char owner[50];
	char udTime[50];
	char a;
	int  fileSize;
	struct FNode* next;
}FNode;
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
		}
		return p;
	}
	return NULL;
} 
itoa(int i, char* string)
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
int main(int argc, char **argv)
{
	int c, s, bytes,fp;
	char buf[BUF_SIZE];                   /* buffer for incoming file */ 
	char t[50];
	struct hostent *h;                    /* info about server */
	struct sockaddr_in channel;
	uint16_t SERVER_PORT=atoi(argv[2]);
	char a[50]={'\0'},l_filename[50]={'\0'};  
	int i=0,j=0,k=0,f_size=0;
    time_t now;
	struct FNode* head;
	struct FNode* p;
	
	if (argc != 3) fatal("Usage: client server-name file-name");
	h = gethostbyname(argv[1]);           /* look up host's IP address */
	if (!h) fatal("gethostbyname failed");
	
	s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s < 0) fatal("socket");  memset(&channel, 0, sizeof(channel));
	channel.sin_family= AF_INET;
	memcpy(&channel.sin_addr.s_addr, h->h_addr, h->h_length);
	channel.sin_port= htons(SERVER_PORT);
	c = connect(s, (struct sockaddr *) &channel, sizeof(channel));
	if (c < 0) fatal("connect failed");	
    CreatFileLink(&head);//create linklist
	
	while(1){		
		puts("please input ");
		gets(a);//get instructions
		k=strlen(a);
		
		if(strncmp(a,"get ",4)==0 && k>4){//get file
			for(i=0;i<k-4;i++){
				a[i]=a[i+4];               
			}    
			a[i]='\0';			
			if(!(send(s, a, strlen(a)+1,0)<0)){
				printf("file name: %s has been sended\n",a); }
			else {
				puts("send failed");
				break;
			}           
			
			bytes=read(s, buf, BUF_SIZE);
			if(bytes<=0) break;
			if(strncmp(buf,"non",3)==0){//file non-exist
				puts("non-exit the file");
				bzero(buf,BUF_SIZE);
				continue;
			}
			fp=open(a,O_RDWR|O_CREAT);
			if(fp<0){
				puts("open file failed");
				write(s,"failed",7);
				continue;
			}
			else{				
				write(s,"ok",3);
				p=(FNode*)malloc(LEN);
				p->a='d';//this is dowloaded file in the client
				p->fileSize=0;
				time(&now);
				strcpy(p->udTime,ctime(&now));
				strcpy(p->owner,buf);
				strcpy(p->fileName,a);
				bzero(buf, BUF_SIZE);
				while(1){
					bytes=read(s, buf, BUF_SIZE);
					p->fileSize=p->fileSize+bytes;
					if(buf[bytes-1]==EOF){
						write(fp,buf,bytes-3);
						puts(buf);
						break;	/* check for end of file */	
					}
					if(buf[strlen(buf)-2]=='@' && buf[strlen(buf)-1]=='#'){
						write(fp,buf,bytes-3);
						break;	/* check for end of file */	
					}
					write(fp,buf,bytes);					
				}
				close(fp);
				
				p->fileSize=p->fileSize-2*sizeof(char)-1;
				AddNode(head,p);
				printf("file:%s has been downloaded\n",a);
				bzero(buf,BUF_SIZE);
				bzero(a,strlen(a)+1);
			}
		}//end get file
		else if(strncmp(a,"put ",4)==0 && k>4){//put file	
			for(j=0,i=4;a[i]!='\0'&& a[i]!=' ';i++,j++){//get local_filename
				l_filename[j]=a[i];
			}
			l_filename[j]='\0';
			if((fp=open(l_filename,O_RDONLY))==-1){
				puts("open failed!");
				continue;
			}
			if((send(s, a, strlen(a)+1,0)<0)){//
				puts("send failed");
				break;
			}
			bytes=read(s,buf,BUF_SIZE);
			if(strncmp(buf,"not:",4)==0){//conflict filename 
				puts(buf);
				close(fp);
				continue;
			}
			else if(strncmp(buf,"successful",10)==0){
				p=(FNode*)malloc(LEN);
				strcpy(p->fileName,l_filename);
				strcpy(p->owner,"               ");
				p->a='u';//this file will be uploaded
				p->fileSize=0;
				time(&now);
				strcpy(p->udTime,ctime(&now));
				while(1){
					bytes=read(fp,buf,BUF_SIZE);
					p->fileSize=p->fileSize+bytes;
					if(bytes<=0){close(fp);break;}
					write(s,buf,bytes);
				}
				write(s,"@#",3);
				bzero(buf,BUF_SIZE);
				AddNode(head,p);
				puts("upload is successful!");
			}
			else{
				puts(buf);//illegle input
				close(fp);
				continue;
			}
			bzero(buf,BUF_SIZE);
			bzero(a,strlen(a)+1);
		}//end put local-filename [remote-filename]
		else if(strncmp(a,"lsFile",6)==0){//lsFile	
			if(!(send(s,a,strlen(a)+1,0)<0)){
				printf("The struction %s has been sent\n",a);
				bzero(buf,BUF_SIZE);
				bytes=read(s, buf, BUF_SIZE);
				if(bytes<=0)break;		
				puts(buf);
			}
			bzero(buf,BUF_SIZE);
		}//end lsFile
		else if(strcmp(a,"exit")==0){//exit
			if(!(send(s,a,strlen(a)+1,0)<0)){
				printf("The struction exit has been sent\n");
				bzero(buf,BUF_SIZE);
				//
				for(p=head->next;p!=NULL;p=p->next){
					strcat(strcat(buf,p->fileName),"  ");
					strcat(strcat(buf,p->udTime),"  ");
					bzero(t,50);
					itoa(p->fileSize,t);
					strcat(strcat(buf,t),"  ");
					strcat(strcat(buf,p->owner),"  ");					
					if(p->a=='u'){
						strcat(strcat(buf,"Upload"),"\n");
					}
					else{
						strcat(strcat(buf,"download"),"\n");
					}
					
				}
				puts(buf);
				bzero(buf,BUF_SIZE);
				//
				close(s);
				exit(0);
			}
			else {puts("exit send failed");}
		}//end exit
		else{
			puts("illegle input!");
		}//illegle
		
	}//end while
}
fatal(char *string){
	printf("%s\n", string);
	exit(1);
}
