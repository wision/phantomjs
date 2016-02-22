/*
  This file is part of the PhantomJS project from Ofi Labs.

  Copyright (C) 2011 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Copyright (C) 2011 Ivan De Marino <ivan.de.marino@gmail.com>
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

#include <string>
#include <iostream>
#include "rabbit.h"

#include "encoding.h"
#include "qamqp/amqp.h"
#include "qamqp/amqp_queue.h"
#include "qamqp/amqp_exchange.h"

#include "consts.h"

#include <QByteArray>
#include <QHostAddress>
#include <QMetaType>
#include <QThread>
#include <QUrl>
#include <QVector>
#include <QDebug>

#include <QFile>

Rabbit::Rabbit(QObject *parent)
    : QObject(parent)
{
	// qInstallMsgHandler(0);
    setObjectName("rabbit");
    qRegisterMetaType<RabbitResponse*>("RabbitResponse*");
	client = new QAMQP::Client(this);
}

Rabbit::~Rabbit() {
    // close();
}

QString Rabbit::readFileBase64(QString path) {
	QFile* file = new QFile(path);
	file->open(QIODevice::ReadOnly);
	return QString(file->readAll().toBase64());
}

QString Rabbit::stringToBase64(QString string) {
    QByteArray ba;
    ba.append(string);
    return ba.toBase64();
	// return string.toUtf8().toBase64();
}

QString Rabbit::base64ToString(QString string) {
	QByteArray ba = QByteArray::fromBase64(string.toUtf8());
	return QString::fromUtf8(ba.data(), ba.size());
}


QObject* Rabbit::createQueue() {
	return new RabbitQueue(this, client);
}


QObject* Rabbit::createExchange() {
	return new RabbitExchange(this, client);
}

void Rabbit::openConnection(const QString& connString) {
	QUrl con((QString(connString)));
	connect(client, SIGNAL(connected()), this, SLOT(connected()));
	connect(client, SIGNAL(disconnected()), this, SLOT(disconnected()));
	connect(client, SIGNAL(error(QString)), this, SLOT(error(QString)));
	client->open(con);
}

void Rabbit::disconnected() {
	newDisconnected();
}

void Rabbit::error(QString e) {
	newError(e);
}

void Rabbit::connected() {
	newConnected();
}

// ----------------------------------------------------------------------------

RabbitQueue::RabbitQueue(QObject* parent,QAMQP::Client* client)
	:QObject(parent)
{
    setObjectName("rabbitQueue");
	queue = client->createQueue();
}


void RabbitQueue::declare(QString name, qint16 options) {
    connect(queue, SIGNAL(declared()), this, SLOT(declared()));
    connect(queue, SIGNAL(closed()), this, SLOT(closed()));
	queue->declare(name, QAMQP::Queue::QueueOptions(options));
}

void RabbitQueue::setQOS(qint32 prefetchSize, quint16 prefetchCount) {
	queue->setQOS(prefetchSize, prefetchCount);
}

void RabbitQueue::messageReceived(QAMQP::Queue* q) {
	while (q->hasMessage()) {
		QAMQP::MessagePtr message = q->getMessage();

		// q->ack(message);
		// return;

		QString s = QString::fromUtf8(message->payload.data(), message->payload.size());


		RabbitResponse *rr = new RabbitResponse(this, q, message);
		QVariantMap headers;

		// todo takhle se dostanu k properties
		// QHash<QAMQP::Frame::Content::Property, QVariant>::iterator i;
		// for (i = message->property.begin(); i != message->property.end(); ++i)
		// {
			// if(i.key() == AMQP_BASIC_HEADERS_FLAG) {
				// std::cout << i.key() << "---\n";
				// QString g = i.value().toString();
				// std::cout << "ttttttttt " << i.value().type() << "\n";
				// headersObject["headers"] = i.value().toHash().size();
			// }
		// }

		QVariant rawHeaders = message->property[QAMQP::Message::MessageProperty(AMQP_BASIC_HEADERS_FLAG)];
		if(!rawHeaders.isNull()) {
			QHashIterator<QString, QVariant> i(rawHeaders.toHash());
			while (i.hasNext()) {
				i.next();
				headers[i.key()] = i.value();
			}
		}
		newMsg(s, headers, "DeliveryInfo: not implemented", rr);
   }
}

void RabbitQueue::consume() {
	connect(queue, SIGNAL(messageReceived(QAMQP::Queue*)), this, SLOT(messageReceived(QAMQP::Queue*)));
	queue->consume();
}

void RabbitQueue::declared() {
	newDeclared();
}

void RabbitQueue::closed() {
	newClosed();
}

// ----------------------------------------------------------------------------

RabbitExchange::RabbitExchange(QObject* parent, QAMQP::Client* client)
	:QObject(parent)
{
    setObjectName("rabbitExchange");
	this->client = client;
}

void RabbitExchange::declare(QString name, QString type, qint16 options) {
	exchange = client->createExchange(name);
    connect(exchange, SIGNAL(declared()), this, SLOT(declared()));
    connect(exchange, SIGNAL(closed()), this, SLOT(closed()));
	exchange->declare(type, QAMQP::Exchange::ExchangeOptions(options));
}

void RabbitExchange::publish(QString message, QString routingKey, QString contentType) {
	QByteArray utf8;
	utf8.append(message.toUtf8());
	exchange->publish(utf8, routingKey, contentType);

}

void RabbitExchange::declared() {
	newDeclared();
}

void RabbitExchange::closed() {
	newClosed();
}

// ----------------------------------------------------------------------------

RabbitResponse::RabbitResponse(QObject* parent, QAMQP::Queue* q, QAMQP::MessagePtr &m): QObject(parent) {
	setObjectName("response");
	queue = q;
	message = m;
}

void RabbitResponse::ack() {
	queue->ack(message);
	delete this;
}

