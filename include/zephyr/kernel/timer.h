/*
 * Copyright (c) 2016, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_KERNEL_TIMER_H_
#define ZEPHYR_INCLUDE_KERNEL_TIMER_H_

#include <zephyr/tracing/tracing_macros.h>

/**
 * @brief Kernel timer structure
 *
 * This structure is used to represent a kernel timer.
 * All the members are internal and should not be accessed directly.
 */
struct k_timer {
	/**
	 * @cond INTERNAL_HIDDEN
	 */

	/*
	 * _timeout structure must be first here if we want to use
	 * dynamic timer allocation. timeout.node is used in the double-linked
	 * list of free timers
	 */
	struct _timeout timeout;

	/* wait queue for the (single) thread waiting on this timer */
	_wait_q_t wait_q;

	/* runs in ISR context */
	void (*expiry_fn)(struct k_timer *timer);

	/* runs in the context of the thread that calls k_timer_stop() */
	void (*stop_fn)(struct k_timer *timer);

	/* timer period */
	k_timeout_t period;

	/* timer status */
	uint32_t status;

	/* user-specific data, also used to support legacy features */
	void *user_data;

	SYS_PORT_TRACING_TRACKING_FIELD(k_timer)

#ifdef CONFIG_OBJ_CORE_TIMER
	struct k_obj_core  obj_core;
#endif
	/**
	 * INTERNAL_HIDDEN @endcond
	 */
};

/**
 * @cond INTERNAL_HIDDEN
 */
#define Z_TIMER_INITIALIZER(obj, expiry, stop) \
	{ \
	.timeout = { \
		.node = {},\
		.fn = z_timer_expiration_handler, \
		.dticks = 0, \
	}, \
	.wait_q = Z_WAIT_Q_INIT(&obj.wait_q), \
	.expiry_fn = expiry, \
	.stop_fn = stop, \
	.period = {}, \
	.status = 0, \
	.user_data = 0, \
	}

/**
 * INTERNAL_HIDDEN @endcond
 */

#endif /* ZEPHYR_INCLUDE_KERNEL_TIMER_H_ */
