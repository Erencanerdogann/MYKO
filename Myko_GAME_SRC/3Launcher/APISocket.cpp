#include "stdafx.h"
#include "APISocket.h"
#include <winsock.h>
#include "crc32.h"

static WSAData s_WSData;
int			CAPISocket::s_nInstanceCount = 0;


#ifdef _CRYPTION
BOOL		CAPISocket::s_bCryptionFlag = FALSE;			//0 : ���ȣȭ , 1 : ��ȣȭ
CJvCryption	CAPISocket::s_JvCrypt;
uint32_t	CAPISocket::s_wSendVal = 0;
uint32_t	CAPISocket::s_wRcvVal = 0;
#endif

const uint16_t PACKET_HEADER = 0XAA55;
const uint16_t PACKET_TAIL = 0X55AA;

#ifdef _N3GAME
#include "LogWriter.h"
#endif

CAPISocket::CAPISocket()
{
	m_hSocket = (void*)INVALID_SOCKET;
	m_hWndTarget = NULL;
	m_szIP.clear();
	m_dwPort = 0;

	if (s_nInstanceCount++ == 0)
		WSAStartup(0x0101, &s_WSData);

	m_iSendByteCount = 0;
	m_bConnected = FALSE;
	m_bEnableSend = TRUE; // ������ ����..?
}

CAPISocket::~CAPISocket()
{
	Release();

	s_nInstanceCount--;
	if (s_nInstanceCount == 0)
	{
		WSACleanup();
	}
}

void CAPISocket::Release()
{
	this->Disconnect();

	while (!m_qRecvPkt.empty())
	{
		auto pkt = m_qRecvPkt.front();
		delete pkt;
		m_qRecvPkt.pop();
	}

	m_iSendByteCount = 0;

#ifdef _DEBUG
	for (int i = 0; i < 255; i++)
	{
		memset(m_Statistics_Send_Sum, 0, sizeof(m_Statistics_Send_Sum));
		memset(m_Statistics_Recv_Sum, 0, sizeof(m_Statistics_Recv_Sum));
	}
#endif
}

void CAPISocket::Disconnect()
{
	if ((SOCKET)m_hSocket != INVALID_SOCKET)
		closesocket((SOCKET)m_hSocket);

	m_hSocket = (void*)INVALID_SOCKET;
	m_hWndTarget = NULL;
	m_szIP.clear();
	m_dwPort = 0;

	m_bConnected = FALSE;
	m_bEnableSend = TRUE; // ������ ����..?

#ifdef _CRYPTION
	InitCrypt(0); // ��ȣȭ ����..
#endif // #ifdef _CRYPTION
}

