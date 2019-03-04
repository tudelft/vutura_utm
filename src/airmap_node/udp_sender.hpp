
// UdpSender
class UdpSender
{
public:
	UdpSender(const char* host, const int port) : m_host(host), m_port(port), m_sockfd(-1) {}
	int connect() {
		// create socket
		m_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
		if (m_sockfd < 0)
			return -1;

		// get server
		struct hostent *server = gethostbyname(m_host);
		if (server == NULL)
			return -1;

		// set port and IP protocol
		bzero(&m_serveraddr, sizeof(m_serveraddr));
		m_serveraddr.sin_family = AF_INET;
		bcopy(server->h_addr, &m_serveraddr.sin_addr.s_addr, server->h_length);
		m_serveraddr.sin_port = htons(m_port);

		return 0;
	}

	int sendmsg(const char* msg, int msglen) {
		if (m_sockfd < 0)
			return -1;
		return sendto(m_sockfd, msg, msglen, 0, (struct sockaddr*)&m_serveraddr, sizeof(m_serveraddr));
	}

private:
	const char* m_host;
	int  m_port;
	int m_sockfd;
	struct sockaddr_in m_serveraddr;
};


