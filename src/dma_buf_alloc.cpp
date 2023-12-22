/*
 * Copyright (C) Photon Vision.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "dma_buf_alloc.h"

#include <fcntl.h>
#include <linux/dma-buf.h>
#include <linux/dma-heap.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <stdexcept>

DmaBufAlloc::DmaBufAlloc(const std::string &heap_name) {
    int heap_fd = open(heap_name.c_str(), O_RDWR, 0);
    if (heap_fd < 0) {
        throw std::runtime_error("failed to open dma_heap");
    }
    m_heap_fd = heap_fd;
}

DmaBufAlloc::~DmaBufAlloc() { close(m_heap_fd); }

int DmaBufAlloc::alloc_buf_fd(size_t len) {
    struct dma_heap_allocation_data alloc = {};
    alloc.len = len;
    alloc.fd_flags = O_CLOEXEC | O_RDWR;

    int success = ioctl(m_heap_fd, DMA_HEAP_IOCTL_ALLOC, &alloc);
    if (success < 0) {
        throw std::runtime_error("failed to allocate dma-heap");
    }
    return alloc.fd;
}
