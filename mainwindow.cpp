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

#include <QTimer>
#include <QWaitCondition>
#include <QMessageBox>
#include <QCoreApplication>
#include <QStringBuilder>
#include <QFileDialog>
#include <QFile>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QtXml/QXmlDefaultHandler>
#include <QDebug>
#include <QStringBuilder>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "statusframe.h"
#include "configframe.h"
#include "logframe.h"
#include "dwarfforeman.h"
#include "formanthread.h"

extern dfsettings settings;
extern std::vector<dfjob *> dfjobs;

LogFrame *logFrame;
ConfigFrame *configFrame;
StatusFrame *statusFrame;
int seconds = settings.seconds;
extern FormanThread formanThread;
extern QWaitCondition actionNeeded;
QTimer *timer;
QString fileName;

class DFConfigParser : public QXmlDefaultHandler
{
public:

    bool startElement( const QString&, const QString&, const QString &name, const QXmlAttributes &attrs )
    {
        if ( name == "foreman" )
        {
            for( int i = 0; i < attrs.count(); i++ )
            {
                if( attrs.localName( i ) == "seconds" )
                    settings.seconds = attrs.value( i ).toUInt();
                else if( attrs.localName( i ) == "buffer" )
                    settings.buffer = attrs.value( i ).toUInt();
                else if( attrs.localName( i ) == "logenabled" )
                    settings.logenabled = (attrs.value( i ) == "true");
                else if( attrs.localName( i ) == "cutalltrees" )
                    settings.cutalltrees = (attrs.value( i ) == "true");
                else if( attrs.localName( i ) == "gatherallplants" )
                    settings.gatherallplants = (attrs.value( i ) == "true");
            }
            return true;
        }

        if ( name == "job" )
        {
            QString name;
            bool enabled = false;
            uint32_t target = 0;

            for ( int i = 0; i < attrs.count(); i++ )
            {
                if ( attrs.localName( i ) == "name" )
                    name = attrs.value( i );
                else if ( attrs.localName( i ) == "target" )
                    target = attrs.value( i ).toUInt();
                else if ( attrs.localName( i ) == "enabled" )
                    enabled = (attrs.value( i ) == "true");
            }

            if ( name.size() > 0 )
            {
                for ( unsigned int i = 0; i < dfjobs.size() ; i++ )
                {
                    if ( dfjobs[i]->name == name )
                    {
                        dfjobs[i]->enabled = enabled;
                        dfjobs[i]->target = target;
                        return true;
                    }
                }
            }
            return true;
        }
        return true;
    }

    bool fatalError(const QXmlParseException &exception)
    {
        qDebug() << "Parse error at line " << exception.lineNumber() << " column" << exception.columnNumber() << " " <<
                exception.message();
        return false;
    }

};


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    statusFrame = new StatusFrame;
    configFrame = new ConfigFrame;
    logFrame = new LogFrame;
    ui->stackedWidget->addWidget(statusFrame);
    ui->stackedWidget->addWidget(configFrame);
    ui->stackedWidget->addWidget(logFrame);
    connect(ui->statusButton,SIGNAL(clicked()),this,SLOT(statusClicked()));
    connect(ui->configButton,SIGNAL(clicked()),this,SLOT(configClicked()));
    connect(ui->logButton,SIGNAL(clicked()), this, SLOT(logClicked()));
    connect(ui->tickButton,SIGNAL(clicked()),this,SLOT(tickClicked()));
    connect(ui->actionSaveAs,SIGNAL(triggered()), this, SLOT(saveAs()));
    connect(ui->actionSave, SIGNAL(triggered()), this, SLOT(save()));
    connect(ui->actionExit, SIGNAL(triggered()), this, SLOT(exit()));
    connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(open()));
    connect(this,SIGNAL(activate(int)),ui->stackedWidget,SLOT(setCurrentIndex(int)));
    connect(configFrame,SIGNAL(configChanged()),statusFrame,SLOT(updateStatus()));
    connect(&formanThread, SIGNAL(actionDone()), statusFrame, SLOT(updateStatus()));
    connect(&formanThread, SIGNAL(actionDone()), this, SLOT(processingDone()));
    connect(&formanThread, SIGNAL(actionStatus(QString)), this, SLOT(updateStatus(QString)));
    connect(&formanThread, SIGNAL(actionPopup(QString, bool)), this, SLOT(showPopup(QString,bool)));
    connect(&formanThread, SIGNAL(actionLog(QString)), logFrame, SLOT(print(QString)));
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(tick()));
}


