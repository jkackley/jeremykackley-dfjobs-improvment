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

#include <QMutex>
#include <QWaitCondition>
#include <QString>
#include <windows.h>
#include <tlhelp32.h>
#include <vector>
#include <algorithm>
#include <QStringBuilder>
#include "dwarfforeman.h"
#include "formanthread.h"

using namespace std;

extern QWaitCondition actionNeeded;
extern dfsettings settings;
extern std::vector<dfjob *> dfjobs;

QMutex mutex;

std::vector<uint32_t> usedMemory;
typedef map<std::string, map<std::string, uint32_t> > itemMap;
itemMap itemCount;
map<uint16_t, int> pendingCount;

HANDLE hDF;

uint32_t queuePointer = 0, itemPointer = 0, inorganicPointer = 0, otherPointer = 0, organicAllPointer = 0,
    organicPlantPointer = 0, organicTreePointer = 0, creatureTypePointer = 0, reactionPointer = 0;
vector<string> inorganicMaterials, organicMaterials, otherMaterials, creatureTypes, reactionTypes;

void FormanThread::run()
{
    emit actionStatus("Connecting..");

    if(!attach()) return;

    while(true)
    {
        countItems(false);
        settings.logcount = itemCount["wood"]["all"];

        for (unsigned int i = 0; i < dfjobs.size() ; i++ )
        {
            //qDebug() << dfjobs[i]->name;
            dfjobs[i]->count = 0;
            for (unsigned int j = 0; j < dfjobs[i]->result.size() ; j++)
            {
                dfjobs[i]->count += itemCount[dfjobs[i]->result[j]["type"]][dfjobs[i]->result[j]["material"]];
                //qDebug() << dfjobs[i]->result[j]["type"].c_str();
                //std::map<std::string, std::string> k;
                //k =  dfjobs[i]->result[j];
                //qDebug() << "[" << k["type"].c_str() << "] [" << k["material"].c_str() << "]";
            }
        }

        countPending();

        for (unsigned int i = 0; i < dfjobs.size() ; i++ )
        {
            if (!dfjobs[i]->enabled) continue;
            insertOrder(dfjobs[i]);
            cullOrder(dfjobs[i]);
        }

        emit actionDone();
        mutex.lock();
        actionNeeded.wait(&mutex);
        mutex.unlock();
    }


//    // Millable Products
//    dfitems[ITEM_DYE].pending = 0;
//    dfitems[ITEM_FLOUR].pending = 0;
//    dfitems[ITEM_SUGAR].pending = 0;
//    const uint16_t jobType = 0x006B;
//    const uint8_t data[54] = ITEM_DYE_DATA;
//    dfitems[ITEM_DYE].count = itemCount[69][-1][420][15];
//    dfitems[ITEM_DYE].count += itemCount[69][-1][420][16];
//    dfitems[ITEM_DYE].count += itemCount[69][-1][420][17];
//    dfitems[ITEM_DYE].count += itemCount[69][-1][421][18];
//    dfitems[ITEM_FLOUR].count = itemCount[69][-1][421][2];
//    dfitems[ITEM_FLOUR].count += itemCount[69][-1][421][10];
//    dfitems[ITEM_FLOUR].count += itemCount[69][-1][421][20];
//    dfitems[ITEM_SUGAR].count = itemCount[69][-1][421][3];

//    uint16_t amount = 0;
//    if (dfitems[ITEM_DYE].enabled && (dfitems[ITEM_DYE].count < dfitems[ITEM_DYE].max))
//    {
//        dfitems[ITEM_DYE].pending = dfitems[ITEM_DYE].max - dfitems[ITEM_DYE].count;
//        amount += dfitems[ITEM_DYE].max - dfitems[ITEM_DYE].count;
//    }
//    if (dfitems[ITEM_SUGAR].enabled && (dfitems[ITEM_SUGAR].count < dfitems[ITEM_SUGAR].max))
//    {
//        dfitems[ITEM_SUGAR].pending = dfitems[ITEM_SUGAR].max - dfitems[ITEM_SUGAR].count;
//        amount += dfitems[ITEM_SUGAR].max - dfitems[ITEM_SUGAR].count;
//    }
//    if (dfitems[ITEM_FLOUR].enabled && (dfitems[ITEM_FLOUR].count < dfitems[ITEM_FLOUR].max))
//    {
//        dfitems[ITEM_FLOUR].pending = dfitems[ITEM_FLOUR].max - dfitems[ITEM_FLOUR].count;
//        amount += dfitems[ITEM_FLOUR].max - dfitems[ITEM_FLOUR].count;
//    }

//    pendingCount[jobType] = pendingCount[jobType] + insertOrder(jobType, data, amount, 0, pendingCount[jobType]);
//    cullOrder(jobType, data, amount, 0, pendingCount[jobType]);

//    //forbid millable items not part of the group
//    if (dfitems[ITEM_DYE].enabled || dfitems[ITEM_SUGAR].enabled || dfitems[ITEM_FLOUR].enabled)
//    {
//        if ((dfitems[ITEM_DYE].pending > 0) || (dfitems[ITEM_SUGAR].pending > 0) || (dfitems[ITEM_FLOUR].pending > 0))
//        {
//            countItems(true);
//        }
//    }
}

