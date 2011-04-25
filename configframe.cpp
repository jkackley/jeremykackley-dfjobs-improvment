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

#include <QT>
#include "configframe.h"
#include "ui_configframe.h"
#include "dwarfforeman.h"

extern dfsettings settings;
extern std::vector<dfjob *> dfjobs;

ConfigFrame::ConfigFrame(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::ConfigFrame)
{

    ui->setupUi(this);

    configUpdated();

    connect(ui->logenabled, SIGNAL(stateChanged(int)), this, SLOT(updateLogEnabled(int)));
    connect(ui->cutalltrees, SIGNAL(stateChanged(int)), this, SLOT(updateCutAllTrees(int)));
    connect(ui->gatherallplants, SIGNAL(stateChanged(int)), this, SLOT(updateGatherAllPlants(int)));
    connect(ui->seconds, SIGNAL(valueChanged(int)), this, SLOT(updateSeconds(int)));
    connect(ui->buffer, SIGNAL(valueChanged(int)), this, SLOT(updateBuffer(int)));
    connect(ui->treeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(onItemDoubleClicked(QTreeWidgetItem*,int)));
    connect(ui->treeWidget, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(onItemChanged(QTreeWidgetItem*,int)));

}

void ConfigFrame::onItemDoubleClicked(QTreeWidgetItem * item, int t)
{
    static QTreeWidgetItem *previous;

    if(previous != NULL)
        ui->treeWidget->closePersistentEditor(previous, 2);

    if ((t == 2) && (item->childCount() == 0))
    {
        ui->treeWidget->openPersistentEditor(item,2);
        previous = item;
    }
}

void ConfigFrame::onItemChanged(QTreeWidgetItem * item, int t)
{
    dfjob *job = NULL;
    for (unsigned int i = 0; i < dfjobs.size() ; i++)
    {
        if (dfjobs[i]->name == item->text(0))
        {
            job = dfjobs[i];
            break;
        }
    }

    if (t == 1)
    {
        job->enabled = (item->checkState(1) == Qt::Checked);
    }
    else if (t == 3)
    {
        job->all = (item->checkState(3) == Qt::Checked);
    }
    else if (t == 2)
    {
        const int a = item->text(t).toInt();
        if (a < 0 )
        {
            item->setText(t, "0");
            job->target = 0;
        }
        else
        {
            job->target = a;
        }
        ui->treeWidget->closePersistentEditor(item, 2);
    }

    emit configChanged();
}

void ConfigFrame::configUpdated()
{
    const Qt::CheckState lookup[2] = { Qt::Unchecked, Qt::Checked } ;
    ui->logenabled->setCheckState(lookup[settings.logenabled]);
    ui->cutalltrees->setCheckState(lookup[settings.cutalltrees]);
    ui->gatherallplants->setCheckState(lookup[settings.gatherallplants]);
    ui->seconds->setValue(settings.seconds);
    ui->buffer->setValue(settings.buffer);

    ui->treeWidget->clear();
    std::map<QString,QTreeWidgetItem *> map;
    QTreeWidgetItem *catagory;
    QTreeWidgetItem *item;
    for (unsigned int i = 0; i < dfjobs.size(); i++)
    {
        catagory = map[dfjobs[i]->catagory];
        if (catagory == NULL)
        {
            catagory = new QTreeWidgetItem(ui->treeWidget);
            catagory->setText(0, dfjobs[i]->catagory);
            ui->treeWidget->addTopLevelItem(catagory);
            map[dfjobs[i]->catagory] = catagory;
        }
        item = new QTreeWidgetItem(catagory);
        item->setText(0, dfjobs[i]->name);
        item->setCheckState(1, lookup[dfjobs[i]->enabled]);
        item->setText(2, QString::number(dfjobs[i]->target));
        item->setCheckState(3, lookup[dfjobs[i]->all]);
    }
}

void ConfigFrame::updateSeconds(int seconds)
{
    settings.seconds = seconds;
    emit configChanged();
}

void ConfigFrame::updateBuffer(int buffer)
{
    settings.buffer = buffer;
    emit configChanged();
}

void ConfigFrame::updateLogEnabled(int state)
{
    settings.logenabled = (state == Qt::Checked);
    emit configChanged();
}

void ConfigFrame::updateCutAllTrees(int state)
{
    settings.cutalltrees = (state == Qt::Checked);
    emit configChanged();
}

void ConfigFrame::updateGatherAllPlants(int state)
{
    settings.gatherallplants = (state == Qt::Checked);
    emit configChanged();
}

ConfigFrame::~ConfigFrame()
{
    delete ui;
}
