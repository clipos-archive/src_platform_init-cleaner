// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright © 2013-2018 ANSSI. All Rights Reserved.
/**
 *  Author: Mickaël Salaün <clipos@ssi.gouv.fr>
 *
 *  Copyright (C) 2013 SGDSN/ANSSI
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

#include <clip/clip.h>
#include <sys/capability.h>
#include <signal.h>
#include <stdio.h>

#include "init-cleaner.h"

/*
 * This process need:
 * - to be init (PID 1) in the jail
 * - to have CAP_KILL capability
 * - to have CAP_SETUID capability
 * - to have CAP_SETGID capability
 * - a read-only filesystem
 */

static clip_sock_t g_cmd_sock = {
	.sock = -1,
	.name = "command",
	.path = NULL,
	.handler = cmd_handler,
};

static struct cmd g_cmd = {
	.uid = -1,
	.gid = -1,
	.exe = NULL,
	.argv = NULL,
	.allowed_gid = -1,
};

/* Should be run in a read-only jail */
int do_clean(void)
{
	/* Kill all processes except ourself (init) */
	if (kill(-1, SIGKILL)) {
		ERROR("Failed to kill everyone");
		return -1;
	}
	/* Check for remaining processes */
	if (!kill(-1, 0)) {
		ERROR("Some process is still alive");
		return -1;
	}
	return 0;
}

int spawn_cmd(void)
{
	int child;

	child = fork();
	switch(child) {
		case -1:
			return -1;
			break;
		case 0:
			/* Child process */
			INFO("Executing %s", g_cmd.exe);
			if (clip_revokeprivs(g_cmd.uid, g_cmd.gid, NULL, 0, 0)) {
				ERROR("Failed to revoke privs");
				return -1;
			}
			if (clip_daemonize()) {
				ERROR("Failed to start a new session");
				return -1;
			}
			execve(g_cmd.exe, g_cmd.argv, NULL);
			/*ERROR("Failed to execute command");*/
			return -1;
			break;
		default:
			/*INFO("New child: %d", child);*/
			break;
	}
}

int do_init(void)
{
	if (do_clean()) {
		ERROR("Failed to cleanup");
		return -1;
	}
	return spawn_cmd();
}

/* Socket order:
 * R: reset all processes (killall and init)
 */

int cmd_handler(int sock, struct clip_sock_t *csock)
{
	char order;
	uid_t euid;
	gid_t egid;
	int ret = 0;

	INFO("Entering handler");
	/* Verify command orderer */
	clip_getpeereid(sock, &euid, &egid);
	if (egid != g_cmd.allowed_gid) {
		WARNING("Unallowed requester GID: %d", egid);
		return -1;
	}

	/* Read the command */
	clip_sock_read(sock, &order, sizeof(order), -1, 0);
	switch (order) {
		case 'R':
			do_init();
			break;
		default:
			ERROR("Unknown command: %c", order);
			ret = -1;
			break;
	}
	close(sock);
	return ret;
}

void print_usage(const char *name) {
	printf("usage: %s -s <socket-path> -a <allowed-launcher-gid> -u <spawned-process-uid> -g <spawned-process-gid> -- /path/to/bin args...\n", name);
}

extern char* optarg;
extern int optind, opterr, optopt;

static int set_options(int argc, const char *argv[])
{
	int flag;
	const char *name = argv[0];

	while ((flag = getopt(argc, argv, "s:a:u:g:")) != -1) {
		switch (flag) {
			case 's':
				g_cmd_sock.path = optarg;
				break;
			case 'a':
				g_cmd.allowed_gid = atoi(optarg);
				break;
			case 'u':
				g_cmd.uid = atoi(optarg);
				break;
			case 'g':
				g_cmd.gid = atoi(optarg);
				break;
		}
	}
	if (!g_cmd_sock.path) {
		INFO("Missing socket path");
		print_usage(name);
		return -1;
	}
	if (g_cmd.uid < 0) {
		INFO("Missing UID");
		print_usage(name);
		return -1;
	}
	if (g_cmd.gid < 0) {
		INFO("Missing GID");
		print_usage(name);
		return -1;
	}
	if (g_cmd.allowed_gid < 0) {
		INFO("Missing allowed GID");
		print_usage(name);
		return -1;
	}
	return 0;
}

int main(int argc, const char *argv[])
{
	const char *name = argv[0];

	/* Drop useless capabilities */
	if (clip_revokeprivs(0, 0, NULL, 0, 1UL << CAP_KILL || 1UL << CAP_SETUID || 1UL << CAP_SETGID)) {
		ERROR("Failed to revoke privs");
		return -1;
	}
	/* TODO: handle all signals */
	if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
		ERROR("Failed to handle CHLD signal");
		return -1;
	}

	/* Parse arguments */
	if (set_options(argc, argv)) {
		return -1;
	}
	argc -= optind;
	argv += optind;
	if (argc <= 0) {
		INFO("Missing command");
		print_usage(name);
		return -1;
	}
	g_cmd.exe = argv[0];
	g_cmd.argv = argv;

	/* Wait for orders */
	/* TODO: chmod + chown */
	g_cmd_sock.sock = clip_sock_listen(g_cmd_sock.path, &g_cmd_sock.sau, 0);
	if (g_cmd_sock.sock < 0) {
		ERROR("Failed to open socket");
		return -1;
	}
	for (;;) {
		if (clip_accept_one(&g_cmd_sock, 1, 1)) {
			ERROR("Failed to accept socket");
			return -1;
		}
	}
	return 0;
}
