#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <qsocket.h>

#include "avossocket.h"
#include "extapp_msg.h"

//#define ALOG_ENABLE_DEBUG
#include "alog.h"

using namespace archos;

static const char* stringForType(int type)
{
	static const char* strings[] =
	{
	"",
	"EXTAPP_PACKET_STARTUP_READY",
	"EXTAPP_PACKET_TOGGLE_VISIBLE",
	"EXTAPP_PACKET_BATTERY_STATUS",
	"EXTAPP_PACKET_LANG_CHANGED",
	"EXTAPP_PACKET_WIFI_STAT",
	"EXTAPP_PACKET_TERMINATE",
	"EXTAPP_PACKET_REQUEST_BATTERY_STATUS",
	"EXTAPP_PACKET_REQUEST_WIFI_STAT",
	"EXTAPP_PACKET_NO_POWEROFF",
	"EXTAPP_PACKET_ALLOW_POWEROFF",
	"EXTAPP_PACKET_BACKLIGHT_ON",
	"EXTAPP_PACKET_OPEN_URL",
	"EXTAPP_PACKET_TIMEFORMAT_CHANGED",
	"EXTAPP_PACKET_REQUEST_SOUND_STATUS",
	"EXTAPP_PACKET_SOUND_STATUS",
	};
	
	static const char* unknown = "*UNKNOWN*";

	if (type < sizeof(strings) / sizeof(char*)) {
		return strings[type];
	} else {
		ALOG_WARNING("Unknown message type %d", type);
		return unknown;
	}
}

/*
Usage example:

#include <extapp.h>
#include <avossocket.h>
[...]

QPdfDlg::QPdfDlg() : QWidget(NULL, "mainwidget", QWidget::WStyle_Customize | 
QWidget::WStyle_NoBorder)
        , m_docLoaded(false)
        , m_pages(0)
        , m_doc(0)
{
        avossocket = archos::AvosSocket::getInstance();
        if ( avossocket->start() == -1) {
                qWarning("failed to setup the avossocket");
                qApp->quit();
        }
        connect(avossocket, SIGNAL(packetReady(extapp_msg_t*)), this, 
SLOT(handleMsg(extapp_msg_t*)));
       [...]
}

[...]

void QPdfDlg::handleMsg(extapp_msg_t* msg)
{
        switch ( msg->header.packet_type ) {
                case EXTAPP_PACKET_BATTERY_STATUS:
                        qWarning("new battery status 0x%x\n", *((int*)msg->data));
                        break;
                case EXTAPP_PACKET_LANG_CHANGED:
                        atr_unload();
                        if (atr_load(tr_path) ) {
                                qWarning("failed to load translations!\n");
                        }
                        qApp->setFont(FontChooser::getFont());
                        restartGui();
                        break;
                default:
                        qWarning("apdf: unkown message (%x).\n", msg->header.packet_type);
                        break;
        }
        avossocket->freeMsg(msg);
}
*/

archos::AvosSocket* archos::AvosSocket::getInstance()
{
	if ( instance == 0 ) {
		instance = new AvosSocket();
	}
	return instance;
}

archos::AvosSocket::AvosSocket(void)
	: avosSocket(new QSocket()) { }

archos::AvosSocket::~AvosSocket(void)
{
	delete avosSocket;

}

int archos::AvosSocket::start(void)
{
	if ( (socket_fd = socket(PF_UNIX, SOCK_STREAM, 0)) < 0 ) {
		ALOG_WARNING("Failed to open comm socket to AVOS");
		return -1;
	}

	struct sockaddr addr;
	addr.sa_family = AF_UNIX;
	strcpy(addr.sa_data, "/tmp/easocket");

	if (::connect(socket_fd, &addr, sizeof(addr)) == -1) {
		ALOG_WARNING("Failed to connect() comm socket to AVOS");
		return -2;
	}

	avosSocket->setSocket(socket_fd);
	connect(avosSocket, SIGNAL(readyRead()), this, SLOT(handleMsg()) );
	return 0;
}

#define HEADER_LEN	sizeof(extapp_msg_header_t)

