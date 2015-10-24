/* 
 * File:   nextTTX.h
 * Author: goebel
 *
 * Created on 9. April 2015, 09:58
 */

#ifndef NEXTTTX_H
#define	NEXTTTX_H
#include <stdint.h>

class nextTTX {
public:
    int64_t nextTtx_;
    int64_t nextTtxBasedOnTtx_;
    uint32_t nextTtxBasedOnChirpnr_;
    uint32_t nextTtxBasedOnPacketnr_;

    nextTTX(int64_t nextTtx_, int64_t nextTtxBasedOnTtx_, uint32_t nextTtxBasedOnChirpnr_, uint32_t nextTtxBasedOnPacketnr_) :
    nextTtx_(nextTtx_), nextTtxBasedOnTtx_(nextTtxBasedOnTtx_), nextTtxBasedOnChirpnr_(nextTtxBasedOnChirpnr_), nextTtxBasedOnPacketnr_(nextTtxBasedOnPacketnr_) {
    }

    nextTTX();
    nextTTX(const nextTTX& orig);
    virtual ~nextTTX();
private:

};

#endif	/* NEXTTTX_H */

