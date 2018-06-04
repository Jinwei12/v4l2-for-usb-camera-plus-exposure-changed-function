#ifndef COMMON_H
#define COMMON_H

#define BUFFERS_NUM 5
struct _buffers{
    void *start;
    size_t length;
    uint32_t offset;
};

#endif // COMMON_H
