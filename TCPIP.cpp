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
#define BUFFER_DEBUG 0
using namespace std;
const int PG_NEW = 0, PG_CONNECTING = 1, PG_OK = 2, PG_CLOSE = 3;
class PG_buffer
{
public:
	string B;
	char c;
	int fd;
	PG_buffer()
	{
		B = "";
		fd = -1;
	}
	int read_buf()
	{
		int r = 0, t;
		int count = 0;
		while (1)
		{
			t = read(fd, &c, 1);
			if (t == 0) return -1; // got an EOF
			if (t == -1) break;
			B += c;
			r++;

		}
		if (BUFFER_DEBUG)cout << "read " << r << " byte(s) from fd: " << fd << endl;
		cout << B.substr(B.size()-r) << endl;
		return r;
	}
	int write_buf()
	{
		if (B.size() == 0)
		{
			if (BUFFER_DEBUG)cout << "nothing to write" << endl;
			return 0;
		}
		int r = write(fd, B.c_str(), B.size());
		if (r == B.size()) B = "";
		else B = B.substr(r);
		if (BUFFER_DEBUG)cout << "write " << r << " byte(s) to fd: " << fd << endl;
		return B.size();
	}
	string get()
	{
		string r = B;
		B = "";
		return r;
	}
	int get(string &q, int len)
	{
		if (B.size() < len)return -1;
		q = B.substr(0,len);
		B = B.substr(len);
		return 0;
	}
	int put(string &q)
	{
		B += q;
		return 0;
	}

};
class PG_FD_select_socket
{
public:
	string host_name;
	int port;
	PG_buffer r_buf, w_buf;
	int fd;
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
		r_buf.fd = fd; w_buf.fd = fd;
		conekuto();
	}
	void conekuto()
	{
		if (FSM != PG_CONNECTING){cerr << "calling conekuto at the wrong time" << endl;}
		if (connect(fd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		{
			if (errno != EINPROGRESS){perror("failed in conekuto"); exit(1);}
			else cout << "connecting but failed" << endl;
		}
		else
		{
			FSM = PG_OK;
		}
	}
	int send(){return w_buf.write_buf();}
	int recv(){return r_buf.read_buf();}
	string get(){return r_buf.get();}
	int get(string &q, int len){return r_buf.get(q,len);}
	int put(string q){return w_buf.put(q);}
	bool r_chk(){return r_buf.B.size() > 0;}
	bool w_chk(){return w_buf.B.size() > 0;}
};
class PG_FD_select
{
public:
	fd_set rfds, wfds, rs, ws;
	vector<PG_FD_select_socket> sock;
	int close_count;
	int link_table[1000];
	void reset()
	{
		//memset(link_table, 0, sizeof(link_table));
		for (int i = 0; i < 1000; i++)link_table[i] = -1;
		close_count = 0;
		FD_ZERO(&rfds); FD_ZERO(&wfds); FD_ZERO(&rs); FD_ZERO(&ws);
	}
	PG_FD_select()
	{
		reset();
	}
	int get(int q)
	{
		for (int i = 0; i < sock.size(); i++)
			if (sock[i].fd == q) return i;
		cerr << "get sock user_number error at fd: " << q << endl;
		return -1;
	}
	void add(string host_name, int port)
	{
		PG_FD_select_socket tmp;
		tmp.init(host_name, port);
		FD_SET(tmp.fd, &rs); FD_SET(tmp.fd, &ws);
		sock.push_back(tmp);

	}
	void add_fd(int q)
	{
		PG_FD_select_socket tmp;
		tmp.fd = q;
		FD_SET(tmp.fd, &rs); FD_SET(tmp.fd, &ws);
		sock.push_back(tmp);
	}
	int go()
	{
		
		char buf[100000];
		socklen_t n;
		while(sock.size() > close_count)
		{

			memcpy(&rfds, &rs, sizeof(rfds));
			memcpy(&wfds, &ws, sizeof(wfds));
			int s_s = select(1024, &rfds, &wfds, (fd_set*)0, (struct timeval*)0);
			if (s_s < 0){perror("select error1"); exit(1);}
			int t, error;
			usleep(500000);
			if (s_s == 0){cout << "not ready yet" << endl; continue;}
			for (int i = 0; i < sock.size(); i++)
			{

				switch (sock[i].FSM)
				{
					case PG_CONNECTING:
						cout << "switch connecting" << endl;
						if (FD_ISSET(sock[i].fd, &rfds) || FD_ISSET(sock[i].fd, &wfds))
						{
							if (getsockopt(sock[i].fd, SOL_SOCKET, SO_ERROR, (void*)&error, &n) < 0 || error != 0)
							{
								perror("select error2");
								exit(1);
							}
							else sock[i].FSM = PG_OK;
						}
						cout << "connecting end" << endl;
						cout << sock[i].FSM << endl;
						break;
					
					case PG_OK:
						//cout << "rfds " << FD_ISSET(sock[i].fd, &rfds) << endl;
						//cout << "wfds " << FD_ISSET(sock[i].fd, &wfds) << endl;
						if (FD_ISSET(sock[i].fd, &wfds))
						{
							//cout << "switch send" << endl;
							sock[i].send();
						}
						if (FD_ISSET(sock[i].fd, &rfds))
						{
							//cout << "switch recv" << endl;
							int t = sock[i].recv();
							
							
							if (t == -1) // means read return 0
							{
								FD_CLR(sock[i].fd, &rs);
								FD_CLR(sock[i].fd, &ws);
								shutdown(sock[i].fd, 2);
								close(sock[i].fd);
								close_count ++;
								cerr << "sock got an EOF" << endl;
								break;
							}
							
							cout << t << endl;
							if (link_table[i] != -1)
							{
								int aim = link_table[i];
								sock[aim].put(sock[i].get());
							}
						}
						break;	
				}
			}
		}
		cout << "go end" << endl;
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
			usleep(50000);
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
				//dup2(c_fd, 0);
				//dup2(c_fd, 1);
				//dup2(c_fd, 2);
				//close(c_fd);
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
	//PG_TCP_server Rixia;
	//Rixia.go(8002);
	PG_FD_select a;
	a.add("192.168.1.11",8001);
	a.add("192.168.1.11",7500);
	a.link_table[0] = 1;
	a.go();
	cout << "end" << endl;
	int b;
	while(cin >> b);

}
