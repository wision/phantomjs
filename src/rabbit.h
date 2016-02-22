/*
  This file is part of the PhantomJS project from Ofi Labs.

  Copyright (C) 2011 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Milian Wolff <milian.wolff@kdab.com>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef RABBIT_H
#define RABBIT_H

#include <QVariantMap>
#include <QMutex>
#include <QSemaphore>

#include "qamqp/amqp.h"
#include "qamqp/amqp_queue.h"

class Config;
class RabbitResponse;
class RabbitQueue;
class RabbitExchange;

class Rabbit : public QObject
{
    Q_OBJECT

public:
    Rabbit(QObject *parent);
    virtual ~Rabbit();

public slots:
	void openConnection(const QString& connString);
	void connected();
	void disconnected();
	void error(QString);

	QString readFileBase64(QString);
	QString stringToBase64(QString);
	QString base64ToString(QString);
	QObject* createQueue();
	QObject* createExchange();

signals:
	void newConnected();
	void newDisconnected();
	void newError(QString);

public:
	QAMQP::Client* client;

private:
	QAMQP::Queue* queue_;
	QAMQP::Exchange* exchange_;
};

class RabbitResponse: public QObject {

	Q_OBJECT

public:
	RabbitResponse(QObject* parent, QAMQP::Queue* q, QAMQP::MessagePtr &m);

public slots:
	void ack();

private:
	QAMQP::Queue* queue;
	QAMQP::MessagePtr message;
};

class RabbitQueue: public QObject {

	Q_OBJECT

public:
	RabbitQueue(QObject* parent, QAMQP::Client* q);

public slots:
	void declare(QString key, qint16 options);
	void setQOS(qint32 prefetchSize, quint16 prefetchCount);
	void consume();

	void messageReceived(QAMQP::Queue* q);

	void declared();
	void closed();

signals:
	void newDeclared();
	void newClosed();
	void newMsg(QString, QVariantMap headers, QString deliveryInfo, QObject *res);

private:
	QAMQP::Queue* queue;
};

class RabbitExchange: public QObject {

	Q_OBJECT

public:
	RabbitExchange(QObject* parent, QAMQP::Client* q);

public slots:
	void declare(QString key, QString type, qint16 options);
	void publish(QString message, QString routingKey, QString contentType);

	void declared();
	void closed();

signals:
	void newDeclared();
	void newClosed();

private:
	QAMQP::Exchange* exchange;
	QAMQP::Client* client;
};

#endif // amqp_H
