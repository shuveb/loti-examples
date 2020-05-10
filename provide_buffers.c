#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <liburing.h>

#define BUF_SZ      512
#define BUF_COUNT   32
#define BUF_GRP_ID  0xBEEF

int setup_buffers(struct io_uring *ring) {
    struct io_uring_sqe *sqe;

    sqe = io_uring_get_sqe(ring);
    io_uring_prep_provide_buffers(sqe, bufs, BUF_SZ, BUF_COUNT, BUF_GRP_ID, 0);

    io_uring_submit(&ring);
    io_uring_wait_cqe(&ring, &cqe);
    if (cqe->res < 0) {
        printf("cqe->res = %d\n", cqe->res);
        exit(1);
    }
    io_uring_cqe_seen(&ring, cqe);
}

int provide_buffers(struct io_uring *ring) {

}

int main() {
    struct io_uring ring;

    /* Ensure that the running kernel supports IORING_OP_PROVIDE_BUFFERS */
    struct io_uring_probe *probe = io_uring_get_probe();
    if(!io_uring_opcode_supported(probe, IORING_OP_PROVIDE_BUFFERS)) {
        fprintf(stderr, "The provide buffers operation is not supported by the running kernel.\n");
        free(probe);
        return 1;
    }
    free(probe);

    int ret = io_uring_queue_init(8, &ring, 0);
    if (ret) {
        fprintf(stderr, "Unable to setup io_uring: %s\n", strerror(-ret));
        return 1;
    }
    provide_buffers(&ring);
    io_uring_queue_exit(&ring);
    return 0;
}
