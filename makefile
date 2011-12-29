main: TCPIP.cpp
	g++ -o hw_4 hw_4.cpp
	g++ -o socks_client hw_4_socks_client.cpp
	g++ -o hw_3_cgi hw_3_cgi.cpp
	chmod +x hw_3_cgi
	cp ./hw_3_cgi /var/www/cgi-bin/
