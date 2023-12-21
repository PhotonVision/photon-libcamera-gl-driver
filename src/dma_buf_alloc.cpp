#include "dma_buf_alloc.h"

#include <stdexcept>

#include <fcntl.h>
#include <unistd.h>

#include <linux/dma-buf.h>
#include <linux/dma-heap.h>
#include <sys/ioctl.h>

DmaBufAlloc::DmaBufAlloc(const std::string &heap_name) {
    int heap_fd = open(heap_name.c_str(), O_RDWR, 0);
    if (heap_fd < 0) {
        throw std::runtime_error("failed to open dma_heap");
    }
    m_heap_fd = heap_fd;
}

DmaBufAlloc::~DmaBufAlloc() { close(m_heap_fd); }

int DmaBufAlloc::alloc_buf_fd(std::size_t len) {
    struct dma_heap_allocation_data alloc = {};
    alloc.len = len;
    alloc.fd_flags = O_CLOEXEC | O_RDWR;

    int success = ioctl(m_heap_fd, DMA_HEAP_IOCTL_ALLOC, &alloc);
    if (success < 0) {
        throw std::runtime_error("failed to allocate dma-heap");
    }
    return alloc.fd;
}
