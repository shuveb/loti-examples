#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include "liburing.h"

#define BUF_SIZE    512
#define FILE_NAME1   "/tmp/io_uring_test.txt"
#define STR1        "What is this life if, full of care,\n"
#define STR2        "We have no time to stand and stare."

int start_fixed_buffer_ops(struct io_uring *ring) {
    struct iovec iov[4];
    struct io_uring_sqe *sqe;
    struct io_uring_cqe *cqe;

    int fd = open(FILE_NAME1, O_RDWR | O_TRUNC | O_CREAT, 0644);
    if (fd < 0 ) {
        perror("open");
        return 1;
    }

    for (int i = 0; i < 4; i++) {
        iov[i].iov_base = malloc(BUF_SIZE);
        iov[i].iov_len = BUF_SIZE;
        memset(iov[i].iov_base, 0, BUF_SIZE);
    }

    int ret = io_uring_register_buffers(ring, iov, 4);
    if(ret) {
        fprintf(stderr, "Error registering buffers: %s", strerror(-ret));
        return 1;
    }

    sqe = io_uring_get_sqe(ring);
    if (!sqe) {
        fprintf(stderr, "Could not get SQE.\n");
        return 1;
    }

    int str1_sz = strlen(STR1);
    int str2_sz = strlen(STR2);
    strncpy(iov[0].iov_base, STR1, str1_sz);
    strncpy(iov[1].iov_base, STR2, str2_sz);
    io_uring_prep_write_fixed(sqe, fd, iov[0].iov_base, str1_sz, 0, 0);

    sqe = io_uring_get_sqe(ring);
    if (!sqe) {
        fprintf(stderr, "Could not get SQE.\n");
        return 1;
    }
    io_uring_prep_write_fixed(sqe, fd, iov[1].iov_base, str2_sz, str1_sz, 1);

    io_uring_submit(ring);

    for(int i = 0; i < 2; i ++) {
        int ret = io_uring_wait_cqe(ring, &cqe);
        if (ret < 0) {
            fprintf(stderr, "Error waiting for completion: %s\n",
                    strerror(-ret));
            return 1;
        }
        /* Now that we have the CQE, let's process the data */
        if (cqe->res < 0) {
            fprintf(stderr, "Error in async operation: %s\n", strerror(-cqe->res));
        }
        printf("Result of the operation: %d\n", cqe->res);
        io_uring_cqe_seen(ring, cqe);
    }

    sqe = io_uring_get_sqe(ring);
    if (!sqe) {
        fprintf(stderr, "Could not get SQE.\n");
        return 1;
    }

    io_uring_prep_read_fixed(sqe, fd, iov[2].iov_base, str1_sz, 0, 2);

    sqe = io_uring_get_sqe(ring);
    if (!sqe) {
        fprintf(stderr, "Could not get SQE.\n");
        return 1;
    }

    io_uring_prep_read_fixed(sqe, fd, iov[3].iov_base, str2_sz, str1_sz, 3);

    io_uring_submit(ring);
    for(int i = 0; i < 2; i ++) {
        int ret = io_uring_wait_cqe(ring, &cqe);
        if (ret < 0) {
            fprintf(stderr, "Error waiting for completion: %s\n",
                    strerror(-ret));
            return 1;
        }
        /* Now that we have the CQE, let's process the data */
        if (cqe->res < 0) {
            fprintf(stderr, "Error in async operation: %s\n", strerror(-cqe->res));
        }
        printf("Result of the operation: %d\n", cqe->res);
        io_uring_cqe_seen(ring, cqe);
    }
    printf("Contents read from file:\n");
    printf("%s%s", iov[2].iov_base, iov[3].iov_base);
}

int main() {
    struct io_uring ring;

    int ret = io_uring_queue_init(8, &ring, 0);
    if (ret) {
        fprintf(stderr, "Unable to setup io_uring: %s\n", strerror(-ret));
        return 1;
    }
    start_fixed_buffer_ops(&ring);
    io_uring_queue_exit(&ring);
    return 0;
}
