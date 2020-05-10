#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include "liburing.h"

#define FILE_NAME1   "/tmp/io_uring_test.txt"
#define STR         "Hello, io_uring!"
char buff[32];

int link_operations(struct io_uring *ring) {
    struct io_uring_sqe *sqe;
    struct io_uring_cqe *cqe;

    int fd = open(FILE_NAME1, O_RDWR | O_TRUNC | O_CREAT, 0644);
    if (fd < 0 ) {
        perror("open");
        return 1;
    }

    sqe = io_uring_get_sqe(ring);
    if (!sqe) {
        fprintf(stderr, "Could not get SQE.\n");
        return 1;
    }

    io_uring_prep_write(sqe, fd, STR, strlen(STR), 0 );
    sqe->flags |= IOSQE_IO_LINK;

    sqe = io_uring_get_sqe(ring);
    if (!sqe) {
        fprintf(stderr, "Could not get SQE.\n");
        return 1;
    }

    io_uring_prep_read(sqe, fd, buff, strlen(STR),0);
    sqe->flags |= IOSQE_IO_LINK;

    sqe = io_uring_get_sqe(ring);
    if (!sqe) {
        fprintf(stderr, "Could not get SQE.\n");
        return 1;
    }

    io_uring_prep_close(sqe, fd);

    io_uring_submit(ring);

    for (int i = 0; i < 3; i++) {
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
    printf("Buffer contents: %s\n", buff);
}

int main() {
    struct io_uring ring;

    int ret = io_uring_queue_init(8, &ring, 0);
    if (ret) {
        fprintf(stderr, "Unable to setup io_uring: %s\n", strerror(-ret));
        return 1;
    }
    link_operations(&ring);
    io_uring_queue_exit(&ring);
    return 0;
}
