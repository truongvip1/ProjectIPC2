#include "common.h"
#include <sys/types.h>
#include <unistd.h>

// Hàm đọc thông tin từ /proc/net/dev
int get_net_stats(const char *iface, struct net_stats *stats) {
    FILE *fp;
    char line[256];
    char iface_colon[64];
    snprintf(iface_colon, sizeof(iface_colon), "%s:", iface);

    fp = fopen("/proc/net/dev", "r");
    if (fp == NULL) {
        perror("fopen /proc/net/dev");
        return -1;
    }

    // Bỏ qua 2 dòng header
    fgets(line, sizeof(line), fp);
    fgets(line, sizeof(line), fp);

    while (fgets(line, sizeof(line), fp)) {
        // Tìm dòng chứa interface mong muốn
        if (strstr(line, iface_colon)) {
            // Định dạng của dòng trong /proc/net/dev (có thể thay đổi giữa các kernel)
            // Interface: Receive bytes packets errs drop fifo frame compressed multicast | Transmit bytes packets errs drop fifo colls carrier compressed
            sscanf(line, "%*s %lld %lld %lld %*d %*d %*d %*d %*d %lld %lld %lld",
                   &stats->rx_bytes, &stats->rx_packets, &stats->rx_errors,
                   &stats->tx_bytes, &stats->tx_packets, &stats->tx_errors);
            stats->timestamp = time(NULL);
            fclose(fp);
            return 0;
        }
    }

    fprintf(stderr, "Interface %s not found in /proc/net/dev\n", iface);
    fclose(fp);
    return -1;
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <analyzer_pid>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    pid_t analyzer_pid = atoi(argv[1]);
    int shmid;
    int semid;
    struct net_stats *shared_data;

    // 1. Kết nối tới Shared Memory đã tạo bởi Analyzer
    shmid = shmget(SHM_KEY, sizeof(struct net_stats), 0666);
    if (shmid == -1) {
        perror("shmget (monitor)");
        exit(EXIT_FAILURE);
    }
    shared_data = (struct net_stats *)shmat(shmid, NULL, 0);
    if (shared_data == (void *)-1) {
        perror("shmat (monitor)");
        exit(EXIT_FAILURE);
    }

    // 2. Kết nối tới Semaphore đã tạo bởi Analyzer
    semid = semget(SEM_KEY, 1, 0666);
    if (semid == -1) {
        perror("semget (monitor)");
        shmdt(shared_data);
        exit(EXIT_FAILURE);
    }

    printf("Monitor (PID: %d) started, targeting Analyzer PID: %d\n", getpid(), analyzer_pid);

    // 3. Vòng lặp thu thập và gửi dữ liệu
    while (1) {
        struct net_stats current_stats;
        if (get_net_stats(NET_INTERFACE, &current_stats) == 0) {

            // 4. Đồng bộ: Đợi semaphore
            if (semaphore_op(semid, -1) == -1) { // Wait (P operation)
                 fprintf(stderr, "Monitor %d failed semaphore wait\n", getpid());
                 break; // Thoát nếu không thể đợi semaphore
            }

            // 5. Ghi dữ liệu vào Shared Memory
            memcpy(shared_data, &current_stats, sizeof(struct net_stats));
             printf("Monitor (PID: %d) wrote %lld RX bytes, %lld TX bytes at %ld\n",
                   getpid(), shared_data->rx_bytes, shared_data->tx_bytes, shared_data->timestamp);


            // 6. Đồng bộ: Giải phóng semaphore
            if (semaphore_op(semid, 1) == -1) { // Signal (V operation)
                 fprintf(stderr, "Monitor %d failed semaphore signal\n", getpid());
                 // Dù lỗi vẫn cố gắng gửi tín hiệu và tiếp tục
            }

            // 7. Gửi tín hiệu SIGUSR1 cho Analyzer
            if (kill(analyzer_pid, SIGUSR1) == -1) {
                perror("kill (monitor)");
                // Analyzer có thể đã chết, Monitor nên thoát ra
                 break;
            }
             // printf("Monitor (PID: %d) sent signal to Analyzer %d\n", getpid(), analyzer_pid);

        } else {
            fprintf(stderr, "Monitor (PID: %d) failed to get net stats\n", getpid());
        }

        // Ngủ một khoảng thời gian trước khi thu thập lại
        sleep(5); // Ví dụ: 5 giây
    }

    // 8. Dọn dẹp: Detach Shared Memory
    shmdt(shared_data);
    printf("Monitor (PID: %d) exiting.\n", getpid());

    return 0;
}
