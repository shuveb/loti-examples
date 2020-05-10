#include <sys/eventfd.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <liburing.h>
#include <fcntl.h>

#define BUFF_SZ   512

char buff[BUFF_SZ + 1];
struct io_uring ring;

void error_exit(char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

void *listener_thread(void *data) {
    struct io_uring_cqe *cqe;
    int efd = (int) data;
    eventfd_t v;
    printf("%s: Waiting for completion event...\n", __FUNCTION__);

    int ret = eventfd_read(efd, &v);
    if (ret < 0) error_exit("eventfd_read");

    printf("%s: Got completion event.\n", __FUNCTION__);

    ret = io_uring_wait_cqe(&ring, &cqe);
    if (ret < 0) {
        fprintf(stderr, "Error waiting for completion: %s\n",
                strerror(-ret));
        return NULL;
    }
    /* Now that we have the CQE, let's process it */
    if (cqe->res < 0) {
        fprintf(stderr, "Error in async operation: %s\n", strerror(-cqe->res));
    }
    printf("Result of the operation: %d\n", cqe->res);
    io_uring_cqe_seen(&ring, cqe);

    printf("Contents read from file:\n%s\n", buff);
    return NULL;
}

int setup_io_uring(int efd) {
    int ret = io_uring_queue_init(8, &ring, 0);
    if (ret) {
        fprintf(stderr, "Unable to setup io_uring: %s\n", strerror(-ret));
        return 1;
    }
    io_uring_register_eventfd(&ring, efd);
    return 0;
}

int read_file_with_io_uring() {
    struct io_uring_sqe *sqe;

    sqe = io_uring_get_sqe(&ring);
    if (!sqe) {
        fprintf(stderr, "Could not get SQE.\n");
        return 1;
    }

    int fd = open("/etc/passwd", O_RDONLY);
    io_uring_prep_read(sqe, fd, buff, BUFF_SZ, 0);
    io_uring_submit(&ring);

    return 0;
}

int main() {
    pthread_t t;
    int efd;

    /* Create an eventfd instance */
    efd = eventfd(0, 0);
    if (efd < 0)
        error_exit("eventfd");

    /* Create the listener thread */
    pthread_create(&t, NULL, listener_thread, (void *)efd);

    sleep(2);

    /* Setup io_uring instance and register the eventfd */
    setup_io_uring(efd);

    /* Initiate a read with io_uring */
    read_file_with_io_uring();

    /* Wait for th listener thread to complete */
    pthread_join(t, NULL);

    /* All done. Clean up and exit. */
    io_uring_queue_exit(&ring);
    return EXIT_SUCCESS;
}
