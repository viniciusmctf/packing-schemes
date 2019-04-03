
#include "../../../charm/tmp/BaseLB.h"

namespace ckutil {
    class HashMap {
    public:
        HashMap() = default;
        HashMap(int sz) : hash_size(sz) { the_map.reserve(sz); }
        ~HashMap();
        void init(const BaseLB::stats* const st);
        int getHash(const LDObjKey &);
        int getHash(const LDObjid &oid, const LDOMid &mid);
        int getSendHash(LDCommData &cData);
        int getRecvHash(LDCommData &cData);

    private:
        int hash_size;
        std::vector<int> the_map = {};
    };
}