void MainWindow::open()
{
    QString openFile = QFileDialog::getOpenFileName(this, "Load Settings", "", "Foreman Settings (*.fml)");
    if (openFile == "") return;

    QFile inputFile(openFile);
    if (!QFile::exists(openFile))
    {
        showPopup("Failed to open file.",false);
        return;
    }

    DFConfigParser handler;
    QXmlInputSource source( &inputFile );
    QXmlSimpleReader reader;
    reader.setContentHandler( &handler );
    reader.setErrorHandler( &handler );
    reader.parse( source );

    configFrame->configUpdated();
    statusFrame->updateStatus();

}

void MainWindow::saveAs()
{
    fileName = QFileDialog::getSaveFileName(this, "Save Settings", "", "Foreman Settings (*.fml)");
    if(fileName == "") return;
    save();
}

void MainWindow::save()
{
    if(fileName == "") saveAs();
    if(fileName == "") return;
    QFile outputFile(fileName);
    if (!outputFile.open(QIODevice::WriteOnly))
    {
        showPopup("Failed to save file.", false);
        return;
    }

    QXmlStreamWriter writer(&outputFile);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();

    writer.writeStartElement("foreman");
    writer.writeAttribute("seconds", QString::number(settings.seconds));
    writer.writeAttribute("buffer", QString::number(settings.buffer));
    writer.writeAttribute("logenabled", settings.logenabled ? "true" : "false");
    writer.writeAttribute("cutalltrees", settings.cutalltrees ? "true" : "false");
    writer.writeAttribute("gatherallplants", settings.gatherallplants ? "true" : "false");

    for (unsigned int i = 0; i < dfjobs.size() ; i++)
    {
        writer.writeStartElement("job");
        writer.writeAttribute("name", dfjobs[i]->name);
        writer.writeAttribute("enabled", dfjobs[i]->enabled ? "true" : "false");
        writer.writeAttribute("target", QString::number(dfjobs[i]->target));
        writer.writeEndElement();
    }

    writer.writeEndElement();

    writer.writeEndDocument();
}

void MainWindow::exit()
{
    QCoreApplication::exit(0);
}

void MainWindow::tick()
{
    if(seconds--<1) seconds=settings.seconds;
    if(seconds != 0)
    {
        ui->statusBar->showMessage("Next tick in " % QString::number(seconds) % " seconds.");
    }
    else
    {
        timer->stop();
        ui->statusBar->showMessage("Processing.");
        actionNeeded.wakeAll();
    }
}

void MainWindow::tickClicked()
{
    seconds=1;
    tick();
}

void MainWindow::statusClicked()
{
    emit activate(0);
}

void MainWindow::configClicked()
{
    emit activate(1);
}

void MainWindow::logClicked()
{
    emit activate(2);
}

void MainWindow::updateStatus(QString text)
{
    statusBar()->showMessage(text);
}

void MainWindow::processingDone()
{
    statusBar()->showMessage("Processing... done");
    timer->start(1000);
}

void MainWindow::showPopup(QString text, bool exit)
{
    QMessageBox msgBox;
    msgBox.setText(text);
    msgBox.exec();
    if(exit) QCoreApplication::exit(-1);
}

MainWindow::~MainWindow()
{
    delete ui;
}