void archos::AvosSocket::handleMsg(void)
{
	ALOG_DEBUG_FUNCTION_BLOCK;
	static char *msg_buf = 0;
	static unsigned int buffer_fill = 0;
	static int header_complete = 0;
	int msg_len;

	while ( 1 ) {
		// try to get a header first
		if ( !header_complete ) {
			ALOG_DEBUG("!header_complete");
			if ( msg_buf == 0 ) {
				msg_buf = (char*)calloc(1, sizeof(extapp_msg_t));
				if ( msg_buf == 0 ) {
					ALOG_WARNING("extapp oom!");
					break;
				}
			}
			msg_len = avosSocket->readBlock(msg_buf + buffer_fill, HEADER_LEN - buffer_fill);
			ALOG_DEBUG("read %d bytes", msg_len);
			if ( msg_len >= 0 ) {
				buffer_fill += msg_len;
			}
			else {
				ALOG_WARNING("error while reading header from the extapp socket.");
				buffer_fill = 0;
				break;
			}
			if ( buffer_fill < sizeof(extapp_msg_header_t) ) {
				ALOG_WARNING("header incomplete (expected %d bytes, got %d).", sizeof(extapp_msg_header_t), buffer_fill);
				break;
			}
			buffer_fill = 0;
		
			extapp_msg_header_t *header = (extapp_msg_header_t*)msg_buf;
			if ( header->magic != (int)EXTAPP_MAGIC ) {
				ALOG_WARNING("extapp header magic missing!");
				buffer_fill = 0;
				break;
			}
		
			if ( header->data_length > EXTAPP_MSG_MAX_DATA_LENGTH ) {
				ALOG_WARNING("%i is greater than EXTAPP_MSG_MAX_DATA_LENGTH", header->data_length);
				buffer_fill = 0;
				break;
			}
			header_complete = 1;
			buffer_fill = 0;
		}
		
		// a packet with a data portion, go and get it
		extapp_msg_t *msg = (extapp_msg_t*)msg_buf;
		if ( msg->header.data_length > 0 ) {
			if ( msg->data == 0 ) {
				msg->data = (char*)calloc(1, msg->header.data_length);
				if ( msg->data == 0 ) {
					ALOG_WARNING("extapp oom!");
					break;
				}
			}
			msg_len = avosSocket->readBlock((char*)msg->data + buffer_fill, msg->header.data_length - buffer_fill);
			if ( msg_len >= 0 ) {
				buffer_fill += msg_len;
			}
			else {
				ALOG_WARNING("error while reading msg from the extapp socket.");
				buffer_fill = 0;
				header_complete = 0;
				break;
			}
		
			if ( buffer_fill < msg->header.data_length ) {
				ALOG_WARNING("data incomplete.");
				break;
			}
		}
		header_complete = 0;
		buffer_fill = 0;
		
		ALOG_DEBUG("packet ready (type=%s)", stringForType(((extapp_msg_t*)msg_buf)->header.packet_type));
		emit packetReady((extapp_msg_t*)msg_buf);
		msg_buf = 0;
	}
}

// must be called for every packet passed as paramter of the packetReady(extapp_msg_t*) signal.
void archos::AvosSocket::freeMsg(extapp_msg_t* msg)
{
	free(msg->data);
	free(msg);
}

int archos::AvosSocket::sendMsg(extapp_msg_t* msg)
{
	ALOG_DEBUG_FUNCTION_BLOCK;
	ALOG_DEBUG("type=%s", stringForType(msg->header.packet_type));

	unsigned int count = 0;
	int err = 0;
	while ( count < sizeof(extapp_msg_header_t) ) {
		err = avosSocket->writeBlock(((char*)&msg->header) + count, sizeof(extapp_msg_header_t) - count );
		if ( err == -1 ) {
			ALOG_WARNING("failed to write extapp msg header");
			return -1;
		}
		count += err;
		ALOG_DEBUG("%i bytes of the extapp msg header written", count);
	}

	count = 0;
	while ( count < msg->header.data_length ) {
		err = avosSocket->writeBlock(((char*)msg->data) + count, msg->header.data_length + count);
		if ( err == -1 ) {
			ALOG_WARNING("failed to write extapp msg");
			return -1;
		}
		count += err;
		ALOG_DEBUG("%i bytes of the extapp msg written", count);
	}
	ALOG_DEBUG("message sent");
	return 0;
}

archos::AvosSocket *archos::AvosSocket::instance = NULL;
