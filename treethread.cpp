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

//#include <QString>
//#include <QFutureSynchronizer>
//#include <QtConcurrentRun>
//#include <QDebug>
//#include <math.h>
//#include <algorithm>
#include "treethread.h"
//#include "dwarfforeman.h"

using namespace std;

//extern dfsettings settings;
//extern dfitem dfitems[];

//extern uint32_t x_max,y_max,z_max, tx_max, ty_max;

//vector<cords *> busy;

void TreeThread::run() {
    // When a tree is being cut, it isn't designated. If you redesignate it, it throws an annoying invalid dig error.
    //uint32_t cvector = meminfo->getAddress ("creature_vector");
    //uint32_t race_offset = meminfo->getOffset ("creature_race");
    //uint32_t job_type_offset = meminfo->getOffset("job_type");
    //uint32_t current_job_offset = meminfo->getOffset("creature_current_job");

    //DFHack::t_creature creature;
    /*
    DFHack::DfVector <uint32_t> p_cre (p, cvector);
    for(uint32_t i = 0; i < p_cre.size(); i++)
    {  */
        /*
        creature.race = p->readDWord(p_cre[i] + race_offset);
        creature.current_job.occupationPtr = p->readDWord(p_cre[i] + current_job_offset);
        if(creature.current_job.occupationPtr > 0)
        {
            creature.current_job.active = true;
            creature.current_job.jobType = p->readByte (creature.current_job.occupationPtr + job_type_offset);
        }
        else
        {
            creature.current_job.active = false;
            creature.current_job.jobType = 0;
        }*/ /*
        uint32_t occupation =  p->readDWord(p_cre[i] + current_job_offset);
        if ((p->readDWord(p_cre[i] + race_offset) == 200) && (occupation))
        {
            uint8_t jobType = p->readByte (occupation + job_type_offset);
            if ((jobType == 9) || (jobType == 10))
            {
                cords *cord = new cords;
                cord->x = p->readWord(occupation+16);
                cord->y = p->readWord(occupation+18);
                cord->z = p->readWord(occupation+20);
                busy.push_back(cord);
            }

        } */
        /*
        if((creature.race == 200) && creature.current_job.active && ((creature.current_job.jobType == 9) || (creature.current_job.jobType == 10)))
        {
            cords *hrm = new cords;
            hrm->x = p->readWord(creature.current_job.occupationPtr+16);
            hrm->y = p->readWord(creature.current_job.occupationPtr+18);
            hrm->z = p->readWord(creature.current_job.occupationPtr+20);
            busy.push_back(hrm);
        } */ /*
    }

    settings.logpending = 0;
    uint32_t numTrees;
    v->Start(numTrees);
    //////////////
    QFutureSynchronizer<void> synchronizer;

    const int MAXS = 600;

    for (uint32_t i = 0 ; i < numTrees; i += MAXS)
        synchronizer.addFuture(QtConcurrent::run(this,&TreeThread::cutTree,i,((i+MAXS)<numTrees) ? (i+MAXS) : numTrees));

    synchronizer.waitForFinished();
    //////////////////
    v->Finish();
    for (uint32_t i = 0; i < busy.size(); i++)
    {
        delete busy[i];
    }
    busy.clear();
    */
}

void TreeThread::cutTree(uint32_t start, uint32_t end)
{
    /*
    //uint32_t desOff = meminfo->getOffset ("map_data_designation");
    //uint32_t tileOff = meminfo->getOffset ("map_data_type");
    vector<uint32_t> dirty;
    uint32_t pending = 0;

    for (uint32_t i = start; i < end; i++)
    {
        DFHack::t_tree tree;
        v->Read(i,tree);
        // make sure we are not wasting our time
        if ((tree.type == 0 || tree.type == 1) && !settings.logenabled) continue;
        if ((tree.type == 2 || tree.type == 3) && !settings.gatherallplants) continue;
        // Make sure tree isn't on the boundary. why?
        //if(tree.x == 0 || tree.x == tx_max - 1 || tree.y == 0 || tree.y == ty_max - 1) continue;
        uint32_t blockPtr = Maps->getBlockPtr(floor(tree.x/16),floor(tree.y/16),tree.z);
        qDebug(QString::number(blockPtr,16).toAscii());
        uint32_t designation = p->readDWord(blockPtr + desOff + ((((tree.x%16) * 16) + (tree.y%16))*4));
        // Make sure it isn't hiden.
        if ((designation & 512) == 512)
        {   // Clean up the mess if the stupid fucking user tried this with reveal on.
            if((designation & 16) == 16)
            {
                designation = (designation & ~16) | (-false & 16);
                p->writeDWord(blockPtr + desOff + ((((tree.x%16) * 16) + (tree.y%16))*4), designation);
                dirty.push_back(blockPtr);
            }
            continue;
        }
        // Make sure it is not already designated.
        if ((designation & 16) == 16)
        {
            if (tree.type == 0 || tree.type == 1)
                pending++;
            continue;
        }
        // Don't waste time
        if ((tree.type == 0 || tree.type == 1) && !settings.cutalltrees) continue;
        //Make sure it isn't a sapling... (or dead)
        uint16_t tileType = p->readWord(blockPtr + tileOff + ((((tree.x%16) * 16) + (tree.y%16))*2));
        if((tileType != 24) && (tileType != 34)) continue;
        // Make sure none of the dwarfs are about to cut this down
        int abort = 0;
        for (uint32_t i = 0; i < busy.size(); i++)
        {
            if ((busy[i]->x == tree.x) && (busy[i]->y == tree.y) && (busy[i]->z == tree.z))
            {
                abort = 1;
                break;
            }
        } if (abort == 1) continue;

        // Chop chop chop, suck it elves!
        designation = (designation & ~16) | (-true & 16);
        p->writeDWord(blockPtr + desOff + ((((tree.x%16) * 16) + (tree.y%16))*4), designation);
        dirty.push_back(blockPtr);
        if (tree.type == 0 || tree.type == 1) pending++;
    }

    settings.logpending += pending;

    std::sort(dirty.begin(),dirty.end());
    dirty.erase(std::unique(dirty.begin(), dirty.end()), dirty.end());
    for (uint32_t i = 0; i < dirty.size(); i++)
    {
        uint32_t addr_of_struct = p->readDWord(dirty[i]);
        uint32_t dirtydword = p->readDWord(addr_of_struct);
        dirtydword &= 0xFFFFFFFE;
        dirtydword |= (uint32_t) true;
        p->writeDWord (addr_of_struct, dirtydword);
    }
    */
}
