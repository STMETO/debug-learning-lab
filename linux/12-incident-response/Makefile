CC := gcc
all: incident_service
incident_service: service.c
	$(CC) -g -Wall -Wextra -o $@ $<
.PHONY: clean
clean:
	rm -f incident_service
