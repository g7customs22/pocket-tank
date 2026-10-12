#ifndef SLOT_JOURNAL_H
#define SLOT_JOURNAL_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#define SJ_SLOTS 3
#define SJ_BANK 8192u
#define SJ_META 49152u
#define SJ_UNDO 57344u
#define SJ_BYTES 106496u
#define SJ_PAYLOAD_MAX (SJ_BANK-32u)
typedef struct {
    void *ctx;
    bool (*read)(void *,uint32_t,void *,size_t);
    bool (*write)(void *,uint32_t,const void *,size_t);
    bool (*erase)(void *,uint32_t,size_t);
} sj_io_t;
typedef struct { uint32_t active, present; } sj_meta_t;
typedef struct { sj_io_t io; sj_meta_t meta; uint32_t schema; unsigned char *scratch; size_t capacity; } sj_t;
bool sj_mount(sj_t *,sj_io_t,uint32_t schema,void *scratch,size_t capacity);
bool sj_load(sj_t *,unsigned slot,void *data,size_t len);
bool sj_save(sj_t *,unsigned slot,const void *data,size_t len);
bool sj_activate(sj_t *,unsigned slot);
bool sj_register(sj_t *,unsigned slot);
bool sj_undo_save(sj_t *,unsigned slot,const void *,size_t);
bool sj_undo_load(sj_t *,unsigned slot,void *,size_t);
#endif
