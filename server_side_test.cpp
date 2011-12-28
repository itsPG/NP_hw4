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
using namespace std;

class PG_TCP_server
{
public:
	int c_fd, l_fd, pid;
	string my_ip;
	int my_port;
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
	void go(int aim_port)
	{

		struct sockaddr_in sin;
		struct sockaddr_in ccin;
		
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
			r = bind(l_fd, (struct sockaddr *)&sin, sizeof(sin));
			cout << "bind: " << sin.sin_port << endl;
			if(r == 0)break;
			usleep(500000);
		}
		r = listen(l_fd, 10); 
		cout << "listen: " << r << endl;
		printf("waiting ...\n");
		while(1)
		{
			usleep(500000);
			c_fd = accept(l_fd, (struct sockaddr *) &ccin, &len); 
			my_port = ntohs(ccin.sin_port);
			char addr_p[INET_ADDRSTRLEN];
			my_ip = inet_ntop(AF_INET, &ccin.sin_addr, addr_p, sizeof(addr_p));
			cout << "accept: " << c_fd << endl;
			cout << "IP: " << my_ip << endl;
			cout << "port: " << ntohs(ccin.sin_port) << endl;
			/*
			if (pid = harmonics())
			{
				cout << "parent" << endl;
				close(c_fd);
			}
			else
			{
				close(l_fd);
				dup2(c_fd, 0);
				dup2(c_fd, 1);
				dup2(c_fd, 2);
				close(c_fd);
				return ;
			}
			*/

			close(l_fd);
			int flags = fcntl(c_fd, F_GETFL, 0);
			fcntl(c_fd, F_SETFL, flags | O_NONBLOCK);
			while (1)
			{
				string t;
				char buf[10000] = {0};
				read(c_fd, buf, sizeof(buf));
				cout << "read: " << buf << endl;
				getline(cin,t);
				int w = write(c_fd, t.c_str(), t.size());
				cout << "write " << w << endl;
			}
		}
	}
	void go()
	{
		go(7000);
	}

};
int main()
{
	PG_TCP_server Rixia;
	Rixia.go(8001);

}