bool FormanThread::compareJob(const dfjob *job, const uint32_t jobptr)
{
    uint8_t data[56];
    ReadProcessMemory(hDF, (void *)jobptr, (void *) &data, 56, 0);

    const uint8_t type = data[0];
    //const int16_t subtype3 = *((int16_t *)(data+0x02));
    //const int16_t subtype2 = *((int16_t *)(data+0x04));
    const int16_t subtype1 = *((int16_t *)(data+0x24));
    //const int16_t matid = *((int16_t *)(data+0x28));
    //const int16_t gembitmask = *((int16_t *)(data+0x2C));
    //const int16_t memorial = *((int16_t *)(data+0x30));
    //const int16_t bitmask = *((int16_t *)(data+34));
    //const int16_t left = *((int16_t *)(data+0x38));
    //const int16_t total = *((int16_t *)(data+0x3A));

    if (type == 0xD5)
    {
        std::string text = readSTLString(jobptr+8);
        if (job->reaction.empty() || text.empty()) return false;
        if (job->reaction == text) return true;
        return false;
    }
    if (job->type == type)
    {
        if (job->materialType.empty()) return true;
        if ((job->materialType == "bone") &&
                (data[4] == 0x00) && (data[5] == 0x00) && (data[52] == 0x20))
            return true;
        if ((job->materialType == "wood") && (data[52] == 0x02))
            return true;
        if ((job->materialType == "other") && ((int16_t)otherMaterials.size() > subtype1) &&
                (otherMaterials[subtype1] == job->other))
            return true;
        if ((job->materialType == "inorganic") && ((int16_t)inorganicMaterials.size() > subtype1) &&
                (inorganicMaterials[subtype1] == job->inorganic))
            return true;
    }

    return false;
}

void FormanThread::cullOrder(dfjob *job)
{
    if (job->pending == 0) return;
    if ((job->count + job->pending) <= job->target) return;
    if (((job->count + job->pending) - job->target)  > 10000) return;
    uint32_t amount = (job->count + job->pending) - job->target;
    if (amount > job->pending) amount = job->pending;
    if (amount <= settings.buffer) return;
    amount -= settings.buffer;
    //actionLog(QString::number(amount));
    amount = amount / job->stack;
    if (!amount) return;

    uint32_t queuebase = readDWord(queuePointer);
    uint32_t queuepos = readDWord(queuePointer+4);

    uint32_t a = amount;
    for (int jobs = (queuepos-queuebase)/4; jobs > 0; jobs--)
    {

        queuebase = readDWord(queuePointer);
        queuepos = readDWord(queuePointer+4);
        if (a == 0)
        {
            job->pending -= amount;
            return;
        }

        uint32_t jobptr = readDWord(queuebase + (jobs * 4)-4);

        if (compareJob(job, jobptr))
        {
            const uint16_t remaining = readWord(jobptr+0x38);
            if (remaining > a)
            {
                writeWord(jobptr+0x38,remaining-a);
                writeWord(jobptr+0x3a,remaining-a);
                job->pending -= (amount * job->stack);
                return;
            }

            //actionLog("deleting the sucker");
            a -= remaining;
            int b = ((queuepos-queuebase)/4) - jobs + 1;

            uint8_t data[b*4];
            ReadProcessMemory(hDF, (void *)(queuebase + (jobs * 4)), (void *)&data, b * 4, 0);
            WriteProcessMemory(hDF,(void *)(queuebase + (jobs *4) -4) , (void *)&data, b * 4, 0);
            writeDWord(queuePointer+4, readDWord(queuePointer+4) - 4);
        }
    }
    job->pending -= amount;
    return;
}

