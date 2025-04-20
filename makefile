# Makefile for Network Monitoring System (Detailed Analysis)

CC = gcc
CFLAGS = -Wall -Wextra -g
# CFLAGS = -Wall -Wextra -O2

# Linker flags: Thêm -lpcap cho Monitor
LDFLAGS_MONITOR = -lpcap
LDFLAGS_ANALYZER =
LDFLAGS_LOGGER =

TARGETS = analyzer monitor logger

SOURCES_ANALYZER = analyzer.c
SOURCES_MONITOR = monitor.c
SOURCES_LOGGER = logger.c
HEADERS = common.h

.PHONY: all
all: $(TARGETS)

analyzer: $(SOURCES_ANALYZER) $(HEADERS)
	$(CC) $(CFLAGS) $(SOURCES_ANALYZER) -o analyzer $(LDFLAGS_ANALYZER)

# Monitor cần link với libpcap (-lpcap)
monitor: $(SOURCES_MONITOR) $(HEADERS)
	$(CC) $(CFLAGS) $(SOURCES_MONITOR) -o monitor $(LDFLAGS_MONITOR)

logger: $(SOURCES_LOGGER) $(HEADERS)
	$(CC) $(CFLAGS) $(SOURCES_LOGGER) -o logger $(LDFLAGS_LOGGER)

.PHONY: clean
clean:
	@echo "Cleaning up compiled files..."
	rm -f $(TARGETS) *.o core
	@echo "Cleanup complete."
