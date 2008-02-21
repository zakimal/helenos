/*
 * Copyright (c) 2008 Jakub Jermar 
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */
 
#include <vfs.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ipc/ipc.h>
#include <ipc/services.h>
#include <async.h>
#include <atomic.h>
#include <futex.h>
#include <errno.h>
#include <string.h>
#include "../../srv/vfs/vfs.h"

int vfs_phone = -1;
atomic_t vfs_phone_futex = FUTEX_INITIALIZER;

static int vfs_connect(void)
{
	if (vfs_phone < 0)
		vfs_phone = ipc_connect_me_to(PHONE_NS, SERVICE_VFS, 0, 0);
	return vfs_phone;
}

int mount(const char *fs_name, const char *mp, const char *dev)
{
	int res;
	ipcarg_t rc;
	aid_t req;

	int dev_handle = 0;	/* TODO */

	futex_down(&vfs_phone_futex);
	async_serialize_start();
	if (vfs_phone < 0) {
		res = vfs_connect();
		if (res < 0) {
			async_serialize_end();
			futex_up(&vfs_phone_futex);
			return res;
		}
	}
	req = async_send_1(vfs_phone, VFS_MOUNT, dev_handle, NULL);
	rc = ipc_data_write_start(vfs_phone, (void *)fs_name, strlen(fs_name));
	if (rc != EOK) {
		async_wait_for(req, NULL);
		async_serialize_end();
		futex_up(&vfs_phone_futex);
		return (int) rc;
	}
	rc = ipc_data_write_start(vfs_phone, (void *)mp, strlen(mp));
	if (rc != EOK) {
		async_wait_for(req, NULL);
		async_serialize_end();
		futex_up(&vfs_phone_futex);
		return (int) rc;
	}
	async_wait_for(req, &rc);
	async_serialize_end();
	futex_up(&vfs_phone_futex);
	return (int) rc;
}

static int _open(const char *path, int lflag, int oflag, ...)
{
	int res;
	ipcarg_t rc;
	ipc_call_t answer;
	aid_t req;
	
	futex_down(&vfs_phone_futex);
	async_serialize_start();
	if (vfs_phone < 0) {
		res = vfs_connect();
		if (res < 0) {
			async_serialize_end();
			futex_up(&vfs_phone_futex);
			return res;
		}
	}
	req = async_send_3(vfs_phone, VFS_OPEN, lflag, oflag, 0, &answer);
	rc = ipc_data_write_start(vfs_phone, path, strlen(path));
	if (rc != EOK) {
		async_wait_for(req, NULL);
		async_serialize_end();
		futex_up(&vfs_phone_futex);
		return (int) rc;
	}
	async_wait_for(req, &rc);
	async_serialize_end();
	futex_up(&vfs_phone_futex);
	return (int) IPC_GET_ARG1(answer);
}

int open(const char *path, int oflag, ...)
{
	return _open(path, L_FILE, oflag);
}

int close(int fildes)
{
	int res;
	ipcarg_t rc;

	futex_down(&vfs_phone_futex);
	async_serialize_start();
	if (vfs_phone < 0) {
		res = vfs_connect();
		if (res < 0) {
			async_serialize_end();
			futex_up(&vfs_phone_futex);
			return res;
		}
	}
		
	rc = async_req_1_0(vfs_phone, VFS_CLOSE, fildes);

	async_serialize_end();
	futex_up(&vfs_phone_futex);
	
	return (int)rc;
}

ssize_t read(int fildes, void *buf, size_t nbyte) 
{
	int res;
	ipcarg_t rc;
	ipc_call_t answer;
	aid_t req;

	futex_down(&vfs_phone_futex);
	async_serialize_start();
	if (vfs_phone < 0) {
		res = vfs_connect();
		if (res < 0) {
			async_serialize_end();
			futex_up(&vfs_phone_futex);
			return res;
		}
	}
	req = async_send_1(vfs_phone, VFS_READ, fildes, &answer);
	rc = ipc_data_read_start(vfs_phone, (void *)buf, nbyte);
	if (rc != EOK) {
		async_wait_for(req, NULL);
		async_serialize_end();
		futex_up(&vfs_phone_futex);
		return (ssize_t) rc;
	}
	async_wait_for(req, &rc);
	async_serialize_end();
	futex_up(&vfs_phone_futex);
	if (rc == EOK)
		return (ssize_t) IPC_GET_ARG1(answer);
	else
		return -1;
}