void FormanThread::insertOrder(dfjob *job)
{

    if ((job->count + job->pending) >= job->target) return;
    if ((job->target - (job->count + job->pending) > 10000)) return;
    uint16_t amount = job->target - (job->count + job->pending);
    amount += settings.buffer;
    amount = amount / job->stack;
    if (!amount) return;

    if(job->pending > 0)
    {
        uint32_t queuebase = readDWord(queuePointer);
        uint32_t queuepos = readDWord(queuePointer+4);

        for (int jobs = (queuepos-queuebase)/4; jobs > 0; jobs--)
        {
            uint32_t jobptr = readDWord(queuebase + (jobs * 4)-4);
            if (compareJob(job, jobptr))
            {
                const uint16_t remaining = readWord(jobptr+0x38) + amount;
                writeWord(jobptr+0x38,remaining);
                writeWord(jobptr+0x3a,remaining);
                job->pending += (amount * job->stack);
                return;
            }
        }
    }

    //Find free memory
    uint32_t freeSpot = readDWord(queuePointer) + 0x3000;
    sort(usedMemory.begin(), usedMemory.end());
    for (uint32_t i = 0; i < usedMemory.size(); i++) //
    {
        if(usedMemory[i] < freeSpot) continue;
        if(usedMemory[i] == freeSpot) { freeSpot += 64; continue; }
        if(usedMemory[i] > freeSpot) break;
    }
    usedMemory.push_back(freeSpot);

    uint8_t data[64] = { 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x6e, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x72, 0x6f, 0x77, 0x20, 0xff, 0xff,
                         0x75, 0x72, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
                         0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00 };

    if (!job->reaction.empty())
    {
        data[0] = 0xD5;
        data[0x18] = job->reaction.size();

        if (job->reaction.size() > 16)
        {
            memcpy (data+8, &job->reactionPtr, 4);
            data[0x1c] = 0x1F;
        }
        else
        {
            const char *blah = job->reaction.c_str();
            memcpy (data+8, blah, job->reaction.size());
            data[0x1c] = 0x0F;
        }
    }
    else
    {
        data[0] = job->type;
        if (job->materialType == "bone")
        {
            data[4] = 0x00;
            data[5] = 0x00;
            data[52] = 0x20;
        }
        if (job->materialType == "wood")
        {
            data[52] = 0x02;
        }
        if (job->materialType == "other")
        {
            for (unsigned short i = 0; i <= otherMaterials.size(); i++)
            {
                if (i == otherMaterials.size())
                {
                    actionLog("invalid material in dfjobs.xml");
                    return;
                }
                if (otherMaterials[i] == job->other)
                {
                    memcpy(data+36, &i, 2);
                    break;
                }
            }
        }
        if (job->materialType == "inorganic")
        {
            for (unsigned int i = 0; i <= inorganicMaterials.size(); i++)
            {
                if (i == inorganicMaterials.size())
                {
                    actionLog("invalid material in dfjobs.xml");
                    return;
                }
                if (inorganicMaterials[i] == job->inorganic)
                {
                    data[36] = 0x00;
                    data[37] = 0x00;
                    memcpy(data+40, &i, 4);
                    break;
                }
            }
        }
    }

    if (job->subtype == "bolt") memset (data+4, 0, 2); // UGLY HACK JUST FOR NOW

    memcpy (data+56, &amount, 2);
    memcpy (data+58, &amount, 2);
    WriteProcessMemory(hDF, (void *) freeSpot, (void *) &data, 64, 0);
    writeDWord(readDWord(queuePointer+4), freeSpot);
    writeDWord(queuePointer+4, readDWord(queuePointer+4)+4);
    job->pending += (amount * job->stack);
    return;
}

void FormanThread::countPending()
{
    uint32_t queuebase = readDWord(queuePointer);
    uint32_t queuepos = readDWord(queuePointer+4);

    for (unsigned int i = 0; i < dfjobs.size() ; i++)
    {
        dfjobs[i]->pending = 0;
    }

    usedMemory.clear();
    pendingCount.clear();
    for (int jobs = (queuepos-queuebase)/4; jobs > 0; jobs--)
    {
        uint32_t job = readDWord(queuebase + (jobs * 4)-4);
        usedMemory.push_back(job);

        for (unsigned int z = 0; z < dfjobs.size(); z++)
        {
            if (compareJob(dfjobs[z], job))
            {
                dfjobs[z]->pending += (readWord(job+0x38)  * dfjobs[z]->stack);
                break;
            }
        }
    }
}

