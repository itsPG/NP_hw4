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
class PG_socks_v4_client
{
public:
	string encode_request(int mode, int port, int a, int b, int c, int d, string id, string dn)
	{
		string r = "";
		r += (unsigned char)4;
		r += (unsigned char)mode;
		r += (unsigned char)(port / 256);
		r += (unsigned char)(port % 256);
		r += (unsigned char)a;
		r += (unsigned char)b;
		r += (unsigned char)c;
		r += (unsigned char)d;
		r += id;
		r += '\0';
		if (a == 0 && b == 0 && c == 0)
		{
			r += dn;
			r += '\0';
		}
		return r;
	}
	int conekuto(int r_fd, int w_fd, int mode, int port, int a, int b, int c, int d, string id, string dn)
	{
		string tmp = encode_request(mode, port, a, b, c, d, id, dn);
		write(w_fd, tmp.c_str(), tmp.size());
		char buf[8] = {0};
		read(r_fd, buf, 8);
		if (buf[1] != 90)
		{
			cerr << "socks server connect failed" << endl;
			exit(1);
		}
		else
		{
			cerr << "connect success" << endl;
			return 0;
		}
	}
};
PG_socks_v4_client Rixia;
PG_TCP_client Elie;

int main()
{
	Elie.init("itsPG.org", 8001);
	
	//this is the basic  1 / 2
	//Rixia.conekuto(Elie.fd, Elie.fd, 1, 23, 140, 112, 172, 1, "PG", "");
	
	//this is the basic  2 / 2 
	//Rixia.conekuto(Elie.fd, Elie.fd, 1, 23, 0, 0, 0, 1, "PG", "ptt.cc"); 

	//empty str
	//Rixia.conekuto(Elie.fd, Elie.fd, 1, 23, 0, 0, 0, 1, "", "");
	
	//wrong DN
	//Rixia.conekuto(Elie.fd, Elie.fd, 1, 23, 0, 0, 0, 1, "", "asdfasdfawixefrjapserogj.rgfhjauioshrgpu.asrdghiuqerhgp.0.0");
	
	//wrong DN2
	//Rixia.conekuto(Elie.fd, Elie.fd, 1, 23, 0, 0, 0, 1, "", "ptt.cc\0ptt2.cc\0asdf\0");
	
	string big = "0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789";
	for (int i = 1; i <= 7; i++)big += big;
	//Rixia.conekuto(Elie.fd, Elie.fd, 1, 23, 0, 0, 0, 1, big, big);
	
	Rixia.conekuto(Elie.fd, Elie.fd, 1, 23, 0, 0, 0, 1, "\1\2\3\4\5\6\7\8\9\10\11\12\13\14\15", "ptt.cc");
	
	//cout << "succes"
	dup2(Elie.fd, 0);
	//dup2(Elie.fd, 1);
	char c;
	while (cin.get(c)) cout << c ;
}