ssize_t write(int fildes, const void *buf, size_t nbyte) 
{
	int res;
	ipcarg_t rc;
	ipc_call_t answer;
	aid_t req;

	futex_down(&vfs_phone_futex);
	async_serialize_start();
	if (vfs_phone < 0) {
		res = vfs_connect();
		if (res < 0) {
			async_serialize_end();
			futex_up(&vfs_phone_futex);
			return res;
		}
	}
	req = async_send_1(vfs_phone, VFS_WRITE, fildes, &answer);
	rc = ipc_data_write_start(vfs_phone, (void *)buf, nbyte);
	if (rc != EOK) {
		async_wait_for(req, NULL);
		async_serialize_end();
		futex_up(&vfs_phone_futex);
		return (ssize_t) rc;
	}
	async_wait_for(req, &rc);
	async_serialize_end();
	futex_up(&vfs_phone_futex);
	if (rc == EOK)
		return (ssize_t) IPC_GET_ARG1(answer);
	else
		return -1;
}

off_t lseek(int fildes, off_t offset, int whence)
{
	int res;
	ipcarg_t rc;

	futex_down(&vfs_phone_futex);
	async_serialize_start();
	if (vfs_phone < 0) {
		res = vfs_connect();
		if (res < 0) {
			async_serialize_end();
			futex_up(&vfs_phone_futex);
			return res;
		}
	}
		
	off_t newoffs;
	rc = async_req_3_1(vfs_phone, VFS_SEEK, fildes, offset, whence,
	    (ipcarg_t)&newoffs);

	async_serialize_end();
	futex_up(&vfs_phone_futex);

	if (rc != EOK)
		return (off_t) -1;
	
	return newoffs;
}

int ftruncate(int fildes, off_t length)
{
	int res;
	ipcarg_t rc;
	
	futex_down(&vfs_phone_futex);
	async_serialize_start();
	if (vfs_phone < 0) {
		res = vfs_connect();
		if (res < 0) {
			async_serialize_end();
			futex_up(&vfs_phone_futex);
			return res;
		}
	}
	rc = async_req_2_0(vfs_phone, VFS_TRUNCATE, fildes, length);
	async_serialize_end();
	futex_up(&vfs_phone_futex);
	return (int) rc;
}

DIR *opendir(const char *dirname)
{
	DIR *dirp = malloc(sizeof(DIR));
	if (!dirp)
		return NULL;
	dirp->fd = _open(dirname, L_DIRECTORY, 0);
	if (dirp->fd < 0) {
		free(dirp);
		return NULL;
	}
	return dirp;
}

struct dirent *readdir(DIR *dirp)
{
	ssize_t len = read(dirp->fd, &dirp->res.d_name[0], NAME_MAX + 1);
	if (len <= 0)
		return NULL;
	return &dirp->res;
}

void rewinddir(DIR *dirp)
{
	(void) lseek(dirp->fd, 0, SEEK_SET);
}

int closedir(DIR *dirp)
{
	(void) close(dirp->fd);
	free(dirp);
	return 0;
}

int mkdir(const char *path, mode_t mode)
{
	int res;
	ipcarg_t rc;
	aid_t req;
	
	futex_down(&vfs_phone_futex);
	async_serialize_start();
	if (vfs_phone < 0) {
		res = vfs_connect();
		if (res < 0) {
			async_serialize_end();
			futex_up(&vfs_phone_futex);
			return res;
		}
	}
	req = async_send_1(vfs_phone, VFS_MKDIR, mode, NULL);
	rc = ipc_data_write_start(vfs_phone, path, strlen(path));
	if (rc != EOK) {
		async_wait_for(req, NULL);
		async_serialize_end();
		futex_up(&vfs_phone_futex);
		return (int) rc;
	}
	async_wait_for(req, &rc);
	async_serialize_end();
	futex_up(&vfs_phone_futex);
	return EOK; 
}

static int _unlink(const char *path, int lflag)
{
	int res;
	ipcarg_t rc;
	aid_t req;
	
	futex_down(&vfs_phone_futex);
	async_serialize_start();
	if (vfs_phone < 0) {
		res = vfs_connect();
		if (res < 0) {
			async_serialize_end();
			futex_up(&vfs_phone_futex);
			return res;
		}
	}
	req = async_send_0(vfs_phone, VFS_UNLINK, NULL);
	rc = ipc_data_write_start(vfs_phone, path, strlen(path));
	if (rc != EOK) {
		async_wait_for(req, NULL);
		async_serialize_end();
		futex_up(&vfs_phone_futex);
		return (int) rc;
	}
	async_wait_for(req, &rc);
	async_serialize_end();
	futex_up(&vfs_phone_futex);
	return EOK; 
}

int unlink(const char *path)
{
	return _unlink(path, L_NONE);
}

int rmdir(const char *path)
{
	return _unlink(path, L_DIRECTORY);
}

/** @}
 */
