#include <stdio.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
//---------------------
#include <fcntl.h>
//---------------------
#include <sys/stat.h>
//---------------------
#include <sys/types.h>
#include <sys/wait.h>
//---------------------
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
//--------------------
#include <getopt.h>
//------html templates----------------------------
char getstring[] = "GET ";
char endstring[] = "HTTP";
char OK[] = "HTTP/1.1 200 OK\n";
char NOTFOUND[] = "HTTP/1.1 404 NOT FOUND\n";
char ACCEPTR[] = "Accept-Ranges: bytes\n";
char CONTENTL[] = "Content-Length: ";
char CONTENTT[] = "Content-Type: text/html\n\n";
char NOTFOUNDHTML[] = "<!DOCTYPE html PUBLIC\n\"-//W3C//DTD XHTML 1.0 Transitional//EN\"\n\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n<html>\n<head><title>404</title></head>\n<body>\n<h1> 404 </h1>\n<h2> file not found </h2>\n</body>\n</html>\n\n";
//------------------------------------------------

int set_nonblock(int fd)
{
    int flags;
#if defined(O_NONBLOCK)
    if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
        flags = 0;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
    flags = 1;
    return ioctl(fd, FIOBIO, &flags);
#endif
} 
//-----------------make daemon---------------------
int makedaemon(char *dir)
{
	//--------------------------------------------
	//--------------start daemon------------------
	umask(0);

	if (setsid() < 0){
	//	perror("setsid\n");
		exit(0);
	}
	if (chdir(dir/*"/home/marek/qt_projects/final/"*/) < 0){
		//perror("chdir\n");
		exit(0);
	}
	//close(STDIN_FILENO);
	//close(STDOUT_FILENO);
	//close(STDERR_FILENO);
	//------------daemon has started-------------

	//printf("makedaemon has done\n");

	return 0;
}
//-------------------------------------------------
char *getfilename(char* buf, char* filename)
{				
	char *fnbegin, *fnend;
	fnbegin = strstr(buf, getstring);
	fnbegin += 5;
	if ((fnend = strchr(buf, '?')) == NULL){
		fnend = strstr(buf, endstring);
		fnend--;
	}
	memset(filename, '\0', BUFSIZ);
	strncpy(filename, fnbegin, fnend-fnbegin);
	printf("FILENAME IS:%s\n", filename);
	
	//printf("getfilename has finished\n");
	
	return filename;
}
//-------------------------------------------------
char *makeanswer(int html, char* full_answer, int head)
{
	char buf_answ[2000];
	int bcount = 0;
	memset(buf_answ,'\0', 2000);
	bcount = read(html, buf_answ, sizeof(buf_answ));
								
	memset(full_answer, '\0', sizeof(BUFSIZ));

	switch (head){
		case 0:{
			sprintf(full_answer, "%s%s%s%d\n%s%s", OK, ACCEPTR, CONTENTL, bcount, CONTENTT, buf_answ);
			//printf("answer is %s\n", full_answer);
			break;
		}
		case 1:{
			bcount = strlen(NOTFOUNDHTML);
			sprintf(full_answer, "%s%s%s%d\n%s%s", NOTFOUND, ACCEPTR, CONTENTL, bcount, CONTENTT, NOTFOUNDHTML);//buf_answ);
			//printf("answer is %s\n", full_answer);
			break;
		}
	}

	//printf("makeanswer has finished\n");

	return full_answer;
}
//-------------------------------------------------
void daem(char *ip, int port, char *dir)
{
	pid_t pid;
	struct sockaddr_in local;
	fd_set read_set;
	
	pid = getpid();
	printf("daemon pid: %d\n", pid);

	printf("ip: %s\n", ip);
	printf("port: %d\n", port);
	printf("dir: %s\n", dir);

	signal(SIGHUP, SIG_IGN);
	
	if (makedaemon(dir) != 0){
		exit(0);
	}

	int ss = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ss == -1){
		//perror("socket\n");
		exit(0);
	}

/*	if (inet_aton(ip, &local.sin_addr) == 0){
		perror("inet_aton\n");
		exit(0);
	}*/

	local.sin_addr.s_addr = htonl(INADDR_ANY);
	local.sin_port = htons(port);
	local.sin_family = AF_INET;

	if (bind(ss, (struct sockaddr*)&local, sizeof(local)) == -1){
		//perror("bind\n");
		exit(0);
	}

//	set_nonblock(ss);
	
	if (listen(ss, 5) == -1){
		//perror("listen\n");
		exit(0);
	}
while(1){
	int cs = accept(ss, NULL, NULL);
	char buf[255];
	char full_answer[BUFSIZ];
	int html;
	char filename[255];

	while(1){
		FD_ZERO(&read_set);
		FD_SET(cs, &read_set);
		int rslt = select(FD_SETSIZE, &read_set, NULL, NULL, NULL);

		if (rslt){
			//printf("rslt is not zero\n");
			if (FD_ISSET(cs, &read_set)){
				read(cs, buf, BUFSIZ);
				printf("\nquestion is:\n%s\n", buf);
				
				if (strlen(buf) < 5){
					//memset(buf, '\0', 255);
 					//continue;
					break;
				}
				getfilename(buf, filename);			
			
			//	if (filename == "exit")
			//		break;
	
				if((html = open(filename, O_RDONLY)) > 0){
					printf("file %s is find\n", filename);
					makeanswer(html, full_answer, 0);
					close(html);
					send(cs, full_answer, strlen(full_answer), 0);
				}
				else{
					printf("file %s is NOT find\n", filename);
					html = open("nfound.html", O_RDONLY);
					makeanswer(html, full_answer, 1);
					close(html);
					send(cs, full_answer, strlen(full_answer), 0);
				}
				//break;
			}
			memset(buf, '\0', 255);
		}
	}
	//sleep(13);
	//printf("daem is finishing!!!\n");
	close(cs);
	}
	close(ss);	
	return;
}

int main(int argc, char *argv[])
{
	pid_t pid;
	
	char ip[25] = "127.0.0.1";
	int port = 12347;
	char dir[100] = "~/home";

	extern char* optarg;
	extern int optind;	
	int opchar = 0;
	int opindex = 0;
	struct option opts[] = {
		{"addr", required_argument, 0, 'h'},
		{"port", required_argument, 0, 'p'},
		{"dir", required_argument, 0, 'd'},
		{0, 0, 0, 0}
	};

	while ((opchar = getopt_long(argc, argv, "hpd", opts, &opindex)) != -1){
		switch (opchar){
			case 'h':	
					sprintf(ip, "%s", argv[optind++]);
					printf("opt is h[ip]: %s\n", ip);
					break;	
			case 'p':	
					port = atoi(argv[optind++]);
					printf("opt is p[port]: %d\n", port);
					break;	
			case 'd':	
					sprintf(dir, "%s", argv[optind++]);
					printf("opt is d[dir]: %s\n", dir);
					break;
			case '?':	
					printf("opt is none\n");
					//exit(0);
					break;
		}
	}

	

	pid = fork();
	printf("!pid: %d\n", getpid());

	if (pid == -1)	{
		perror("fork\n");
		exit(0);
	}
	if (pid == 0){
		printf("-------------------\n");
		daem(ip, port, dir);
	}	
	else{
	//	waitpid(pid, &status, WUNTRACED);
	//	wait(&status);
	//	exit(EXIT_SUCCESS);
	}
	return 0;
}