void FormanThread::countItems(bool forbid)
{
    if(!forbid) itemCount.clear();

    const uint32_t itembase = readDWord(itemPointer);
    const uint32_t itempos = readDWord(itemPointer+4);
    const uint32_t itemSize = itempos-itembase;
    const uint32_t numItems = itemSize >> 2;

    const uint32_t * __restrict items = (uint32_t *) malloc(itemSize);
    ReadProcessMemory(hDF, (void *) itembase, (void *) items, itemSize, 0);

    char className[256] = { 0 };
    for (unsigned int i = 0; i < numItems; i++)
    {
        const uint32_t vtable = readDWord(items[i]);
        const uint32_t type = readWord(readDWord(vtable)) >> 8;
        const uint32_t matoff = readDWord(readDWord(vtable+8)) >> 24 ;
        const int16_t typeC = readWord(items[i]+matoff);
        const int16_t typeD = readWord(items[i]+matoff + (matoff % 4 ? 2:4));
        const unsigned int quantity = readByte(items[i]+0x58);

        // Time to cull the list...
        uint8_t statusFlags[4];
        ReadProcessMemory(hDF,(void *)(items[i]+0xC),statusFlags,4,0);
        if((statusFlags[1] == 0xC0) || // Caravan
                (statusFlags[2] > 0) || // Marked forbidden or dumped
                (statusFlags[0] == 2) || // Assigned to a task
                ((statusFlags[1] & 4) == 4) || // A wall or floor or something
                (statusFlags[0] == 32))  // Part of a building
            continue;


        className[0] = '\0';
        ReadProcessMemory(hDF, (void *)(readDWord(readDWord(vtable-0x4)+0xC)+0xC), (void *)&className, 255, 0);
        className[255] = '\0';
        if (strlen(className) > 4) className[strlen(className)-4] = '\0';
        strcpy(className, className+5);
        if (strcmp(className,"corpse") == 0) continue; // corpses are weird
        if (strcmp(className,"remains") == 0) continue; // so are remains lol

        if ((strcmp(className,"bin") == 0) || (strcmp(className,"barrel") == 0))
        {
            const uint32_t csize = (readDWord(items[i] + 0x2c) - readDWord(items[i] + 0x28))/sizeof(uint32_t);
            if (csize > 1) continue;
            if ((csize == 1) && (statusFlags[0] != 0)) continue;
        }

        std::string subtype;
        switch (type)
        {
        case 13: // instruments
        case 14: // toys
        case 24: // weapons
        case 25: // armor
        case 26: // shoes
        case 27: // shields
        case 28: // helms
        case 29: // gloves
        case 38: // ammo
        case 59: // pants
        case 64: // siege ammo
        case 67: // traps
            subtype = readSTLString(readDWord(items[i]+0xA0)+0x24);
            break;
        case 71: // food
            subtype = readSTLString(readDWord(items[i]+0x90)+0x24);
            break;
        }

        string material, materialType, itemName(className);

        if (typeC == 0)
        {
            if ((typeD < 0) || (typeD >= (int16_t)inorganicMaterials.size())) { material = "unknown"; materialType = "unknown"; }
            else { material = inorganicMaterials[typeD]; materialType = "inorganic"; }
        }
        else if ((typeC >= 419) && (typeC <= 618))
        {
            if ((typeD < 0) || (typeD >= (int16_t)organicMaterials.size())) { material = "unknown"; materialType = "unknown"; }
            else { material = organicMaterials[typeD]; materialType = "organic"; }
        }
        else if ((typeC < 19) && (typeC > 0))
        {
            if ((typeC < 0) || (typeC >= (int16_t)otherMaterials.size())) { material = "unknown"; materialType = "unknown"; }
            else { material = otherMaterials[typeC]; materialType = "other"; }
        }
        else if ((typeD == 1) && (typeC >= 0))
        {
            if (typeC >= (int16_t)creatureTypes.size()) { material = "unknown"; materialType = "unknown"; }
            else { material = creatureTypes[typeC]; materialType = "creature"; }
        }
        else if (typeD >= 0)
        {
            if (typeD >= (int16_t)creatureTypes.size()) { material = "unknown"; materialType = "unknown"; }
            else { material = creatureTypes[typeD]; materialType = "creature"; }
        }
        else
        {
            material = "unknown";
            materialType = "unknown";
        }

        itemCount[itemName][material] += quantity;
        itemCount[itemName][materialType] += quantity;
        itemCount[itemName]["all"] += quantity;

        if(!subtype.empty())
        {
            itemCount[subtype][material] += quantity;
            itemCount[subtype][materialType] += quantity;
            itemCount[subtype]["all"] += quantity;
        }

    }
    free((uint32_t *) items);


//    // unforbid all millable plants
//    if (!forbid)
//    {
//        uint8_t statusFlags[4];
//        DF->ReadRaw(p_items[i]+0xC,4,statusFlags);
//        if (statusFlags[1] == 0xC0) continue; // caravan
//
//        if ((type == 53) && (typeB== -1) && (typeC == 419) &&
//                ((typeD == 2)||(typeD == 3)||(typeD == 10)||(typeD == 15)||(typeD == 16)||(typeD == 17)||(typeD == 18)||(typeD == 20)))
//        {
//            //actionLog("trying to clear forbidden status");
//            p->writeByte(p_items[i]+0xE, 0x00);
//        }
//        itemCount[className][matDesc] += p->readWord(p_items[i]+0x68);
//        itemCount[className]["all"] += p->readWord(p_items[i]+0x68);
//        if ((typec > 419) && (typec < 618))
//            itemCount[className]["organic"] += p->readWord(p_items[i]+0x68);
//    }

//    if (forbid)
//    {
//        if ((dfitems[ITEM_DYE].pending==0) && (type == 53) && (typeB== -1) && (typeC == 419) &&
//                ((typeD == 15)||(typeD == 16)||(typeD == 17)||(typeD == 18)))
//        {
//            p->writeByte(p_items[i]+0xE,8);
//        }
//        if ((dfitems[ITEM_FLOUR].pending==0) && (type == 53) && (typeB== -1) && (typeC == 419) &&
//                ((typeD == 2)||(typeD == 10)||(typeD == 20)))
//        {
//            p->writeByte(p_items[i]+0xE,8);
//        }
//        if ((dfitems[ITEM_SUGAR].pending==0) && (type == 53) && (typeB== -1) && (typeC == 419) && (typeD == 3))
//        {
//            p->writeByte(p_items[i]+0xE,8);
//        }
//    }
}


