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

int global_l_fd;
#include "TCPIP_v2.cpp"
string i2s(int q)
{
	ostringstream sout;
	sout << q;
	return sout.str();
}
int s2i(string q)
{
	int r;
	istringstream ssin(q);
	ssin >> r;
	return r;
}
PG_TCP_server Rixia, Rixia2;
PG_TCP_client Elie;
class PG_firewall
{
public:
	bool match(string &a, string &b)
	{
		if (b == "all") return 1;
		return a.find(b) == 0;
	}
	int flag[1001], flag2[1001];
	// flag2 0:any , 1:in , 2:out
	string ip[1001];
	int m;
	PG_firewall()
	{
		m = 0;
	}
	void load(string fn)
	{
		m = 0;
		ifstream fin(fn.data());
		string a, b, c, t;
		
		while (getline(fin, t))
		{
			istringstream ssin(t);
			ssin >> a >> b;
			++m;
			if (a == "allow" ) flag[m] = 0;
			else if (a == "deny") flag[m] = 1;
			else {--m; continue;}
			
			ip[m] = b;
			flag2[m] = 0;
			if (ssin >> c)
			{
				if (c == "in") flag2[m] = 1;
				if (c == "out") flag2[m] = 2;
			}
		}
	}
	int chk(string q)
	{
		for (int i = 1; i <= m; i++)
		{
			if (match(q, ip[i]))
			{
				return flag[i];
			}
		}
		return 0;
	}
	int chk(string in, string out) //  0:allow , 1:deny
	{
		return chk(out);
		cout << in << "~~" << out << endl;
		int f_in = -1, f_out = -1;
		int in_lv, out_lv;
		for (int i = 1; i <= m; i++)
		{
			if (match(in, ip[i]) && (flag2[i] == 0 || flag2[i] == 1))
			{
				f_in = flag[i];
				in_lv = i;
				cout << "in " << flag[i] << " at lv " << i << endl;
				break;
			}
		}
		for (int i = 1; i <= m; i++)
		{
			if (match(out, ip[i]) && (flag2[i] == 0 || flag2[i] == 2))
			{
				f_out = flag[i];
				out_lv = i;
				cout << "out " << flag[i] << " at lv " << i << endl;
				break;
			}
		}
		if (f_in == -1 && f_out == -1) return 0;
		if (f_in == 0 && f_out == 0) return 0;
		if (f_in == 1 && f_out == 1) return 1;
		
		if (f_in == -1 && f_out != -1) return f_out;
		if (f_in != -1 && f_out == -1) return f_in;
		
		return in_lv < out_lv ? f_in : f_out;
		
	}
};

class PG_socks_v4
{
public:
	unsigned char VN, CD;
	unsigned int DST_PORT, DST_IP;
	string USER_ID, HOST_NAME;
	string ip_str, buf;
	// buf is the query data
	bool reject;
	int type;
	// type:1 -> socksv4 , type:2 -> socksv4a
	PG_firewall FireWall;
	PG_socks_v4()
	{
		FireWall.load("iptable.txt");
		
	}
	void decode_request(string q)
	{
		
		VN = (unsigned char)q[0];
		CD = (unsigned char)q[1];
		DST_PORT = (unsigned char)q[2] << 8 | (unsigned char)q[3];
		DST_IP = 
			(unsigned char)q[4] << 24 | 
			(unsigned char)q[5] << 16 |
			(unsigned char)q[6] << 8 |
			(unsigned char)q[7];
		ip_str = i2s((unsigned char)q[4]) + "." + i2s((unsigned char)q[5]) + "." + i2s((unsigned char)q[6]) + "." + i2s((unsigned char)q[7]);
		USER_ID = "";
		USER_ID = q.substr(8);
		
	}
	void err_exit(string q)
	{
		cout << "\033[1;35m" ;
		cout << q;
		cout << "\033[0;m" <<endl;
		exit(1);
	}
	void chk_request()
	{
		if (VN != 4)err_exit("VERSION check failed");
		if (CD != 1 && CD != 2)err_exit("COMMAND check failed");
	}
	void get_request(int fd)
	{
		reject = 0;
		string q = "", tmp;
		PG_get(fd, q, 8);
		PG_get(fd, tmp, -1);
		q += tmp;
		buf = q;
		decode_request(q);
		chk_request();
		if (q[4] == 0 && q[5] == 0 && q[6] == 0)
		{
			PG_get(fd, HOST_NAME, -1);
			struct hostent *he;
			he = gethostbyname(HOST_NAME.c_str());
			struct sockaddr_in sin;
			sin.sin_addr = *((struct in_addr *)he->h_addr);
			char addr_p[INET_ADDRSTRLEN];
			ip_str = inet_ntop(AF_INET, &sin.sin_addr, addr_p, sizeof(addr_p));
		}
		//cout << "get request read size " << q.size() << endl;
		if (q.size() < 9)
		{
			cerr << "!!!!!!!!!!!!!!! socks request error : " << q << " size : " << q.size() << endl;
			//exit(1);
		}
		
		
		cout << "#" << getpid() << " is created for " << ip_str << endl;
		reject = FireWall.chk(Rixia.my_ip, ip_str);
		// 1:in, 2:out 
		// 
	}
	void print()
	{
		cout << "\033[1;33m" ;
		cout << "VN: " << (int)VN << ", CD: " << (int)CD << ", DST IP: " << ip_str;
		cout << ",DST_PORT: " << DST_PORT << ", USERID:" << USER_ID << endl;
		cout << "\033[0;m" ;
	}
	void allow_msg()
	{
		cout << "\033[1;32m" ;
		cout << "Permit Src = " << Rixia.my_ip << "(" << Rixia.my_port << "), ";
		cout << "Dst = " << ip_str << "(" << DST_PORT << ")" << endl;
		cout << "\033[0;m" ;
	}
	void deny_msg()
	{
		cout << "\033[1;31m" ;
		cout << "Reject Src = " << Rixia.my_ip << "(" << Rixia.my_port << "), ";
		cout << "Dst = " << ip_str << "(" << DST_PORT << ")" << endl;
		cout << "\033[0;m" ;
	}
	void reply(int fd)
	{
		string r = "        ";
		r[0] = 0;
		r[1] = 90 + reject;
		
		for (int i = 2; i <= 7; i++) r[i] = buf[i];
		write(fd, r.c_str(), 8);
	}
	void BIND_reply(int fd, int port)
	{
		string r = "        ";
		r[0] = 0;
		r[1] = 90 + reject;
		r[2] = port / 256;
		r[3] = port % 256;
		r[4] = 0;
		r[5] = 0;
		r[6] = 0;
		r[7] = 0;
		write(fd, r.c_str(), 8);
	}
};
PG_socks_v4 Tio;
class PG_FD_binder
{
public:
	fd_set rfds, wfds, rs, ws;
	int src_fd, dst_fd;

