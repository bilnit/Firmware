
/****************************************************************************
 *
 *   Copyright (C) 2015 Mark Charlebois. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/**
 * @file vcdevtest_example.cpp
 * Example for Linux
 *
 * @author Mark Charlebois <charlebm@gmail.com>
 */

#include <px4_tasks.h>
#include "vcdevtest_example.h"
#include "drivers/device/device.h"
#include <unistd.h>
#include <stdio.h>

px4::AppState VCDevExample::appState;

using namespace device;

#define TESTDEV "/dev/vdevex"

int writer_main(int argc, char *argv[])
{
	char buf[1] = { '1' };

	int fd = px4_open(TESTDEV, PX4_F_RDONLY);
	if (fd < 0) {
		printf("--- Open failed %d %d", fd, px4_errno);
		return -px4_errno;
	}

	// Wait for 3 seconds
	printf("--- Sleeping for 3 sec\n");
	int ret = sleep(3);
	if (ret < 0) {
		printf("--- usleep failed %d %d\n", ret, errno);
		return ret;
	}

	printf("--- writing to fd\n");
	return px4_write(fd, buf, 1);
}

class VCDevNode : public CDev {
public:
	VCDevNode() : CDev("vcdevtest", TESTDEV) {};

	~VCDevNode() {}

	virtual ssize_t write(px4_dev_handle_t *handlep, const char *buffer, size_t buflen);
};

ssize_t VCDevNode::write(px4_dev_handle_t *handlep, const char *buffer, size_t buflen)
{
	// ignore what was written, but let pollers know something was written
	poll_notify(POLLIN);

	return buflen;
}

VCDevExample::~VCDevExample() {
	if (_node) {
		delete _node;
		_node = 0;
	}
}

int test_pub_block(int fd, unsigned long blocked)
{
	int ret = px4_ioctl(fd, PX4_DEVIOCSPUBBLOCK, blocked);
	if (ret < 0) {
		printf("ioctl PX4_DEVIOCSPUBBLOCK failed %d %d", ret, px4_errno);
		return -px4_errno;
	}

	ret = px4_ioctl(fd, PX4_DEVIOCGPUBBLOCK, 0);
	if (ret < 0) {
		printf("ioctl PX4_DEVIOCGPUBBLOCK failed %d %d", ret, px4_errno);
		return -px4_errno;
	}
	printf("pub_blocked = %d %s\n", ret, (unsigned long)ret == blocked ? "PASS" : "FAIL");

	return 0;
}

int VCDevExample::main()
{
	appState.setRunning(true);

	_node = new VCDevNode();

	if (_node == 0) {
		printf("Failed to allocate VCDevNode\n");
		return -ENOMEM;
	}

	if (_node->init() != PX4_OK) {
		printf("Failed to init VCDevNode\n");
		return 1;
	}

	int fd = px4_open(TESTDEV, PX4_F_RDONLY);

	if (fd < 0) {
		printf("Open failed %d %d", fd, px4_errno);
		return -px4_errno;
	}

	void *p = 0;
	int ret = px4_ioctl(fd, PX4_DIOC_GETPRIV, (unsigned long)&p);
	if (ret < 0) {
		printf("ioctl PX4_DIOC_GETPRIV failed %d %d", ret, px4_errno);
		return -px4_errno;
	}
	printf("priv data = %p %s\n", p, p == (void *)_node ? "PASS" : "FAIL");

	ret = test_pub_block(fd, 1);
	if (ret < 0)
		return ret;
	ret = test_pub_block(fd, 0);
	if (ret < 0)
		return ret;

	int i=0;
	px4_pollfd_struct_t fds[1];

	// Start a task that will write something in 3 seconds
	(void)px4_task_spawn_cmd("writer", 
				       SCHED_DEFAULT,
				       SCHED_PRIORITY_MAX - 6,
				       2000,
				       writer_main,
				       (char* const*)NULL);

	while (!appState.exitRequested() && i<3) {
		sleep(2);

		printf("  polling...\n");
		fds[0].fd = fd;
		fds[0].events = POLLIN;
		fds[0].revents = 0;

		printf("  Calling Poll\n");
		ret = px4_poll(fds, 1, 1000);
		printf("  Done poll\n");
		if (ret < 0) {
			printf("poll failed %d %d\n", ret, px4_errno);
			px4_close(fd);
		} 
		else if (i > 0 && ret == 0)
			printf("  Nothing to read - PASS\n");
		else if (i == 0) {
			if (ret == 1) 
				printf("  %d to read - %s\n", ret, fds[0].revents & POLLIN ? "PASS" : "FAIL");
			else
				printf("  %d to read - FAIL\n", ret);
		
		}
		++i;
	}
	px4_close(fd);

	return 0;
}