bool FormanThread::attach()
{
    uint32_t dfsize = 0;
    uint32_t dfbase = 0;

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof (PROCESSENTRY32);
    const HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (Process32First(hProcessSnap, &pe32))
    {
        do
        {
            if (wcsicmp(TEXT("Dwarf Fortress.exe"), pe32.szExeFile) == 0)
            {
                MODULEENTRY32 me32;
                me32.dwSize = sizeof(MODULEENTRY32);
                const HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pe32.th32ProcessID);
                if (Module32First(hSnapShot, &me32))
                {
                    do
                    {
                        if (wcsicmp(TEXT("Dwarf Fortress.exe"), me32.szModule) == 0)
                        {
                            dfsize = (uint32_t)me32.modBaseSize;
                            dfbase = (uint32_t)me32.modBaseAddr;
                            hDF = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);
                            CloseHandle(hSnapShot);
                            CloseHandle(hProcessSnap);
                            goto dffound;
                        }
                    } while (Module32Next (hSnapShot, &me32));
                }
                CloseHandle(hSnapShot);
            }
        } while (Process32Next(hProcessSnap, &pe32));
    }

    CloseHandle(hProcessSnap);
    actionPopup("Unable to find Dwarf Fortress.exe!",true);
    return false;

    dffound:
        actionLog(QString("Dwarf Fortress.exe found at 0x" % (QString::number(dfbase,16)) % " with a size of " %
                          QString::number(dfsize) % " bytes."));

    actionStatus("Scanning...");
    const uint8_t * __restrict memory8 = (uint8_t*)malloc(dfsize);
    const uint32_t * __restrict memory32 = (uint32_t*)memory8;
    ReadProcessMemory(hDF, (void *)dfbase, (void *)memory8, dfsize, 0);


    uint32_t *dfvector = 0;
    uint32_t trash = 0;
    char className[256] = {0};
    std:: string tstring;
    uint32_t strsize = 0;
    uint32_t strmode = 0;
    uint32_t size = 0;
    uint32_t deleteOperator = 0;

    for (unsigned int a = 0; a < ((dfsize/4)-100); a++)
    {
        //
        const uint32_t vectorBase = *(memory32+a);
        const uint32_t vectorPos = *(memory32+a+1);
        const uint32_t vectorCap = *(memory32+a+2);
        const uint32_t vectorNull = *(memory32+a+3);

        if (vectorNull != 0) goto vectorsearchdone;
        if (vectorBase == 0) goto vectorsearchdone;
        if (vectorPos == 0) goto vectorsearchdone;
        if (vectorCap == 0) goto vectorsearchdone;

        if ((vectorBase % 4) != 0) goto vectorsearchdone;
        if ((vectorPos % 4) != 0) goto vectorsearchdone;
        if ((vectorCap % 4) != 0) goto vectorsearchdone;

        if (vectorBase > vectorPos) goto vectorsearchdone;
        if (vectorBase > vectorCap) goto vectorsearchdone;
        if (vectorPos > vectorCap) goto vectorsearchdone;

        size = vectorPos - vectorBase;
        if (size > 100000) goto vectorsearchdone;
        if (size == 0) goto vectorsearchdone;

        dfvector = (uint32_t *) malloc(size);
        if(ReadProcessMemory(hDF, (void *)vectorBase, (void *)dfvector, size, 0))
        {
            trash = 0;
            className[0] = '\0';

            for (uint32_t c = 0; c < (size / 4); c++)
            {
                if(!ReadProcessMemory(hDF, (void *)*(dfvector+c), (void *)&trash, 4, 0))
                {
                    free ((uint32_t *)dfvector);
                    goto vectorsearchdone;
                }
            }

            for (uint32_t ipad = 0; ipad < (size / 4); ipad++)
            {
                trash = 0;
                strsize = readDWord(*(dfvector+ipad) + 0x10);
                strmode = readDWord(*(dfvector+ipad) + 0x14);

                if (strsize > 255)
                {
                    free ((uint32_t *)dfvector);
                    goto vectorsearchdone;
                }


                if (strmode == 0x0F)
                {
                    ReadProcessMemory(hDF, (void *)*(dfvector+ipad), (void *)&className, strsize, 0);
                    className[strsize] = '\0';
                }

                if (strmode > 0x10)
                {
                    ReadProcessMemory(hDF, (void *)*(dfvector+ipad), (void *)&trash, 4, 0);
                    ReadProcessMemory(hDF, (void *)trash, (void *)&className, strsize, 0);
                    className[strsize] = '\0';
                }

                if ((inorganicPointer == 0) && (strcmp(className,"IRON") == 0))
                {
                    inorganicPointer = (a*4) + dfbase;
                    actionLog("Inorganic Pointer Found: " % QString::number(inorganicPointer, 16));
                    for (trash = 0; trash < (size / 4); trash++)
                    {
                        tstring = readSTLString(*(dfvector+trash));
                        inorganicMaterials.push_back(tstring);
                    }
                }
                if ((organicAllPointer == 0) && ((strcmp(className,"MEADOW-GRASS") == 0) || (strcmp(className,"MUSHROOM_HELMET_PLUMP") == 0)))
                {
                    organicAllPointer = (a*4) + dfbase;
                    actionLog("Organic All Pointer Found: " % QString::number(organicAllPointer, 16));
                    for (trash = 0; trash < (size / 4); trash++)
                    {
                        tstring = readSTLString(*(dfvector+trash));
                        organicMaterials.push_back(tstring);
                    }
                }
                if ((organicPlantPointer == 0) && (strcmp(className,"MUSHROOM_HELMET_PLUMP") == 0))
                {
                    organicPlantPointer = (a*4) + dfbase;
                    actionLog("Organic Plant Pointer Found: " % QString::number(organicPlantPointer, 16));
                }
                if ((organicTreePointer == 0) && (strcmp(className,"MANGROVE") == 0))
                {
                    organicTreePointer = (a*4) + dfbase;
                    actionLog("Organic Tree Pointer Found: " % QString::number(organicTreePointer, 16));
                }
                if ((creatureTypePointer == 0) && (strcmp(className,"TOAD") == 0))
                {
                    creatureTypePointer = (a*4) + dfbase;
                    actionLog("Creature Type Pointer Found: " % QString::number(creatureTypePointer, 16));
                    for (trash = 0; trash < (size / 4); trash++)
                    {
                        tstring = readSTLString(*(dfvector+trash));
                        creatureTypes.push_back(tstring);
                    }
                }
                if ((reactionPointer == 0) && (strcmp(className,"TAN_A_HIDE") == 0))
                {
                    reactionPointer = (a*4) + dfbase;
                    actionLog("Reaction Pointer Found: " % QString::number(reactionPointer, 16));
                    for (trash = 0; trash < (size / 4); trash++)
                    {
                        tstring = readSTLString(*(dfvector+trash));
                        reactionTypes.push_back(tstring);
                    }
                }
            }
            free ((uint32_t *)dfvector);
        }
        else
        {
            free ((uint32_t *)dfvector);
        }

        vectorsearchdone:


            // not a bug here, the stuff we're looking for is always in the first 1/4th of memory space while vectors can be found later
        if ((memory8[a] == 0x8b)    && (memory8[a+1] == 0x15) && (memory8[a+6] == 0x8b)  && (memory8[a+7] == 0x34) &&
            (memory8[a+8] == 0x8a)  && (memory8[a+9] == 0x85) && (memory8[a+10] == 0xf6) && (memory8[a+11] == 0x74) &&
            (memory8[a+12] == 0x0f) && (memory8[a+13] == 0xe8))
        {
            queuePointer = *((uint32_t*)(memory8+a+2));
            deleteOperator = *((uint32_t*)(memory8+a+21)); // 2fer, lol
            actionLog("Queue Pointer Found: " % QString::number(queuePointer, 16));
        }

        if ((memory8[a] == 0x2b)    && (memory8[a+1] == 0x35) && (memory8[a+6] == 0x57)  && (memory8[a+7] == 0xc1) &&
            (memory8[a+8] == 0xfe)  && (memory8[a+9] == 0x02) && (memory8[a+10] == 0x4e) && (memory8[a+11] == 0x78) &&
            (memory8[a+12] == 0x20) && (memory8[a+13] == 0xbf))
        {
            itemPointer = *((uint32_t*)(memory8+a+2));
            actionLog("Item Pointer Found: " % QString::number(itemPointer, 16));
        }

    }
    free((uint8_t *)memory8);

    if (queuePointer && itemPointer && inorganicPointer && organicAllPointer && organicPlantPointer && organicTreePointer &&
            creatureTypePointer && reactionPointer)
    {
        actionLog("All vectors successfully found!");
    }
    else
    {
        actionPopup("Failure finding all vectors! :(", true);
    }

    otherMaterials.push_back("INORGANIC");
    otherMaterials.push_back("AMBER");
    otherMaterials.push_back("CORAL");
    otherMaterials.push_back("GLASS_GREEN");
    otherMaterials.push_back("GLASS_CLEAR");
    otherMaterials.push_back("GLASS_CRYSTAL");
    otherMaterials.push_back("WATER");
    otherMaterials.push_back("COAL");
    otherMaterials.push_back("POTASH");
    otherMaterials.push_back("ASH");
    otherMaterials.push_back("PEARLASH");
    otherMaterials.push_back("LYE");
    otherMaterials.push_back("MUD");
    otherMaterials.push_back("VOMIT");
    otherMaterials.push_back("SALT");
    otherMaterials.push_back("FILTH_B");
    otherMaterials.push_back("FILTH_Y");
    otherMaterials.push_back("UNKNOWN_SUBSTANCE");
    otherMaterials.push_back("GRIME");

    actionStatus("Prepping..");

    if(readDWord(queuePointer+12) == 0xDEADBEAF)
    {
        actionStatus("DF (already) prepared and ready.");
    }
    else
    {
        const LPVOID lpvResult = VirtualAllocEx(hDF, NULL, 0x20000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if(lpvResult == NULL)
        {
                actionPopup("Allocating memory failed.", true);
                return false;
        }
        actionLog("Allocated memory at " % QString::number((uint32_t)lpvResult, 16));

        const uint32_t queuebase = readDWord(queuePointer);
        const uint32_t queuepos = readDWord(queuePointer+4);
        const uint32_t queuesize = queuepos-queuebase;

        const uint32_t * __restrict queue = (uint32_t *) malloc(queuesize);
        ReadProcessMemory(hDF, (void*)queuebase, (void*) queue, queuesize, 0);
        WriteProcessMemory(hDF, (void*)lpvResult, (void*) queue, queuesize, 0);
        free ((uint32_t *)queue);
        writeDWord(queuePointer, (uint32_t) lpvResult);
        writeDWord(queuePointer+4, ((uint32_t) lpvResult) + queuesize);
        writeDWord(queuePointer+8, ((uint32_t) lpvResult) + 0x0FA0);
        writeDWord(queuePointer+12, 0xDEADBEAF);

        // one last EVIL dead to do. put the free operator at +FB0 and load our own goodie in there.
        uint32_t realfree = readDWord(deleteOperator);
        actionLog("Hooking into the free operator at " % QString::number(realfree,16));
        uint32_t mystuff = (uint32_t) lpvResult;
        writeDWord(mystuff+0xFB0, realfree);
        realfree = mystuff+0xFB0;

        uint8_t in[45];
        in[0]  = 0x55;                                                              // push ebp
        in[1]  = 0x89; in[2]  = 0xE5;                                               // mov ebp, esp
        in[3]  = 0x83; in[4]  = 0xEC; in[5]  = 0x18;                                // sub esp, 18
        in[6]  = 0xB8; memcpy(in+7, &mystuff, 4);                                   // mov eax our memory
        in[11] = 0x39; in[12] = 0x45; in[13] = 0x08;                                // cmp [ebp+08],eax
        in[14] = 0x72; in[15] = 0x0F;                                               // jump +15 if it isn't our memory
        in[16] = 0xB8; memcpy(in+17, &mystuff, 4);                                  // mov eax our memory
        in[21] = 0x05; in[22] = 0x00; in[23] = 0x00; in[24] = 0x02; in[25] = 0x00;  // add eax 0x2000
        in[26] = 0x3B; in[27] = 0x45; in[28] = 0x08;                                // cmp eax, [ebp+08]
        in[29] = 0x73; in[30] = 0x0C;                                               // jae 12 it is our memory
        in[31] = 0x8B; in[32] = 0x45; in[33] = 0x08;                                // mov eax, [ebp+08]
        in[34] = 0x89; in[35] = 0x04; in[36] = 0x24;                                // mov [esp], eax
        in[37] = 0xFF; in[38] = 0x15; memcpy(in+39, &realfree, 4);                  // use the real free operator
        in[43] = 0xC9;                                                              // leave
        in[44] = 0xC3;                                                              // ret

        WriteProcessMemory(hDF, (void *)(mystuff+0xFC0), (void *) in, 45, 0);
        DWORD oldprotect;
        if(VirtualProtectEx(hDF, (LPVOID) (deleteOperator), 1, PAGE_READWRITE, &oldprotect))
        {
            actionLog("VirtualProtectEX == true");
            if(writeDWord(deleteOperator, mystuff+0xFC0))
                actionLog("writeDWord == true");
            else actionLog("writeDWord == false");
            VirtualProtectEx(hDF, (LPVOID) (deleteOperator), 1, oldprotect, &oldprotect);
        }
        else actionLog("VirtualProtectEX == false");

        actionStatus("DF prepared and ready.");
    }

    // setup strings
    uint32_t freeSpot = readDWord(queuePointer) + 0x2000;
    for (unsigned int i = 0; i < dfjobs.size() ; i++ )
    {
        if (dfjobs[i]->reaction.size() > 16)
        {
            dfjobs[i]->reactionPtr = freeSpot;
            writeRaw(freeSpot, dfjobs[i]->reaction.c_str(), dfjobs[i]->reaction.size());
            freeSpot += dfjobs[i]->reaction.size() + 1;
        }
    }

    return true;
}

