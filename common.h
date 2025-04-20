#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

#define SHM_KEY 0x1234 // Key cho Shared Memory
#define SEM_KEY 0x5678 // Key cho Semaphore
#define FIFO_PATH "/tmp/net_monitor_fifo" // Đường dẫn cho Named Pipe
#define NET_INTERFACE "ens33" // Tên card mạng cần giám sát

// Cấu trúc dữ liệu chia sẻ trong Shared Memory
struct net_stats {
    long long rx_bytes;
    long long rx_packets;
    long long rx_errors;
    long long tx_bytes;
    long long tx_packets;
    long long tx_errors;
    time_t timestamp; // Thời điểm thu thập
};

// Cấu trúc cho thao tác semaphore
union semun {
    int val;                /* Value for SETVAL */
    struct semid_ds *buf;   /* Buffer for IPC_STAT, IPC_SET */
    unsigned short *array; /* Array for GETALL, SETALL */
    struct seminfo *__buf; /* Buffer for IPC_INFO (Linux specific) */
};

// Hàm trợ giúp cho thao tác semaphore
// op = -1: Wait (P operation)
// op = 1: Signal (V operation)
int semaphore_op(int semid, int op) {
    struct sembuf sb;
    sb.sem_num = 0; // Chỉ dùng semaphore đầu tiên trong set
    sb.sem_op = op;
    sb.sem_flg = SEM_UNDO; // Tự động hoàn tác nếu tiến trình chết
    if (semop(semid, &sb, 1) == -1) {
        perror("semop");
        return -1;
    }
    return 0;
}

#endif // COMMON_H
