#include "common.h"

int main() {
    int fifo_fd;
    char buffer[512]; // Bộ đệm để đọc cảnh báo

    // 1. Mở Named Pipe (FIFO) ở chế độ đọc
    // Việc mở này sẽ block cho đến khi Analyzer mở FIFO để ghi
    printf("Logger (PID: %d) waiting to open FIFO %s for reading...\n", getpid(), FIFO_PATH);
    fifo_fd = open(FIFO_PATH, O_RDONLY);
    if (fifo_fd == -1) {
        perror("open FIFO for reading (logger)");
        exit(EXIT_FAILURE);
    }
    printf("Logger connected to Analyzer via FIFO.\n");

    // 2. Vòng lặp đọc và ghi log
    printf("Logger waiting for alerts...\n");
    while (1) {
        ssize_t bytes_read = read(fifo_fd, buffer, sizeof(buffer) -1); // Đọc từ pipe

        if (bytes_read > 0) {
            buffer[bytes_read] = '\0'; // Đảm bảo chuỗi kết thúc bằng null
            // Ghi log (ở đây là in ra màn hình)
            printf("Logger Received: %s\n", buffer);
            // Bạn có thể thay thế bằng việc ghi vào file log ở đây
            /*
            FILE *log_file = fopen("network_alerts.log", "a");
            if (log_file) {
                fprintf(log_file, "[%ld] %s\n", time(NULL), buffer);
                fclose(log_file);
            } else {
                perror("fopen log file");
            }
            */
        } else if (bytes_read == 0) {
            // Analyzer đã đóng đầu ghi của pipe
            printf("Logger: Analyzer closed the pipe. Exiting.\n");
            break;
        } else {
            // Lỗi đọc pipe
            perror("read from FIFO (logger)");
            break; // Thoát nếu có lỗi
        }
    }

    // 3. Đóng pipe
    close(fifo_fd);
    printf("Logger exiting.\n");

    return 0;
}