bool FormanThread::readRaw(const uint32_t address, LPVOID data, SIZE_T size)
{
    return(ReadProcessMemory(hDF, (LPCVOID)address, data, size, (SIZE_T)NULL));
}

bool FormanThread::writeRaw(const uint32_t address, LPCVOID data, SIZE_T size)
{
    return(WriteProcessMemory(hDF, (LPVOID)address, data, size, (SIZE_T)NULL));
}

const uint32_t FormanThread::readDWord(const uint32_t address)
{
    uint32_t dword = 0;
    ReadProcessMemory(hDF, (LPCVOID)address, (LPVOID)&dword, 4, 0);
    return dword;
}

const uint16_t FormanThread::readWord(const uint32_t address)
{
    uint16_t word = 0;
    ReadProcessMemory(hDF, (LPCVOID)address, (LPVOID)&word, (SIZE_T)2, (SIZE_T)NULL);
    return word;
}

const uint8_t FormanThread::readByte(const uint32_t address)
{
    uint8_t byte = 0;
    ReadProcessMemory(hDF, (LPCVOID)address, (LPVOID)&byte, (SIZE_T)1, (SIZE_T)NULL);
    return byte;
}

bool FormanThread::writeDWord(const uint32_t address, const uint32_t data)
{
    return(WriteProcessMemory(hDF, (LPVOID)address, (LPCVOID)&data, (SIZE_T)4, (SIZE_T)NULL));
}

bool FormanThread::writeWord(const uint32_t address, const uint16_t data)
{
    return(WriteProcessMemory(hDF, (LPVOID)address, (LPCVOID)&data, (SIZE_T)2, (SIZE_T)NULL));
}

bool FormanThread::writeByte(const uint32_t address, const uint8_t data)
{
    return(WriteProcessMemory(hDF, (LPVOID)address, (LPCVOID)&data, (SIZE_T)1, (SIZE_T)NULL));
}

const std::string FormanThread::readSTLString(const uint32_t addr)
{
    const uint32_t strsize = readDWord(addr + 0x10);
    const uint32_t strmode = readDWord(addr + 0x14);

    if (strsize > 255) return string();
    char text[strsize+1];

    if (strmode == 0x0F)
    {
        ReadProcessMemory(hDF, (LPCVOID)addr, (LPVOID)text, (SIZE_T)strsize, 0);
    }
    else if (strmode > 0x10)
    {
        ReadProcessMemory(hDF, (LPCVOID)readDWord(addr), (LPVOID)text, (SIZE_T)strsize, (SIZE_T)NULL);
    }
    else
    {
        text[0] = '\0';
    }

    text[strsize] = '\0';
    std::string stlstring(text);
    return stlstring;
}
