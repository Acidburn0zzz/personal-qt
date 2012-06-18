/*
    Personal-qt
    Copyright (C) 2012 Karasu

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "httpdaemon.h"

#include "constants.h"

HttpDaemon::HttpDaemon(int port, QObject *parent) :
    QTcpServer(parent)
{
    listen(QHostAddress::Any, port);
}

void HttpDaemon::incomingConnection(int socket)
{
    if (disabled)
        return;

    // When a new client connects, the server constructs a QTcpSocket and all
    // communication with the client is done over this QTcpSocket. QTcpSocket
    // works asynchronously, this means that all the communication is done
    // in the two slots readClient() and discardClient().
    QTcpSocket* s = new QTcpSocket(this);
    connect(s, SIGNAL(readyRead()), this, SLOT(readClient()));
    connect(s, SIGNAL(disconnected()), this, SLOT(discardClient()));
    s->setSocketDescriptor(socket);

    qDebug() << "New connection";
}


void HttpDaemon::readClient()
{
    if (disabled) return;

    // This slot is called when the client sent data to the server. The
    // server looks if it was a get request and sends a very simple HTML
    // document back.
    QTcpSocket *socket = (QTcpSocket *)sender();

    if (socket->canReadLine())
    {

        QStringList tokens = QString(socket->readLine()).split(QRegExp("[ \r\n][ \r\n]*"));

        if (tokens[0] == "GET")
        {          
            QTextStream os(socket);
            os.setAutoDetectUnicode(true);
            os << "HTTP/1.0 200 Ok\r\n"
                  "Content-Type: text/html; charset=\"utf-8\"\r\n"
                  "\r\n";

            QString myDate;

            if (tokens[1].size() > 1)
            {
                qDebug() << tokens[1];
                myDate = tokens[1].remove(0,1);
            }
            else
            {
                QDate today = QDate::currentDate();
                myDate = QString().sprintf("%d%02d%02d", today.year(), today.month(), today.day());
            }

            QSqlDatabase db = QSqlDatabase::database(DB_CONNECTION_NAME);

            QSqlQuery qry(db);

            qry.prepare("SELECT Codi, Data, Hora FROM moviments WHERE Data = :mydate");

            qry.bindValue(":mydate", myDate);

            qry.exec();

            if (qry.next())
            {
                os << "<h1>Informe a " + QDateTime::currentDateTime().toString() << "</h1><br />\n";
                os << "<h2>Dades de " + myDate + "</h2><br />\n";

                os << "<table>\n";
                os << "<tr><td>NIF</td><td>Nom</td><td>Hora</td></tr>\n";
                do
                {
                    QString codi = qry.value(0).toString();
                    // QString data = qry.value(1).toString();
                    QString hora = qry.value(2).toString();

                    QSqlQuery qry2(db);

                    qry2.prepare("SELECT NIF, Nom FROM personal WHERE Codi = :codi");

                    qry2.bindValue(":codi", codi);

                    qry2.exec();

                    if (qry2.next())
                    {
                        QString nif = qry2.value(0).toString();
                        QString nom = qry2.value(1).toString();

                        os << "<tr><td>" + nif + "</td><td>" + nom + "</td><td>" + hora + "</td></tr>\n";
                    }
                    else
                    {
                        // something wrong has happened. Can't find this code in db
                        os << "<tr><td>" + codi + "</td><td>" + hora + "</td></tr>\n";
                    }
                }
                while (qry.next());
                os << "</table>\n";
            }
            else
            {
                os << "<h1>No hi ha dades del dia " + myDate + " </h1><br />\n";
            }

            socket->close();

            if (socket->state() == QTcpSocket::UnconnectedState)
            {
                delete socket;
                qDebug() << "Connection closed";
            }
        }
    }
}

void HttpDaemon::discardClient()
{
    QTcpSocket *socket = (QTcpSocket *)sender();

    socket->deleteLater();

    qDebug() << "Connection closed";
}