	void init(int a, int b)
	{
		src_fd = a; dst_fd = b;
		FD_ZERO(&rfds); FD_ZERO(&wfds); FD_ZERO(&rs); FD_ZERO(&ws);
		FD_SET(src_fd, &rs);
		FD_SET(dst_fd, &rs);
	}
	int transfer(int from, int to)
	{
		char buf[500000] = {0};
		int len = read(from, buf, 500000);
		if (len == 0)
		{
			//cout << "#" << getpid() << " got an EOF" << endl;
			return -1;
		}
		int len2 = write(to, buf, len);
		//cout << "#" << getpid() << " read " << len << "byte(s) and write " << len2 << "byte(s) " << endl;
		//cout << buf << endl;
		Tio.allow_msg();
		cout << "\033[0;32m" ;
		string tmp = buf;
		cout << tmp.substr(0,70) ;
		cout << "\033[0;m" << endl;
		return 0;
	}
	void close_socket(int q)
	{
		FD_CLR(q, &rs);
		shutdown (q, 2);
		close(q);
	}
	void go()
	{
		int state = 2;
		while(state > 0)
		{
			memcpy(&rfds, &rs, sizeof(rfds));
			
			//cout << "before select" << endl;
			int s_s = select(1024, &rfds, (fd_set*)0, (fd_set*)0, (struct timeval*)0);
			if (s_s < 0){perror("select error"); exit(1);}
			//cout << "~ select ~" << endl;
			if (FD_ISSET(src_fd, &rfds))
			{
				//cout << "SRC" << endl;
				int r = transfer(src_fd, dst_fd);
				//cout << "r " << r << endl;
				if (r == -1)
				{
					close_socket(src_fd);
					close_socket(dst_fd);
					state = -1;
					break;
				}
			}
			else if (FD_ISSET(dst_fd, &rfds))
			{
				//cout << "DST" << endl;
				int r = transfer(dst_fd, src_fd);
				if (r == -1)
				{
					close_socket(src_fd);
					close_socket(dst_fd);
					state = -1;
					break;
				}
			}
		}
		cout << "#" << getpid() << " end" << endl;
		exit(0);
	}
	
	
};


PG_FD_binder Noel;

int main()
{
	Rixia.init_and_bind(8001,0);
	//Rixia2.init_and_bind(60001,1);
	global_l_fd = Rixia2.l_fd;
	Rixia.go();
	
	
	int c_fd = Rixia.c_fd;
	int bind_port;
	Tio.get_request(c_fd);
	Tio.print();
	if (Tio.reject)
	{
		Tio.deny_msg();
		string tmp = "  "; tmp[0] = 0; tmp[1] = 90;
		write(c_fd, tmp.c_str(), 2);
		exit(0);
	}
	if (Tio.CD == 1)
	{
		cout << "\033[1;36m" ;
		cout << "#" << getpid() << " CONNECT" ;
		cout << "\033[0;m" << endl;
		
		Elie.init(Tio.ip_str, Tio.DST_PORT);
		Tio.reply(c_fd);
		Noel.init(c_fd, Elie.fd);
		Noel.go();
	}
	if (Tio.CD == 2)
	{
		
		cout << "\033[1;36m" ;
		cout << "#" << getpid() << " BIND";
		cout << "\033[0;m" << endl;
		
		for (int i = 60001; i <= 65000; i++)
		{
			cout << "trying " << i << endl;
			int r = Rixia2.init_and_bind(i,1);
			if (r != -1){bind_port = i; cout << "using " << i << endl; break; }
		}
		Tio.BIND_reply(c_fd, bind_port);
		Rixia2.go();
		Tio.BIND_reply(c_fd, bind_port);
		cout << "#" << getpid() << " replyed" << endl;
		close(Rixia2.l_fd);
		Noel.init(Rixia2.c_fd, c_fd);
		Noel.go();
	}
	
}
