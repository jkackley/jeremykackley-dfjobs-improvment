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

#include <QTableWidget>
#include <QTableWidgetItem>
#include <QString>
#include <QStringBuilder>
#include "statusframe.h"
#include "ui_statusframe.h"
#include "dwarfforeman.h"

extern std::vector<dfjob *> dfjobs;
extern dfsettings settings;

StatusFrame::StatusFrame(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::StatusFrame)
{
    ui->setupUi(this);
    updateStatus();
}

void StatusFrame::updateStatus()
{
    int row = 0;
    for (uint32_t i = 0; i < dfjobs.size(); i++)
    {
        if(dfjobs[i]->enabled) row++;
    }
    if (settings.logenabled) row++;

    ui->tableWidget->setSortingEnabled(false);
    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(row);

    row = 0;
    if (settings.logenabled)
    {
        ui->tableWidget->setItem(row, 0, new QTableWidgetItem("logs"));
        ui->tableWidget->setItem(row, 1, new QTableWidgetItem(QString::number(settings.logcount)));
        ui->tableWidget->setItem(row, 2, new QTableWidgetItem("0"));
        ui->tableWidget->setItem(row, 3, new QTableWidgetItem(QString::number(settings.logpending)));
        ui->tableWidget->setItem(row, 4, new QTableWidgetItem("0"));
        row++;
    }
    for (uint32_t i = 0; i < dfjobs.size(); i++)
    {
        if (!dfjobs[i]->enabled) continue;
        ui->tableWidget->setItem(row, 0, new QTableWidgetItem(dfjobs[i]->name));
        ui->tableWidget->setItem(row, 1, new QTableWidgetItem(QString::number(dfjobs[i]->count)));
        ui->tableWidget->setItem(row, 2, new QTableWidgetItem(QString::number(dfjobs[i]->sourcecount)));
        ui->tableWidget->setItem(row, 3, new QTableWidgetItem(QString::number(dfjobs[i]->pending)));
        ui->tableWidget->setItem(row, 4, new QTableWidgetItem(dfjobs[i]->all ? ("(" % QString::number(dfjobs[i]->target) % ")") : QString::number(dfjobs[i]->target)));
        if(dfjobs[i]->target > 0)
            ui->tableWidget->setItem(row, 5, new QTableWidgetItem(dfjobs[i]->all ? "" : QString::number(dfjobs[i]->count*100/dfjobs[i]->target).append("%")));
        row++;
    }

    ui->tableWidget->setSortingEnabled(true);
}

StatusFrame::~StatusFrame()
{
    delete ui;
}
