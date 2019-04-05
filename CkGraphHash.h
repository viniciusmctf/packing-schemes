/** \file CkGraphHash.h
 *  Author: Vinicius M C T de Freitas
 *  Date Created: April 4th, 2019
 *  E-mail: vinicius.mct.freitas@gmail.com
 *          or vinicius.mctf@posgrad.ufsc.br
 *
 *  Note: This class is, in essence, a reimplementation of the hash table already
 *  existing in BaseLB.h, but decoupled from that class, in order to enable the
 *  build and use of communication information within distributed LBs based on
 *  DistBaseLB.h
 */

/**
 *  \addtogroup CkLdb
 */


#ifndef _CK_HASH_TABLE_REIMPL_H_
#define _CK_HASH_TABLE_REIMPL_H_

#include "../../../charm/tmp/BaseLB.h"

namespace ckutil {
    class HashMap {
    public:
        HashMap() = default;
        HashMap(const BaseLB::stats* const st) : st(st) {}
        ~HashMap();
        void init();
        int getHash(const LDObjKey &);
        int getHash(const LDObjid &oid, const LDOMid &mid);
        int getSendHash(LDCommData &cData);
        int getRecvHash(LDCommData &cData);

    private:
        int hash_size;
        const BaseLB::stats* const st;
        std::vector<int> the_map = {};
    };
}


#endif // _CK_HASH_TABLE_REIMPL_H_
