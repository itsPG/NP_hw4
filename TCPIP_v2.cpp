#include <iostream>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#define BUFFER_DEBUG 1
#define PG_NON_BLOCKING 0
using namespace std;

int PG_get(int fd, string &q, int len)
{
	q = "";
	char c;
	for (int i = 1;len == -1 || i <= len; i++)
	{
		int s = read(fd, &c, 1);
		//cout << "read " << (unsigned int)c << endl;
		if (s == -1)
		{
			perror("read failed ");
			return -1;
		}
		if (s == 0)
		{
			return q.size();
		}
		q += c;
		if (len == -1 && c == '\0')
			return q.size();
	}
	return q.size();
}
void PG_put(int fd, string &q)
{
	
	for (int i = 0; i < q.size(); i++)
	{
		int s = write(fd, q.c_str() + i, 1);
		if (s <= 0)
		{
			perror("write failed");
			return ;
		}
	}
}
class PG_TCP_client
{
public:
	int fd;
	struct hostent *he;
	struct sockaddr_in sin;
	string host_name;
	int port;
	void init(string q1, int q2)
	{
		host_name = q1;
		port = q2;
		he = gethostbyname(host_name.c_str());
		if (he == NULL)
		{
			cerr << "gethostbyname error" << endl;
			exit(1);
		}
		fd = socket(AF_INET, SOCK_STREAM, 0);
		bzero(&sin, sizeof(sin));
		sin.sin_family = AF_INET;
		sin.sin_addr = *((struct in_addr *)he->h_addr); 
		sin.sin_port = htons(port);
		conekuto();
	}
	void conekuto()
	{
		if (connect(fd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		{
			cout << "host:" << host_name << " port:" << port << endl;
			perror("conekuto failed");
			exit(1);
		}
	}
	
};
class PG_TCP_server
{
public:
	int c_fd, l_fd, pid;
	string my_ip;
	int my_port;
	bool stay_flag;
	int harmonics()
	{
		int pid, stat;
		if (pid = fork())
		{
			waitpid(pid, &stat, 0);
			return pid;
		}
		else
		{
			if (fork())
			{
				exit(0);
			}
			else return 0;
		}
	}
	int init_and_bind(int aim_port, int stay_flag_in)
	{
		struct sockaddr_in sin;
		struct sockaddr_in cin;
		stay_flag = stay_flag_in;
		socklen_t len;
		char buf[10000]; 
		char addr_p[INET_ADDRSTRLEN]; 
		int port = aim_port;
		int n,r; 
       
		bzero(&sin, sizeof(sin)); 
		sin.sin_family = AF_INET; 
		sin.sin_addr.s_addr = INADDR_ANY; 
		sin.sin_port = htons(port);
    
		l_fd = socket(AF_INET, SOCK_STREAM, 0); 
		while(1)
		{
			
			int enable = 1;
			setsockopt(l_fd, SOL_SOCKET, SO_REUSEADDR,  &enable, sizeof(int));
			
			r = bind(l_fd, (struct sockaddr *)&sin, sizeof(sin));
			cout << getpid() << " bind: " << port << endl;
			if(r == 0) return 0;
			else return -1;
		}
	}
	void go()
	{
		struct sockaddr_in sin;
		struct sockaddr_in cin;
		socklen_t len;
		
		listen(l_fd, 10); 
		cout << "listen: " << endl;
		printf("waiting ...\n");
		while(1)
		{
			if (stay_flag) cout << "before accept" << endl;
			c_fd = accept(l_fd, (struct sockaddr *) &cin, &len); 
			if (stay_flag) cout << "after accept" << endl;
			if (c_fd == -1)
			{
				perror("accept error");
				exit(1);
			}
			
			my_port = ntohs(cin.sin_port);
			char addr_p[INET_ADDRSTRLEN];
			my_ip = inet_ntop(AF_INET, &cin.sin_addr, addr_p, sizeof(addr_p));
			//cout << "accept: " << c_fd << endl;
			//cout << "IP: " << my_ip << endl;
			//cout << "port: " << ntohs(cin.sin_port) << endl;
			cout << "accept: " << my_ip << " / " << my_port << endl;
			//return;
			if (stay_flag)
			{
				cout << "return !! " << endl;
				//close(l_fd);
				return;
			}
			if (pid = harmonics())
			{
				//cout << "parent" << endl;
				close(c_fd);
				//close(global_l_fd);

			}
			else
			{
				close(l_fd);
				//dup2(c_fd, 0);
				//dup2(c_fd, 1);
				//dup2(c_fd, 2);
				//close(c_fd);
				return ;
			}
		}
	}
};