int CAPISocket::Connect(HWND hWnd, const char* pszIP, uint32_t dwPort)
{
	if (!pszIP || !dwPort) return -1;
	//
	if ((SOCKET)m_hSocket != INVALID_SOCKET)
		this->Disconnect();

	//
	int i = 0;
	struct sockaddr_in far server;
	struct hostent far* hp;

	if ((pszIP[0] >= '0') && (pszIP[0] <= '9'))
	{
		memset((char*)&server, 0, sizeof(server));
		server.sin_family = AF_INET;
		server.sin_addr.s_addr = inet_addr(pszIP);
		server.sin_port = htons((u_short)dwPort);
	}
	else
	{
		if ((hp = (hostent far*)gethostbyname(pszIP)) == NULL)
		{
#ifdef _DEBUG
			char msg[256];
			sprintf(msg, "Error: Connecting to %s.", pszIP);
			MessageBox(hWnd, msg, "socket error", MB_OK | MB_ICONSTOP);
#endif
			return INVALID_SOCKET;
		}
		memset((char*)&server, 0, sizeof(server));
		memcpy((char*)&server.sin_addr, hp->h_addr, hp->h_length);
		server.sin_family = hp->h_addrtype;
		server.sin_port = htons((u_short)dwPort);
	}// else 

	// create socket 
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		int iErrCode = ::WSAGetLastError();
#ifdef _DEBUG
		char msg[256];
		sprintf(msg, "Error opening stream socket");
		MessageBox(hWnd, msg, "socket error", MB_OK | MB_ICONSTOP);
#endif
		return iErrCode;
	}

	m_hSocket = (void*)sock;

	// ���� �ɼ�
	int iRecvBufferLen = RECEIVE_BUF_SIZE;
	int iErr = setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*)&iRecvBufferLen, 4);

	// TODO#241 F2 (S127): TCP CONNECT non-blocking + 5sn timeout.
	// ESKI: ciplak blocking connect() -> sunucu gec/dusuk cevap verirse OS default
	//       timeout (20-240sn) boyunca UI thread DONUYORDU ('Checking 1dk donma').
	// YENI: socket non-blocking yap -> connect baslat -> select() ile EN COK 5sn bekle.
	//       5sn'de baglanamazsa hata don (donma YOK). select dali bittikten sonra
	//       asagidaki WSAAsyncSelect zaten event-mod (async) kuruyor.
	{
		u_long ulNonBlock = 1;
		ioctlsocket(sock, FIONBIO, &ulNonBlock);  // non-blocking moda al

		int iConnRet = connect(sock, (struct sockaddr far*) & server, sizeof(server));
		bool bConnected = false;
		if (iConnRet == 0)
		{
			bConnected = true;  // aninda baglandi (lokal/hizli ag)
		}
		else if (::WSAGetLastError() == WSAEWOULDBLOCK)
		{
			// connect devam ediyor — select ile 5sn bekle (yazilabilir = baglandi)
			fd_set writeSet, exceptSet;
			FD_ZERO(&writeSet);  FD_SET(sock, &writeSet);
			FD_ZERO(&exceptSet); FD_SET(sock, &exceptSet);
			struct timeval tv; tv.tv_sec = 5; tv.tv_usec = 0;

			int iSel = select(0, NULL, &writeSet, &exceptSet, &tv);
			if (iSel > 0 && FD_ISSET(sock, &writeSet))
			{
				// SO_ERROR ile gercek baglanti sonucunu dogrula (writeSet tek basina yetmez)
				int soErr = 0; int soLen = sizeof(soErr);
				getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&soErr, &soLen);
				bConnected = (soErr == 0);
			}
			// iSel == 0 -> 5sn timeout (donma yok), iSel < 0 / exceptSet -> hata
		}

		if (!bConnected)
		{
			int iErrCode = ::WSAGetLastError();
			if (iErrCode == 0) iErrCode = WSAETIMEDOUT;  // select-timeout durumu
			closesocket(sock);
			m_hSocket = (void*)INVALID_SOCKET;
			return iErrCode;
		}
	}

	WSAAsyncSelect(sock, hWnd, WM_SOCKETMSG, FD_CONNECT | FD_READ | FD_CLOSE);

	m_hWndTarget = hWnd;
	m_szIP = pszIP;
	m_dwPort = dwPort;
	m_bConnected = TRUE;

#ifdef _DEBUG
	for (i = 0; i < 255; i++)
	{
		memset(m_Statistics_Send_Sum, 0, sizeof(m_Statistics_Send_Sum));
		memset(m_Statistics_Recv_Sum, 0, sizeof(m_Statistics_Recv_Sum));
	}
#endif

	return 0;
}

int	CAPISocket::ReConnect()
{
	return this->Connect(m_hWndTarget, m_szIP.c_str(), m_dwPort);
}

void CAPISocket::Receive()
{
	if (INVALID_SOCKET == (SOCKET)m_hSocket || FALSE == m_bConnected)
		return;

	u_long	dwPktSize;
	u_long	dwRead = 0;
	int		count = 0;

	ioctlsocket((SOCKET)m_hSocket, FIONREAD, &dwPktSize);
	while (dwRead < dwPktSize)
	{
		count = recv((SOCKET)m_hSocket, (char*)m_RecvBuf, RECEIVE_BUF_SIZE, 0);
		if (count == SOCKET_ERROR)
		{
			__ASSERT(0, "socket receive error!");
#ifdef _N3GAME
			int iErr = ::GetLastError();
			CLogWriter::Write("socket receive error! : %d", iErr);
			//TRACE("socket receive error! : %d\n", iErr);
#endif
			break;
		}
		if (count)
		{
			dwRead += count;
			m_CB.PutData(m_RecvBuf, count);
		}
	}

	// packet analysis.
	while (ReceiveProcess());
}

