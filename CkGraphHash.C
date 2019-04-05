
#include "CkGraphHash.h"
#include <algorithm>
#include <exception>

#define GRAPH_TEST_MODE_ 1

const static unsigned int doublingPrimes[] = {
3,
7,
17,
37,
73,
157,
307,
617,
1217,
2417,
4817,
9677,
20117,
40177,
80177,
160117,
320107,
640007,
1280107,
2560171,
5120117,
10000079,
20000077,
40000217,
80000111,
160000177,
320000171,
640000171,
1280000017,
2560000217u,
4200000071u
};

//This routine returns an arbitrary prime larger than x
static unsigned int primeLargerThan(unsigned int x)
{
	int i=0;
	while (doublingPrimes[i]<=x) i++;
	return doublingPrimes[i];
}

static inline int i_abs(int c) { return c>0?c:-c; }

// assume integer is 32 bits
inline static int ObjKey(const LDObjid &oid, const int hashSize) {
  // make sure all positive
  return (((i_abs(oid.id[2]) & 0x7F)<<24)
	 |((i_abs(oid.id[1]) & 0xFF)<<16)
	 |i_abs(oid.id[0])) % hashSize;
}

using ckutil::HashMap;
void HashMap::init() {
    hash_size = st->n_objs*2;
    hash_size = primeLargerThan(hash_size);
    the_map = std::vector<int>(hash_size);
    std::fill(the_map.begin(), the_map.end(), -1);

    for (int i = 0; i < st->n_objs; ++i) {
        const LDObjid &oid = st->objData[i].objID();
        int hash = ObjKey(oid, hash_size);
        #ifndef GRAPH_TEST_MODE_
        CmiAssert(hash != -1);
        #endif
        #ifdef GRAPH_TEST_MODE_
        if (hash == -1) { throw std::out_of_range; }
        #endif
        while (the_map[hash] != -1) {
            hash = (hash+1)%hash_size;
        }
        the_map[hash] = i;
    }
}

int HashMap::getHash(const LDObjKey &objKey) {
    const auto& oid = objKey.objID();
    const auto& mid = objKey.omID();
    return getHash(oid, mid);
}

int HashMap::getHash(const LDObjid &oid, const LDOMid &mid) {
#if CMK_LBDB_ON
    #ifndef GRAPH_TEST_MODE_
    CmiAssert(hash > 0);
    #endif
    #ifdef GRAPH_TEST_MODE_
    if (hash <= 0) { throw std::out_of_range; }
    #endif

    int hash = ObjKey(oid, hash_size);
    for (int id = 0; id < hash_size; ++id) {
        int index = (id + hash)%hash_size;
        if (index == -1 || the_map[index] == -1) {
            return -1;
        }
        if (LDObjIDEqual(st->objData[the_map[index]].objID(), oid) &&
            LDOMidEqual(st->objData[the_map[index]].omID(), mid)) {

            return the_map[index];
        }
    }
#endif
    return -1; // Not found
}

int HashMap::getSendHash(const LDCommData &cData) {
    return getHash(cData.sender);
}

int HashMap::getRecvHash(const LDCommData &cData) {
    return getHash(cData.receiver.get_destObj());
}
