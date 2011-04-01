#ifndef __AVOSSOCKET_H__
#define __AVOSSOCKET_H__

#include <qobject.h>
#include "extapp_msg.h"

class QSocket;

namespace archos {

class AvosSocket : public QObject {
	Q_OBJECT

public:
	static AvosSocket *getInstance(void);
	virtual ~AvosSocket(void);
	int start(void);
	int sendMsg(extapp_msg_t* msg);
	void freeMsg(extapp_msg_t* msg);

signals:
	void packetReady(extapp_msg_t* msg);

private:
	AvosSocket(void);
	AvosSocket(const AvosSocket&);
	static AvosSocket* instance;
	QSocket *avosSocket;
	int socket_fd;

private slots:
	void handleMsg(void);

};

};

#endif // __AVOSSOCKET_H__