BOOL CAPISocket::ReceiveProcess()
{
	int iCount = m_CB.GetValidCount();
	BOOL bFoundTail = FALSE;
	if (iCount >= 7)
	{
		uint8_t* pData = new uint8_t[iCount];
		m_CB.GetData(pData, iCount);
		int head_inc_size = 0;

		if (PACKET_HEADER == ntohs(*((uint16_t*)pData)))
		{
			int16_t siCore = *((int16_t*)(pData + 2));
			if (siCore <= iCount)
			{
				if (PACKET_TAIL == ntohs(*((uint16_t*)(pData + iCount - 2)))) // ��Ŷ ���� �κ� �˻�..
				{
					Packet* tmpPkt = new Packet();
					if (s_bCryptionFlag)
					{
						static uint8_t pTBuf[RECEIVE_BUF_SIZE];
						s_JvCrypt.JvDecryptionFast(siCore, pData + 4, pTBuf);

						uint16_t sig = *(uint16_t*)pTBuf;

						if (sig != 0x1EFC)
						{
							__ASSERT(0, "Crypt Error");
						}
						else
						{
							uint16_t sequence = *(uint16_t*)&pTBuf[2];
							uint8_t  empty = pTBuf[4];
							uint8_t* payload = &pTBuf[5];

							tmpPkt->append(payload, siCore - 5);
						}
					}
					else
					{
						tmpPkt->append(pData + 4, siCore);
					}

					int opCode = tmpPkt->read<uint8_t>();

					Packet* pkt = new Packet(opCode);
					pkt->append(&tmpPkt->contents()[1], tmpPkt->size() -1);

					m_qRecvPkt.push(pkt);
					m_CB.HeadIncrease(siCore + 6); // ȯ�� ���� �ε��� ���� ��Ű��..
					bFoundTail = TRUE;
#ifdef _DEBUG
					uint8_t byCmd = pData[4];
					m_Statistics_Recv_Sum[byCmd].dwTime++;
					m_Statistics_Recv_Sum[byCmd].iSize += siCore;
#endif
				}
			}
		}
		else
		{
			// ��Ŷ�� ������??
			__ASSERT(0, "broken packet header.. skip!");
			m_CB.HeadIncrease(iCount); // ȯ�� ���� �ε��� ���� ��Ű��..
		}

		delete[] pData, pData = NULL;
	}

	return bFoundTail;
}

void CAPISocket::Send(uint8_t* pData, int nSize)
{
	if (!m_bEnableSend) return; // ������ ����..?
	if (INVALID_SOCKET == (SOCKET)m_hSocket || FALSE == m_bConnected)
		return;

#ifdef _CRYPTION
	DataPack DP;

	if (s_bCryptionFlag)
	{
		static uint8_t pTBuf[RECEIVE_BUF_SIZE];

		++s_wSendVal;

		memcpy(pTBuf, &s_wSendVal, sizeof(uint32_t));
		memcpy((pTBuf + 4), pData, nSize);

		*((uint32*)(pTBuf + (nSize + 4))) = crc32(pTBuf, (nSize + 4), -1);

		s_JvCrypt.JvEncryptionFast((nSize + 4 + 4), pTBuf, pTBuf);

		DP.m_Size = (nSize + 4 + 4);
		DP.m_pData = new uint8_t[DP.m_Size];
		memcpy(DP.m_pData, pTBuf, DP.m_Size);

		nSize = DP.m_Size;
		pData = DP.m_pData;
	}
#endif

	int nTotalSize = nSize + 6;
	if (nTotalSize > 200)
		return;

	uint8_t* pSendData = m_RecvBuf;
	*((uint16_t*)pSendData) = htons(PACKET_HEADER);	pSendData += 2;
	*((uint16_t*)pSendData) = nSize;				pSendData += 2;
	memcpy(pSendData, pData, nSize);			pSendData += nSize;
	*((uint16_t*)pSendData) = htons(PACKET_TAIL);	pSendData += 2;

	int nSent = 0;
	int count = 0;
	while (nSent < nTotalSize)
	{
		count = send((SOCKET)m_hSocket, (char*)m_RecvBuf, nTotalSize, 0);
		if (count == SOCKET_ERROR)
		{
			__ASSERT(0, "socket send error!");
#ifdef _N3GAME
			int iErr = ::GetLastError();
			CLogWriter::Write("socket send error! : %d", iErr);
			//TRACE("socket send error! : %d\n", iErr);
			PostQuitMessage(-1);
#endif
			break;
		}
		if (count)
		{
			nSent += count;
		}
	}

#ifdef _DEBUG
	uint8_t byCmd = pData[0]; // ��� �ֱ�..

//	__SocketStatisics SS;
//	SS.dwTime = GetTickCount();
//	SS.iSize = nSize;
//	m_Statistics_Send[byCmd].push_back(SS);

	m_Statistics_Send_Sum[byCmd].dwTime++;
	m_Statistics_Send_Sum[byCmd].iSize += nSize;
#endif

	m_iSendByteCount += nTotalSize;
}
