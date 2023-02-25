/*
 * Copyright (C) 2023 Felipe Franciosi - All Rights Reserved
 */

#include <assert.h>
#include <errno.h>
#include <libserialport.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef __MACH__
#include <sys/uio.h>
#else
#include <sys/stat.h>
#endif

static int
list_ports(void)
{
	struct sp_port **ports, *port;
	enum sp_return sp_ret;
	int i;

	sp_ret = sp_list_ports(&ports);
	if (sp_ret != SP_OK) {
		perror("sp_list_ports");
		return -1;
	}

	for (i = 0, port = ports[i]; port != NULL; port = ports[++i]) {
		printf("%s\n", sp_get_port_name(port));
	}

	sp_free_port_list(ports);

	return 0;
}

static int
loop_on_port(struct sp_port *port)
{
	int port_fd;
	struct pollfd pfds[2];
	enum sp_return sp_ret;

	assert(port != NULL);

	sp_ret = sp_get_port_handle(port, &port_fd);
	if (sp_ret != SP_OK) {
		perror("sp_get_port_handle");
		return -1;
	}

	pfds[0].fd = 0; // stdin
	pfds[0].events = POLLIN;
	pfds[1].fd = port_fd;
	pfds[1].events = POLLIN;

	do {
		char c;
		ssize_t len;
		int rc;

		rc = poll(pfds, sizeof(pfds)/sizeof(struct pollfd), -1);
		if (rc == -1 && errno == EINTR) {
			return 0;
		}

		if (pfds[0].revents == POLLIN) {
			len = read(0, &c, 1);
			if (len == -1) {
				perror("read");
				return -1;
			}
#ifdef DEBUG_SERIAL
			printf("stdin: (0x%02hhX) %c\n", c, c);
			fflush(stdout);
#endif

			if (c == '\n') {
				const char cr = '\r';
				sp_ret = sp_blocking_write(port, &cr, 1, 0);
				if (sp_ret != 1) {
					perror("sp_blocking_write");
					return -1;
				}
			}

			sp_ret = sp_blocking_write(port, &c, 1, 0);
			if (sp_ret != 1) {
				perror("sp_blocking_write");
				return -1;
			}

			sp_ret = sp_drain(port);
			if (sp_ret != SP_OK) {
				perror("sp_drain");
				return -1;
			}
		}

		if (pfds[1].revents == POLLIN) {
			sp_ret = sp_blocking_read(port, &c, 1, 0);
			if (sp_ret != 1) {
				perror("sp_blocking_read");
				return -1;
			}
#ifdef DEBUG_SERIAL
			printf("port: (0x%02hhX) %c\n", c, c);
#else
			printf("%c", c);
#endif
			fflush(stdout);
		}
	} while(1);

	abort(); // unreachable.
	return 0;
}

static int
handle_port(const char *port_name)
{
	struct sp_port *port;
	enum sp_return sp_ret;
	int rc = -1;

	assert(port_name != NULL);

	sp_ret = sp_get_port_by_name(port_name, &port);
	if (sp_ret != SP_OK) {
		perror("sp_get_port_by_name");
		return -1;	
	}

	sp_ret = sp_open(port, SP_MODE_READ_WRITE);
	if (sp_ret != SP_OK) {
		perror("sp_open");
		goto out;
	}

	sp_ret = sp_set_baudrate(port, 115200);
	if (sp_ret != SP_OK) {
		perror("sp_set_baudrate");
		goto out_close;
	}

	rc = loop_on_port(port);

out_close:
	(void)sp_close(port);

out:
	sp_free_port(port);
	return rc;
}

int
main(int argc, char **argv)
{
	int rc;

	if (argc != 2) {
		list_ports();
		return EXIT_FAILURE;
	}

	rc = handle_port(argv[1]);
	if (rc == -1) {
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
