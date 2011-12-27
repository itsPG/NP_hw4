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
using namespace std;
const int PG_NEW = 0, PG_CONNECTING = 1, PG_OK = 2, PG_CLOSE = 3;
class PG_buffer
{
public:
	string B;
	char c;
	int in_fd, out_fd;
	PG_buffer()
	{
		B = "";
		in_fd = -1;
		out_fd = -1;
	}
	int read_buf()
	{
		int r = 0, t;
		while (1)
		{
			t = read(in_fd, &c, 1);
			if (t == -1)
			{
				cerr << "PG_buffer error on fd : " << in_fd << endl;
				return -1;
			}
			if (t == 0)
			{
				return r;
			}
			B += c;
			r++;
		}
	}
	int write_buf()
	{
		int r = write(out_fd, B.c_str(), B.size());
		B = B.substr(r);
		return B.size();
	}
};
class PG_FD_select_socket
{
public:
	string host_name;
	int port;
	
	int fd;
	int (PG_buffer::*w_fun_ptr)();
	int (PG_buffer::*r_fun_ptr)();
	struct hostent *he;
	struct sockaddr_in sin;
	int FSM;
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
		
		int flags = fcntl(fd, F_GETFL, 0);
		fcntl(fd, F_SETFL, flags | O_NONBLOCK);
		FSM = PG_CONNECTING;
	}
	void conekuto()
	{
		if (FSM != PG_CONNECTING){cerr << "calling conekuto at the wrong time" << endl;}
		if (connect(fd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		{
			if (errno != EINPROGRESS){perror("failed in conekuto"); exit(1);}
		}
		else
		{
			FSM = PG_OK;
		}
	}
};
class PG_FD_select
{
	fd_set rfds, wfds, rs, ws;
	vector<PG_FD_select_socket> sock;
	void reset()
	{
		FD_ZERO(&rfds); FD_ZERO(&wfds); FD_ZERO(&rs); FD_ZERO(&ws);
	}
	PG_FD_select()
	{
		reset();
	}
	void add(string host_name, int port)
	{
		PG_FD_select_socket tmp;
		tmp.init(host_name, port);
		sock.push_back(tmp);
	}
	void go()
	{
	}
	
	
};
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
		struct sockaddr_in cin;
		
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
			c_fd = accept(l_fd, (struct sockaddr *) &cin, &len); 
			my_port = ntohs(cin.sin_port);
			char addr_p[INET_ADDRSTRLEN];
			my_ip = inet_ntop(AF_INET, &cin.sin_addr, addr_p, sizeof(addr_p));
			cout << "accept: " << c_fd << endl;
			cout << "IP: " << my_ip << endl;
			cout << "port: " << ntohs(cin.sin_port) << endl;
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
		}
	}
	void go()
	{
		go(7000);
	}

};
int main()
{
	PG_buffer a,b;
	b.B = "12345";
	void (PG_buffer::*t)();
	//t = &PG_buffer::write_buf;
	(b.*t)();





}
