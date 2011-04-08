/*-
 * Copyright (c) 2011, Derek Young
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in the
 *        documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.    IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <QtGui/QApplication>
#include <QWaitCondition>
#include <QSettings>
#include <QFile>
#include <QtXml/QXmlDefaultHandler>
#include <QtXml/QXmlInputSource>
//#include <QDebug>
#include "mainwindow.h"
#include "dwarfforeman.h"
#include "formanthread.h"

dfsettings settings;
FormanThread formanThread;
QWaitCondition actionNeeded;
std::vector<dfjob *> dfjobs;

class DFJobParser : public QXmlDefaultHandler
{
public:
    bool startDocument()
    {
        //qDebug("Starting document");
        inDFJobs = false;
        return true;
    }

    bool endElement( const QString&, const QString&, const QString &name )
    {
        //qDebug() << "endElement " << name;
        if ( name == "dfjobs" )
        {
            inDFJobs = false;
            return true;
        }

        if ( name == "dfjob" )
        {
            dfjobs.push_back(job);
            return true;
        }

        return true;
    }

    bool startElement( const QString&, const QString&, const QString &name, const QXmlAttributes &attrs )
    {

        //qDebug() << "startElement " << name;
        if ( name == "dfjobs" )
        {
            inDFJobs = true;
            return true;
        }

        if ( name == "dfjob" )
        {
            job = new dfjob;
            job->enabled = false;
            job->target = 0;
            job->count = 0;
            job->pending = 0;
            job->stack = 0;
            for( int i=0; i<attrs.count(); i++ )
            {
                if( attrs.localName( i ) == "name" )
                    job->name = attrs.value( i );
                else if( attrs.localName( i ) == "category" )
                    job->catagory = attrs.value( i );
                else if( attrs.localName( i ) == "type" )
                    job->type = (uint8_t) attrs.value( i ).toShort();
                else if ( attrs.localName( i) == "reaction" )
                    job->reaction = attrs.value ( i ).toStdString();
                else if ( attrs.localName( i ) == "stack" )
                    job->stack = attrs.value( i ).toUInt();
                else if ( attrs.localName( i ) == "materialType" )
                    job->materialType = attrs.value (i).toStdString();
                else if ( attrs.localName( i ) == "inorganic" )
                    job->inorganic = attrs.value (i).toStdString();
                else if ( attrs.localName( i ) == "other" )
                    job->other = attrs.value (i).toStdString();
                else if ( attrs.localName( i ) == "subtype" )
                    job->subtype = attrs.value (i).toStdString();
            }
        }

        if ( name == "item" )
        {
            std::map<std::string, std::string> item;
            for( int i=0; i<attrs.count(); i++ )
            {
                if( attrs.localName( i ) == "type" )
                    item["type"] = attrs.value( i ).toStdString();
                else if( attrs.localName( i ) == "material" )
                    item["material"] = attrs.value( i ).toStdString();
            }
            job->result.push_back(item);
        }

        return true;
    }

    bool fatalError(const QXmlParseException &exception)
    {
        //qDebug() << "Parse error at line " << exception.lineNumber() << " column" << exception.columnNumber() << " " <<
        //        exception.message();
        return false;
    }

private:
    bool inDFJobs;
    dfjob *job;

};

int main(int argc, char *argv[])
{
    settings.seconds=60;
    settings.cutalltrees=false;
    settings.gatherallplants=false;
    settings.logenabled=false;
    settings.buffer=5;

    {
        DFJobParser handler;
        QFile file("dfjobs.xml");
        QXmlInputSource source( &file );
        QXmlSimpleReader reader;
        reader.setContentHandler( &handler );
        reader.setErrorHandler( &handler );
        reader.parse( source );
    }

    QApplication a(argc, argv);
    a.setApplicationName("dwarf-foreman");
    MainWindow w;
    w.show();
    formanThread.start();
    return a.exec();
}
