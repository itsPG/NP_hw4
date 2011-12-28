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
class PG_socks_v4
{
public:
	unsigned char VN, CD;
	unsigned int DST_PORT, DST_IP;
	string USER_ID, HOST_NAME;
	string ip_str,buf;
	void get_request(int fd)
	{
		string q = "", tmp;
		PG_get(fd, q, 8);
		PG_get(fd, tmp, -1);
		q += tmp;
		buf = q;
		//cout << "get request read size " << q.size() << endl;
		if (q.size() < 9)
		{
			cerr << "!!!!!!!!!!!!!!! socks request error : " << q << " size : " << q.size() << endl;
			//exit(1);
		}
		VN = q[0];
		CD = q[1];
		DST_PORT = q[2] << 8 | q[3];
		DST_IP = 
			q[4] << 24 | 
			q[5] << 16 |
			q[6] << 8 |
			q[7];
		ip_str = i2s((unsigned char)q[4]) + "." + i2s((unsigned char)q[5]) + "." + i2s((unsigned char)q[6]) + "." + i2s((unsigned char)q[7]);
		USER_ID = q.c_str() + 8;
		cout << "#" << getpid() << " is created for " << ip_str << endl;
	}
	void print()
	{
		cout << "VN: " << (int)VN << endl;
		cout << "CD: " << (int)CD << endl;
		cout << "DST_PORT: " << DST_PORT << endl;
		cout << "DST_IP: " << ip_str << " / " << DST_IP << endl;
		cout << "USER_ID: " << USER_ID << " size: " << USER_ID.size() << endl;
	}
	void reply(int fd)
	{
		string r = "        ";
		r[0] = 0;
		r[1] = 90;
		//r[2] = port / 256;
		//r[3] = port % 256;
		//r[4] = 192;
		//r[5] = 168;
		//r[6] = 1;
		//r[7] = 11;
		for (int i = 2; i <= 7; i++) r[i] = buf[i];
		//for (int i = 0; i <= 7; i++)cout << "r[" << i << "] : " << (unsigned int)r[i] << endl;
		write(fd, r.c_str(), 8);
	}
};
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
			cout << "#" << getpid() << " got an EOF" << endl;
			return -1;
		}
		int len2 = write(to, buf, len);
		cout << "#" << getpid() << " read " << len << "byte(s) and write " << len2 << "byte(s) " << endl;
		//cout << buf << endl;
		return 0;
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
					FD_CLR(src_fd, &rs);
					state --;
					shutdown (dst_fd,2);
					close(dst_fd);
					break;
				}
			}
			else if (FD_ISSET(dst_fd, &rfds))
			{
				//cout << "DST" << endl;
				int r = transfer(dst_fd, src_fd);
				if (r == -1){FD_CLR(dst_fd, &rs); state --;}
			}
		}
		cout << "#" << getpid() << " end" << endl;
		exit(0);
	}
	
	
};
PG_TCP_server Rixia;
PG_TCP_client Elie;
PG_socks_v4 Tio;
PG_FD_binder Noel;
int main()
{
	Rixia.go(8001);
	int c_fd = Rixia.c_fd;
	Tio.get_request(c_fd);
	//Tio.print();
	Elie.init(Tio.ip_str, Tio.DST_PORT);
	Tio.reply(c_fd);
	Noel.init(c_fd, Elie.fd);
	Noel.go();
	
}
