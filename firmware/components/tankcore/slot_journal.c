#include "slot_journal.h"
#include <string.h>
typedef struct { uint32_t magic,schema,key,sequence,length,data_crc,header_crc,reserved; } header_t;
_Static_assert(sizeof(header_t)==32,"journal header layout");
#define MAGIC 0x31544c53u
static uint32_t crc(const void *p,size_t n) {
    const unsigned char *s=p;uint32_t c=~0u;
    while(n--){c^=*s++;for(int k=0;k<8;k++)c=(c>>1)^(0xedb88320u & (0u-(c&1)));}
    return ~c;
}
static uint32_t offset(unsigned key,unsigned bank){return key<3?key*2*SJ_BANK+bank*SJ_BANK:key==3?SJ_META+bank*4096:SJ_UNDO+(key-4)*2*SJ_BANK+bank*SJ_BANK;}
/* Invalid/torn records never replace a verified bank. Read errors fail closed. */
static int read_bank(sj_t *j,unsigned key,unsigned bank,size_t len,header_t *h,void *out){
    uint32_t off=offset(key,bank);
    if(!j->io.read(j->io.ctx,off,h,sizeof *h))return -1;
    bool legacy=j->legacy_schema && h->schema==j->legacy_schema;
    size_t stored_len=legacy&&key!=3?j->legacy_length:len;
    if(h->magic!=MAGIC||(!legacy&&h->schema!=j->schema)||h->key!=key||h->length!=stored_len||h->sequence==0||h->sequence==UINT32_MAX||h->header_crc!=crc(h,24))return 0;
    if(stored_len>len||stored_len>j->capacity)return 0;
    if(!j->io.read(j->io.ctx,off+sizeof *h,out,stored_len))return -1;
    if(crc(out,stored_len)!=h->data_crc)return 0;
    if(legacy&&key!=3 && (!j->upgrade||!j->upgrade(out,stored_len,len)))return 0;
    return 1;
}
static int newest(sj_t *j,unsigned key,size_t len,header_t *best){
    int bank=-1;header_t h;
    for(unsigned b=0;b<2;b++){
        int r=read_bank(j,key,b,len,&h,j->scratch);
        if(r<0)return -2;
        if(r && (bank<0||h.sequence>best->sequence)){*best=h;bank=(int)b;}
    }
    return bank;
}
static bool load(sj_t *j,unsigned key,void *out,size_t len){
    if(len>j->capacity||len>SJ_PAYLOAD_MAX)return false;
    header_t h;int b=newest(j,key,len,&h);
    return b>=0&&read_bank(j,key,(unsigned)b,len,&h,out)==1;
}
static bool save(sj_t *j,unsigned key,const void *data,size_t len){
    if(len>j->capacity||len>SJ_PAYLOAD_MAX)return false;
    header_t old={0};int b=newest(j,key,len,&old);
    if(b==-2||old.sequence>=UINT32_MAX-1)return false;
    unsigned dest=b<0?0:1u-(unsigned)b;uint32_t off=offset(key,dest);
    header_t h={MAGIC,j->schema,key,old.sequence+1,(uint32_t)len,crc(data,len),0,0};
    h.header_crc=crc(&h,24);
    if(!j->io.erase(j->io.ctx,off,key==3?4096:SJ_BANK))return false;
    /* Header last: power loss during payload write leaves the old bank valid. */
    if(!j->io.write(j->io.ctx,off+sizeof h,data,len)||!j->io.write(j->io.ctx,off,&h,sizeof h))return false;
    header_t check;
    return read_bank(j,key,dest,len,&check,j->scratch)==1 && check.sequence==h.sequence && !memcmp(data,j->scratch,len);
}
bool sj_mount_compat(sj_t *j,sj_io_t io,uint32_t schema,uint32_t legacy_schema,size_t legacy_length,bool (*upgrade)(void *,size_t,size_t),void *scratch,size_t capacity){
    memset(j,0,sizeof *j);j->io=io;j->schema=schema;j->scratch=scratch;j->capacity=capacity;j->legacy_schema=legacy_schema;j->legacy_length=legacy_length;j->upgrade=upgrade;
    if(capacity<sizeof(sj_meta_t))return false;
    header_t h;int b=newest(j,3,sizeof j->meta,&h);
    if(b==-2)return false;
    if(b>=0){
        if(!load(j,3,&j->meta,sizeof j->meta))return false;
        return j->meta.active<SJ_SLOTS && !(j->meta.present&~7u) &&
            (!j->meta.present || (j->meta.present&(1u<<j->meta.active)));
    }
    /* No owner marker: claim ONLY an entirely erased reserved range. Never
     * format unknown data, incompatible schema, or a damaged old journal. */
    unsigned char block[128];
    for(uint32_t off=0;off<SJ_BYTES;off+=sizeof block){
        if(!io.read(io.ctx,off,block,sizeof block))return false;
        for(unsigned k=0;k<sizeof block;k++)if(block[k]!=255)return false;
    }
    j->meta.active=0;j->meta.present=0;
    return save(j,3,&j->meta,sizeof j->meta);
}
bool sj_load(sj_t *j,unsigned slot,void *data,size_t len){return slot<SJ_SLOTS&&(j->meta.present&(1u<<slot))&&load(j,slot,data,len);}
bool sj_save(sj_t *j,unsigned slot,const void *data,size_t len){
    if(slot>=SJ_SLOTS||len>j->capacity||len>SJ_PAYLOAD_MAX)return false;
    header_t h;int existing=newest(j,slot,len,&h);
    if(existing==-2||!save(j,slot,data,len))return false;
    /* Seed both banks on first creation, before any active selector commit. */
    return existing>=0||save(j,slot,data,len);
}
bool sj_activate(sj_t *j,unsigned slot){
    if(slot>=SJ_SLOTS)return false;
    sj_meta_t m={slot,j->meta.present|(1u<<slot)};
    if(!save(j,3,&m,sizeof m))return false;
    j->meta=m;return true;
}

bool sj_register(sj_t *j,unsigned slot){
    if(slot>=SJ_SLOTS)return false;
    sj_meta_t m={j->meta.active,j->meta.present|(1u<<slot)};
    if(!save(j,3,&m,sizeof m))return false;
    j->meta=m;return true;
}
bool sj_undo_save(sj_t *j,unsigned slot,const void *data,size_t len){
    if(slot>=SJ_SLOTS)return false;
    header_t h;int prior=newest(j,slot+4,len,&h);
    if(prior==-2||!save(j,slot+4,data,len))return false;
    return prior>=0||save(j,slot+4,data,len);
}
bool sj_undo_load(sj_t *j,unsigned slot,void *data,size_t len){return slot<SJ_SLOTS&&load(j,slot+4,data,len);}

bool sj_mount(sj_t *j,sj_io_t io,uint32_t schema,void *scratch,size_t capacity){return sj_mount_compat(j,io,schema,0,0,NULL,scratch,capacity);}
