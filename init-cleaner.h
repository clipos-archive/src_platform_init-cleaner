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

#define ERROR(fmt, args...) printf("ERROR: %s: " fmt "\n", __func__ , ##args)
#define INFO(fmt, args...) printf("INFO: " fmt "\n", ##args)
#define WARNING(fmt, args...) printf("WARNING: " fmt "\n", ##args)

struct cmd {
	uid_t uid;
	gid_t gid;
	gid_t allowed_gid;
	const char *exe;
	const char **argv;
};

int cmd_handler(int sock, struct clip_sock_t *csock);
