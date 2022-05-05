/* The MIT License (MIT)
 * 
 * Copyright (c) 2022 Monetra Technologies, LLC.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "m_net_int.h"
#include <mstdlib/base/m_defs.h> /* M_CAST_OFF_CONST */
#include "smtp/flow.h"

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

struct M_net_smtp {
	M_event_t                         *el;
	struct M_net_smtp_callbacks        cbs;
	void                              *thunk;
	M_list_t                          *proc_endpoints;
	M_list_t                          *tcp_endpoints;
	size_t                             number_of_endpoints;
	M_list_t                          *endpoint_managers;
	M_net_smtp_status_t                status;
	size_t                             retry_default_ms;
	M_dns_t                           *tcp_dns;
	M_tls_clientctx_t                 *tcp_tls_ctx;
	M_uint64                           tcp_connect_ms;
	M_uint64                           tcp_stall_ms;
	M_uint64                           tcp_idle_ms;
	M_net_smtp_load_balance_t          load_balance_mode;
	size_t                             round_robin_idx;
	size_t                             max_number_of_attempts;
	M_list_t                          *retry_queue;
	M_list_str_t                      *internal_queue;
	M_bool                             is_external_queue_enabled;
	M_bool                             is_external_queue_pending;
	char *                           (*external_queue_get_cb)(void);
};

typedef struct {
	M_net_smtp_t  *sp;
	char          *msg;
	M_hash_dict_t *headers;
	size_t         number_of_tries;
} retry_msg_t;

typedef struct {
	size_t                 idx;
	char                  *command;
	M_list_str_t          *args;
	M_hash_dict_t         *env;
	M_uint64               timeout_ms;
	size_t                 max_processes;
} proc_endpoint_t;

typedef struct {
	size_t                 idx;
	char                  *address;
	M_uint16               port;
	M_bool                 connect_tls;
	char                  *username;
	char                  *password;
	size_t                 max_conns;
} tcp_endpoint_t;

typedef struct {
	const proc_endpoint_t *proc_endpoint;
	const tcp_endpoint_t  *tcp_endpoint;
	size_t                 slot_count;
	size_t                 slot_available;
	M_bool                 is_alive;
	M_bool                 is_failure;
	endpoint_slot_t        slots[];
} endpoint_manager_t;


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
static void process_queue_queue(M_net_smtp_t *sp); /* forward declaration */

static M_bool is_stopped(M_net_smtp_t *sp)
{
	return (
		sp->status == M_NET_SMTP_STATUS_NOENDPOINTS ||
		sp->status == M_NET_SMTP_STATUS_STOPPED
	);
}

static M_bool is_running(M_net_smtp_t *sp)
{
	return (
		sp->status == M_NET_SMTP_STATUS_PROCESSING ||
		sp->status == M_NET_SMTP_STATUS_IDLE
	);
}

static M_bool is_pending(M_net_smtp_t *sp)
{
	if (M_list_len(sp->retry_queue) > 0)
		return M_TRUE;

	if (sp->is_external_queue_enabled) {
		return sp->is_external_queue_pending;
	}
	return (M_list_str_len(sp->internal_queue) > 0);
}

static M_bool is_available_failover(M_net_smtp_t *sp)
{
	const endpoint_manager_t *epm = NULL;
	for (size_t i = 0; i < M_list_len(sp->endpoint_managers); i++) {
		epm = M_list_at(sp->endpoint_managers, i);
		if (epm->slot_available > 0) {
			return M_TRUE;
		}
	}
	return M_FALSE;
}

static M_bool is_available_round_robin(M_net_smtp_t *sp)
{
	const endpoint_manager_t *epm = NULL;
	epm = M_list_at(sp->endpoint_managers, sp->round_robin_idx);
	return (epm->slot_available > 0);
}

static M_bool is_available(M_net_smtp_t *sp)
{
	switch (sp->load_balance_mode) {
		case M_NET_SMTP_LOAD_BALANCE_FAILOVER:
			return is_available_failover(sp);
			break;
		case M_NET_SMTP_LOAD_BALANCE_ROUNDROBIN:
			return is_available_round_robin(sp);
			break;
	}
	return M_FALSE;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

M_net_smtp_t *M_net_smtp_create(M_event_t *el, const struct M_net_smtp_callbacks *cbs, void *thunk)
{
	M_net_smtp_t *sp;

	if (el == NULL)
		return NULL;

	sp = M_malloc_zero(sizeof(*sp));
	sp->el = el;

	if (cbs != NULL) {
		M_mem_move(&sp->cbs, cbs, sizeof(sp->cbs));
	}

	sp->thunk = thunk;
	sp->status = M_NET_SMTP_STATUS_NOENDPOINTS;

	sp->retry_default_ms = 300000; /* 5 min */

	sp->proc_endpoints = M_list_create(NULL, M_LIST_NONE);
	sp->tcp_endpoints = M_list_create(NULL, M_LIST_NONE);
	sp->internal_queue = M_list_str_create(M_LIST_STR_NONE);
	sp->endpoint_managers = M_list_create(NULL, M_LIST_NONE);
	sp->retry_queue = M_list_create(NULL, M_LIST_NONE);

	return sp;
}

static void destroy_proc_endpoint(const proc_endpoint_t *ep)
{
	M_free(ep->command);
	M_list_str_destroy(ep->args);
	M_hash_dict_destroy(ep->env);
}

static void destroy_tcp_endpoint(const tcp_endpoint_t *ep)
{
	M_free(ep->address);
	M_free(ep->username);
	M_free(ep->password);
}

void M_net_smtp_destroy(M_net_smtp_t *sp)
{
	if (sp == NULL)
		return;

	for (size_t i = 0; i < M_list_len(sp->proc_endpoints); i++) {
		destroy_proc_endpoint(M_list_at(sp->proc_endpoints, i));
	}
	M_list_destroy(sp->proc_endpoints, M_TRUE);
	for (size_t i = 0; i < M_list_len(sp->tcp_endpoints); i++) {
		destroy_tcp_endpoint(M_list_at(sp->tcp_endpoints, i));
	}
	M_list_destroy(sp->tcp_endpoints, M_TRUE);
	M_list_destroy(sp->retry_queue, M_TRUE);
	M_list_str_destroy(sp->internal_queue);
	/* endpoint manager should free it self once it finishes processing */
	M_list_destroy(sp->endpoint_managers, M_FALSE);
	M_tls_clientctx_destroy(sp->tcp_tls_ctx);
	M_free(sp);
}

void M_net_smtp_pause(M_net_smtp_t *sp)
{
	if (sp == NULL || !is_running(sp))
		return;

	sp->status = M_NET_SMTP_STATUS_STOPPING;
}

static void reschedule_event_cb(M_event_t *el, M_event_type_t etype, M_io_t *io, void *thunk)
{
	retry_msg_t  *retry = thunk;
	M_net_smtp_t *sp    = retry->sp;
	(void)el;
	(void)io;
	(void)etype;

	M_list_insert(sp->retry_queue, retry);
	process_queue_queue(sp);
}

static void reschedule(const endpoint_slot_t *slot)
{
	retry_msg_t               *retry          = NULL;
	M_bool                     is_requeue     = M_TRUE;
	M_net_smtp_send_failed_cb  send_failed_cb = slot->sp->cbs.send_failed_cb;
	M_net_smtp_reschedule_cb   reschedule_cb  = slot->sp->cbs.reschedule_cb;
	size_t                     num_tries;

	num_tries = slot->number_of_tries + 1;
	if (num_tries >= slot->sp->max_number_of_attempts) {
		is_requeue = M_FALSE;
	}

	if (slot->sp->is_external_queue_enabled) {
		num_tries = 0;
		is_requeue = M_FALSE;
		if (reschedule_cb != NULL) {
			reschedule_cb(slot->msg, slot->sp->retry_default_ms, slot->sp->thunk);
		}
	}

	if (NULL != send_failed_cb) {
		if (is_reqeue) {
			is_requeue = send_failed_cb(slot->headers, slot->errmsg, num_tries, M_TRUE, slot->sp->thunk);
		} else {
			send_failed_cb(slot->headers, slot->errmsg, num_tries, M_FALSE, slot->sp->thunk);
		}
	}

	if (is_requeue) {
		retry = M_malloc_zero(sizeof(*retry));
		retry->sp = slot->sp;
		retry->msg = M_strdup(slot->msg);
		retry->number_of_tries = slot->number_of_tries + 1;
		retry->headers = M_hash_dict_duplicate(slot->headers);
		M_event_timer_oneshot(slot->sp->el, slot->sp->retry_default_ms, M_TRUE, reschedule_event_cb, retry);
	}
}


static M_bool run_state_machine(endpoint_slot_t *slot)
{
	M_state_machine_status_t result;

	result = M_state_machine_run(slot->state_machine, slot);
	if (result == M_STATE_MACHINE_STATUS_WAIT || result == M_STATE_MACHINE_STATUS_DONE) {
		return M_TRUE;
	}

	M_snprintf(slot->errmsg, sizeof(slot->errmsg), "State machine failure: %d", result);
	return M_FALSE;
}

static void failure_process(endpoint_slot_t *slot, const endpoint_manager_t *epm)
{
	M_net_smtp_process_fail_cb  process_fail_cb = slot->sp->cbs.process_fail_cb;
	char                       *stdout_str      = NULL;

	if (process_fail_cb != NULL) {
		stdout_str = M_buf_finish_str(slot->out_buf, NULL);
		process_fail_cb(epm->proc_endpoint->command, slot->result_code, stdout_str, slot->errmsg, slot->sp->thunk);
		M_free(stdout_str);
	}
	M_snprintf(slot->errmsg, sizeof(slot->errmsg), "Process failure: %d", slot->result_code);
}

static void destroy_slot(M_event_t *el, endpoint_slot_t *slot)
{
	endpoint_manager_t *epm     = (endpoint_manager_t*)slot->endpoint_manager;
	M_net_smtp_sent_cb  sent_cb = slot->sp->cbs.sent_cb;

	M_buf_cancel(slot->out_buf);
	slot->out_buf = NULL;
	M_parser_destroy(slot->in_parser);
	slot->in_parser = NULL;
	M_state_machine_destroy(slot->state_machine);
	slot->state_machine = NULL;
	if (slot->is_failure) {
		if (slot->endpoint_type == PROCESS_ENDPOINT) {
			failure_process(slot, epm);
		}
		reschedule(slot);
	} else {
		if (sent_cb != NULL) {
			sent_cb(slot->headers, slot->sp->thunk);
		}
	}
	M_free(slot->msg);
	slot->msg = NULL;
	M_hash_dict_destroy(slot->headers);
	slot->headers = NULL;
	epm->slot_available++;
	process_queue_queue(slot->sp);
}


static void proc_io_cb(M_event_t *el, M_event_type_t etype, M_io_t *io, void *thunk, unsigned int connection_mask)
{
	endpoint_slot_t *slot     = thunk;
	M_io_error_t     io_error = M_IO_ERROR_SUCCESS;
	M_io_t         **slot_io  = NULL;
	size_t           len;
	(void)el;

	switch(etype) {
		case M_EVENT_TYPE_CONNECTED:
			slot->connection_mask |= connection_mask;
			break;
		case M_EVENT_TYPE_DISCONNECTED:
			goto destroy;
			break;
		case M_EVENT_TYPE_READ:
			if (connection_mask == CONNECTION_MASK_IO_STDERR) {
				io_error = M_io_read(io, (unsigned char *)slot->errmsg, sizeof(slot->errmsg) - 1, &len);
				if (io_error == M_IO_ERROR_DISCONNECT) {
					goto destroy;
				}
				if (io_error != M_IO_ERROR_SUCCESS && io_error != M_IO_ERROR_WOULDBLOCK) {
					M_snprintf(slot->errmsg, sizeof(slot->errmsg), "Read failure: %s", M_io_error_string(io_error));
					goto err;
				}
				slot->errmsg[len] = '\0';
				goto err;
			}
			if (connection_mask == CONNECTION_MASK_IO_STDOUT) {
				io_error = M_io_read_into_parser(io, slot->in_parser);
				if (io_error == M_IO_ERROR_DISCONNECT) {
					goto destroy;
				}
				if (io_error != M_IO_ERROR_SUCCESS && io_error != M_IO_ERROR_WOULDBLOCK) {
					M_snprintf(slot->errmsg, sizeof(slot->errmsg), "Read failure: %s", M_io_error_string(io_error));
				}
				goto err; /* shouldn't receive anything on stdout */
			}
			M_snprintf(slot->errmsg, sizeof(slot->errmsg), "Unexpected event: %s", M_event_type_string(etype));
			goto err;
			break;
		case M_EVENT_TYPE_WRITE:
			if (connection_mask == CONNECTION_MASK_IO_STDIN) {
				if (M_buf_len(slot->out_buf) == 0) {
					goto destroy;
				}
			} else {
				M_snprintf(slot->errmsg, sizeof(slot->errmsg), "Unexpected event: %s", M_event_type_string(etype));
				goto err;
			}
			break;
		case M_EVENT_TYPE_ERROR:
			if (connection_mask == CONNECTION_MASK_IO && slot->io_stdin == NULL) {
				/* process exit due to stdin destroyed */
				goto destroy;
			}
		case M_EVENT_TYPE_ACCEPT:
		case M_EVENT_TYPE_OTHER:
			M_snprintf(slot->errmsg, sizeof(slot->errmsg), "Unexpected event: %s", M_event_type_string(etype));
			goto err;
			break;
	}

	if (!run_state_machine(slot))
		goto err;

	io_error = M_io_write_from_buf(slot->io_stdin, slot->out_buf);
	if (io_error != M_IO_ERROR_SUCCESS && io_error != M_IO_ERROR_WOULDBLOCK) {
		M_snprintf(slot->errmsg, sizeof(slot->errmsg), "Write failed: %s", M_io_error_string(io_error));
		goto err;
	}

	return;
err:
	slot->is_failure = M_TRUE;
destroy:
	switch (connection_mask) {
		case CONNECTION_MASK_IO:        slot_io = &slot->io;        break;
		case CONNECTION_MASK_IO_STDIN:  slot_io = &slot->io_stdin;  break;
		case CONNECTION_MASK_IO_STDOUT: slot_io = &slot->io_stdout; break;
		case CONNECTION_MASK_IO_STDERR: slot_io = &slot->io_stderr; break;
	}
	if (*slot_io != NULL) {
		M_io_destroy(io);
		slot->connection_mask &= ~connection_mask;
		if (slot->connection_mask == CONNECTION_MASK_NONE) {
			destroy_slot(el, slot);
		}
	}
	*slot_io = NULL;
}

static void proc_io_stderr_cb(M_event_t *el, M_event_type_t etype, M_io_t *io, void *thunk)
{
	return proc_io_cb(el, etype, io, thunk, CONNECTION_MASK_IO_STDERR);
}

static void proc_io_stdout_cb(M_event_t *el, M_event_type_t etype, M_io_t *io, void *thunk)
{
	return proc_io_cb(el, etype, io, thunk, CONNECTION_MASK_IO_STDOUT);
}

static void proc_io_stdin_cb(M_event_t *el, M_event_type_t etype, M_io_t *io, void *thunk)
{
	return proc_io_cb(el, etype, io, thunk, CONNECTION_MASK_IO_STDIN);
}

static void proc_io_proc_cb(M_event_t *el, M_event_type_t etype, M_io_t *io, void *thunk)
{
	return proc_io_cb(el, etype, io, thunk, CONNECTION_MASK_IO);
}

static void tcp_io_cb(M_event_t *el, M_event_type_t etype, M_io_t *io, void *thunk)
{
	endpoint_slot_t          *slot          = thunk;
	M_net_smtp_t             *sp            = slot->sp;
	M_net_smtp_connect_cb     connect_cb    = sp->cbs.connect_cb;
	M_net_smtp_disconnect_cb  disconnect_cb = sp->cbs.disconnect_cb;

	M_io_error_t     io_error;
	(void)el;

	switch(etype) {
		case M_EVENT_TYPE_CONNECTED:
			slot->connection_mask |= CONNECTION_MASK_IO;
			if (connect_cb != NULL) {
				const endpoint_manager_t *epm = slot->endpoint_manager;
				connect_cb(epm->tcp_endpoint->address, epm->tcp_endpoint->port, sp->thunk);
			}
			break;
		case M_EVENT_TYPE_DISCONNECTED:
			if (disconnect_cb != NULL) {
				const endpoint_manager_t *epm = slot->endpoint_manager;
				disconnect_cb(epm->tcp_endpoint->address, epm->tcp_endpoint->port, sp->thunk);
			}
			goto destroy;
			break;
		case M_EVENT_TYPE_READ:
			io_error = M_io_read_into_parser(io, slot->in_parser);
			if (io_error != M_IO_ERROR_SUCCESS) {
				M_snprintf(slot->errmsg, sizeof(slot->errmsg), "Read failure: %s", M_io_error_string(io_error));
				goto err;
			}
			break;
		case M_EVENT_TYPE_WRITE:
			break;
		case M_EVENT_TYPE_ACCEPT:
		case M_EVENT_TYPE_ERROR:
		case M_EVENT_TYPE_OTHER:
			M_snprintf(slot->errmsg, sizeof(slot->errmsg), "Unexpected event: %s", M_event_type_string(etype));
			goto err;
			return;
	}

	if (!run_state_machine(slot))
		goto err;

	io_error = M_io_write_from_buf(io, slot->out_buf);
	if (io_error != M_IO_ERROR_SUCCESS && io_error != M_IO_ERROR_WOULDBLOCK) {
		M_snprintf(slot->errmsg, sizeof(slot->errmsg), "Write failed: %s", M_io_error_string(io_error));
		goto err;
	}

	return;
err:
	slot->is_failure = M_TRUE;
destroy:
	M_io_destroy(io);
	slot->io = NULL;
	slot->connection_mask &= ~CONNECTION_MASK_IO;
	if (slot->connection_mask == CONNECTION_MASK_NONE) {
		destroy_slot(el, slot);
	}
}

static M_bool bootstrap_proc_slot(M_net_smtp_t *sp, const proc_endpoint_t *proc_ep, endpoint_slot_t *slot)
{
	M_bool                     is_success  = M_TRUE;
	M_io_error_t               io_error    = M_IO_ERROR_SUCCESS;

	slot->endpoint_type = PROCESS_ENDPOINT;
	io_error = M_io_process_create(proc_ep->command, proc_ep->args, proc_ep->env, proc_ep->timeout_ms, &slot->io,
			&slot->io_stdin, &slot->io_stdout, &slot->io_stderr);
	slot->state_machine = smtp_flow_process();
	if (io_error == M_IO_ERROR_SUCCESS) {
		M_event_add(sp->el, slot->io, proc_io_proc_cb, slot);
		M_event_add(sp->el, slot->io_stdin, proc_io_stdin_cb, slot);
		M_event_add(sp->el, slot->io_stdout, proc_io_stdout_cb, slot);
		M_event_add(sp->el, slot->io_stderr, proc_io_stderr_cb, slot);
	} else {
		is_success = M_FALSE;
	}

	return is_success;
}

static M_bool bootstrap_tcp_slot(M_net_smtp_t *sp, const tcp_endpoint_t *tcp_ep, endpoint_slot_t *slot)
{
	M_io_error_t               io_error        = M_IO_ERROR_SUCCESS;
	M_net_smtp_iocreate_cb     iocreate_cb     = sp->cbs.iocreate_cb;
	M_bool                     is_success      = M_TRUE;

	slot->endpoint_type = TCP_ENDPOINT;
	slot->state_machine = smtp_flow_tcp();
	io_error = M_io_net_client_create(&slot->io, sp->tcp_dns, tcp_ep->address, tcp_ep->port, M_IO_NET_ANY);
	if (io_error == M_IO_ERROR_SUCCESS) {
		M_event_add(sp->el, slot->io, tcp_io_cb, slot);
		if (iocreate_cb != NULL) {
			is_success = iocreate_cb(slot->io, slot->errmsg, sizeof(slot->errmsg), sp->thunk);
		}
	} else {
		is_success = M_FALSE;
	}
	return is_success;
}

static void bootstrap_slot(M_net_smtp_t *sp, const proc_endpoint_t *proc_ep, const tcp_endpoint_t *tcp_ep,
		endpoint_slot_t *slot)
{
	M_bool          is_success = M_TRUE;
	M_email_error_t e_error    = M_EMAIL_ERROR_SUCCESS;

	slot->sp = sp;
	slot->out_buf = M_buf_create();
	slot->in_parser = M_parser_create(M_PARSER_FLAG_NONE);
	e_error = M_email_simple_split_header_body(slot->msg, &slot->headers, NULL);
	if (e_error == M_EMAIL_ERROR_SUCCESS && slot->out_buf != NULL && slot->in_parser != NULL) {
		if (proc_ep != NULL) {
			is_success = bootstrap_proc_slot(sp, proc_ep, slot);
		} else {
			is_success = bootstrap_tcp_slot(sp, tcp_ep, slot);
		}
	} else {
		is_success = M_FALSE;
	}
	if (!is_success) {
		destroy_slot(sp->el, slot);
	}
}

static void slate_msg_insert(M_net_smtp_t *sp, const endpoint_manager_t *const_epm, char *msg, size_t num_tries)
{
	endpoint_manager_t *epm  = M_CAST_OFF_CONST(endpoint_manager_t *,const_epm);
	endpoint_slot_t    *slot = NULL;
	for (size_t i = 0; i < epm->slot_count; i++) {
		slot = &epm->slots[i];
		slot->endpoint_manager = epm;
		if (slot->msg == NULL) {
			slot->msg = msg;
			slot->number_of_tries = num_tries;
			if (slot->io == NULL) {
				bootstrap_slot(sp, epm->proc_endpoint, epm->tcp_endpoint, slot);
				slot->is_alive = M_TRUE;
				epm->is_alive = M_TRUE;
			}
			epm->slot_available--;
			return;
		}
	}
}

static void slate_msg_failover(M_net_smtp_t *sp, char *msg, size_t num_tries)
{
	const endpoint_manager_t *epm = NULL;
	for (size_t i = 0; i < M_list_len(sp->endpoint_managers); i++) {
		epm = M_list_at(sp->endpoint_managers, i);
		if (epm->slot_available > 0) {
			slate_msg_insert(sp, epm, msg, num_tries);
		}
	}
}

static void slate_msg_round_robin(M_net_smtp_t *sp, char *msg, size_t num_tries)
{
	const endpoint_manager_t *epm = NULL;
	epm = M_list_at(sp->endpoint_managers, sp->round_robin_idx);
	slate_msg_insert(sp, epm, msg, num_tries);
	sp->round_robin_idx = (sp->round_robin_idx + 1) % sp->number_of_endpoints;
}

static void slate_msg(M_net_smtp_t *sp, char *msg, size_t num_tries)
{
	switch (sp->load_balance_mode) {
		case M_NET_SMTP_LOAD_BALANCE_FAILOVER:
			slate_msg_failover(sp, msg, num_tries);
			break;
		case M_NET_SMTP_LOAD_BALANCE_ROUNDROBIN:
			slate_msg_round_robin(sp, msg, num_tries);
			break;
	}
	return;
}

static void process_retry_queue(M_event_t *event, M_event_type_t type, M_io_t *io, void *cb_arg)
{
	M_net_smtp_t  *sp          = cb_arg;
	retry_msg_t   *retry       = NULL;

	/* This is an internally sent event; don't need these: */
	(void)event;
	(void)io;
	(void)type;

	if (is_available(sp)) {
		retry = M_list_take_first(sp->retry_queue);
		if (retry == NULL)
			return;
		slate_msg(sp, retry->msg, retry->number_of_tries);
		M_free(retry);
	}
}

static void process_internal_queue(M_event_t *event, M_event_type_t type, M_io_t *io, void *cb_arg)
{
	M_net_smtp_t *sp  = cb_arg;
	char         *msg = NULL;

	/* This is an internally sent event; don't need these: */
	(void)event;
	(void)io;
	(void)type;

	if (is_available(sp)) {
		msg = M_list_str_take_first(sp->internal_queue);
		if (msg == NULL)
			return;
		slate_msg(sp, msg, 0);
	}
}

static void process_external_queue(M_event_t *event, M_event_type_t type, M_io_t *io, void *cb_arg)
{
	M_net_smtp_t *sp  = cb_arg;
	char         *msg = NULL;

	/* This is an internally sent event; don't need these: */
	(void)event;
	(void)io;
	(void)type;

	if (is_available(sp)) {
		msg = sp->external_queue_get_cb();
		if (msg == NULL)
			return;

		slate_msg(sp, msg, 0);
	}
}

static M_bool idle_check_endpoint(const endpoint_manager_t *epm)
{
	const endpoint_slot_t *slot = NULL;
	for (size_t i = 0; i < epm->slot_count; i++) {
		slot = &epm->slots[i];
		if (slot->msg != NULL) {
			return M_FALSE;
		}
	}
	return M_TRUE;
}

static M_bool idle_check(M_net_smtp_t *sp)
{
	const endpoint_manager_t *epm = NULL;
	for (size_t i = 0; i < M_list_len(sp->endpoint_managers); i++) {
		epm = M_list_at(sp->endpoint_managers, i);
		if (idle_check_endpoint(epm) == M_FALSE) {
			return M_FALSE;
		}
	}
	return M_TRUE;
}

static void stop(M_net_smtp_t *sp)
{
	const endpoint_manager_t *const_epm = NULL;
	endpoint_manager_t       *epm       = NULL;
	for (size_t i = 0; i < sp->number_of_endpoints; i++) {
		const_epm = M_list_take_first(sp->endpoint_managers);
		epm = M_CAST_OFF_CONST(endpoint_manager_t*, const_epm);
		M_free(epm);
	}
	sp->status = M_NET_SMTP_STATUS_STOPPED;
	if (sp->cbs.processing_halted_cb != NULL) {
		sp->cbs.processing_halted_cb(M_FALSE, sp->thunk);
	}
}

static void process_queue_queue(M_net_smtp_t *sp)
{
	if (is_running(sp) && is_pending(sp)) {
		if (M_list_len(sp->retry_queue) > 0) {
			M_event_queue_task(sp->el, process_retry_queue, sp);
		} else if (sp->is_external_queue_enabled) {
			M_event_queue_task(sp->el, process_external_queue, sp);
		} else {
			M_event_queue_task(sp->el, process_internal_queue, sp);
		}
	} else if (idle_check(sp)) {
		if (sp->status == M_NET_SMTP_STATUS_STOPPING) {
			stop(sp);
		} else {
			sp->status = M_NET_SMTP_STATUS_IDLE;
		}
	}
}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

M_bool M_net_smtp_resume(M_net_smtp_t *sp)
{
	size_t                 proc_i   = 0;
	size_t                 tcp_i    = 0;
	const proc_endpoint_t *proc_ep  = NULL;
	const tcp_endpoint_t  *tcp_ep   = NULL;
	endpoint_manager_t    *epm      = NULL;

	if (sp == NULL)
		return M_FALSE;

	if (sp->status != M_NET_SMTP_STATUS_STOPPED) {
		if (sp->status == M_NET_SMTP_STATUS_NOENDPOINTS) {
			if (sp->cbs.processing_halted_cb != NULL) {
				sp->cbs.processing_halted_cb(M_TRUE, sp->thunk);
			}
		}
		return M_FALSE;
	}

	proc_ep = M_list_at(sp->proc_endpoints, proc_i);
	tcp_ep = M_list_at(sp->tcp_endpoints, tcp_i);
	for (size_t i = 0; i < sp->number_of_endpoints; i++) {
		if (proc_ep != NULL && proc_ep->idx == i) {
			size_t slot_count = proc_ep->max_processes;
			epm = M_malloc_zero(sizeof(*epm) + sizeof(epm->slots[0]) * slot_count);
			epm->proc_endpoint = proc_ep;
			epm->slot_count = slot_count;
			epm->slot_available = slot_count;
			M_list_insert(sp->endpoint_managers, epm);
			proc_i++;
			proc_ep = M_list_at(sp->proc_endpoints, proc_i);
			continue;
		}
		if (tcp_ep != NULL && tcp_ep->idx == i) {
			size_t slot_count = tcp_ep->max_conns;
			epm = M_malloc_zero(sizeof(*epm) + sizeof(epm->slots[0]) * slot_count);
			epm->tcp_endpoint = tcp_ep;
			epm->slot_count = slot_count;
			epm->slot_available = slot_count;
			M_list_insert(sp->endpoint_managers, epm);
			tcp_i++;
			tcp_ep = M_list_at(sp->tcp_endpoints, tcp_i);
			continue;
		}
	}

	sp->status = M_NET_SMTP_STATUS_PROCESSING;
	process_queue_queue(sp);
	return M_TRUE;
}

M_net_smtp_status_t M_net_smtp_status(const M_net_smtp_t *sp)
{
	if (sp == NULL)
		return M_NET_SMTP_STATUS_NOENDPOINTS;
	return sp->status;
}

void M_net_smtp_setup_tcp(M_net_smtp_t *sp, M_dns_t *dns, M_tls_clientctx_t *ctx)
{

	if (sp == NULL || !is_stopped(sp))
		return;

	sp->tcp_dns = dns;
	if (sp->tcp_tls_ctx != NULL)
		M_tls_clientctx_destroy(sp->tcp_tls_ctx);

	M_tls_clientctx_upref(ctx);
	sp->tcp_tls_ctx = ctx;
}

void M_net_smtp_setup_tcp_timeouts(M_net_smtp_t *sp, M_uint64 connect_ms, M_uint64 stall_ms, M_uint64 idle_ms)
{

	if (sp == NULL || !is_stopped(sp))
		return;

	sp->tcp_connect_ms = connect_ms;
	sp->tcp_stall_ms   = stall_ms;
	sp->tcp_idle_ms    = idle_ms;
}


M_bool M_net_smtp_add_endpoint_tcp(
	M_net_smtp_t *sp,
	const char   *address,
	M_uint16      port,
	M_bool        connect_tls,
	const char   *username,
	const char   *password,
	size_t        max_conns
)
{
	tcp_endpoint_t *ep = NULL;

	if (sp == NULL || !is_stopped(sp) || max_conns == 0)
		return M_FALSE;

	if (sp->tcp_dns == NULL)
		return M_FALSE;

	if (connect_tls && sp->tcp_tls_ctx == NULL)
		return M_FALSE;

	ep = M_malloc_zero(sizeof(*ep));
	ep->idx = sp->number_of_endpoints;
	ep->address = M_strdup(address);
	ep->port = port;
	ep->connect_tls = connect_tls;
	ep->username = M_strdup(username);
	ep->password = M_strdup(password);
	ep->max_conns = max_conns;
	M_list_insert(sp->tcp_endpoints, ep);
	sp->number_of_endpoints++;
	if (sp->status == M_NET_SMTP_STATUS_NOENDPOINTS) {
		sp->status = M_NET_SMTP_STATUS_STOPPED;
	}
	return M_TRUE;
}

M_bool M_net_smtp_add_endpoint_process(
	M_net_smtp_t        *sp,
	const char          *command,
	const M_list_str_t  *args,
	const M_hash_dict_t *env,
	M_uint64             timeout_ms,
	size_t               max_processes
)
{
	proc_endpoint_t *ep = NULL;

	if (sp == NULL || !is_stopped(sp) || max_processes == 0)
		return M_FALSE;

	ep = M_malloc_zero(sizeof(*ep));
	ep->idx = sp->number_of_endpoints;
	ep->command = M_strdup(command);
	ep->args = M_list_str_duplicate(args);
	ep->env = M_hash_dict_duplicate(env);
	ep->timeout_ms = timeout_ms;
	ep->max_processes = max_processes;
	M_list_insert(sp->proc_endpoints, ep);
	sp->number_of_endpoints++;
	if (sp->status == M_NET_SMTP_STATUS_NOENDPOINTS) {
		sp->status = M_NET_SMTP_STATUS_STOPPED;
	}
	return M_TRUE;
}

M_bool M_net_smtp_load_balance(M_net_smtp_t *sp, M_net_smtp_load_balance_t mode)
{
	if (sp == NULL || !is_stopped(sp))
		return M_FALSE;

	sp->load_balance_mode = mode;
	return M_TRUE;
}

void M_net_smtp_set_num_attempts(M_net_smtp_t *sp, size_t num)
{
	if (sp == NULL || !is_stopped(sp))
		return;
	sp->max_number_of_attempts = num;
}

M_list_str_t *M_net_smtp_dump_queue(M_net_smtp_t *sp)
{
	M_list_str_t *list;
	char         *msg;

	if (sp == NULL)
		return NULL;

	if (sp->is_external_queue_enabled) {
		list = M_list_str_create(M_LIST_STR_NONE);
		while ((msg = sp->external_queue_get_cb()) != NULL) {
			M_list_str_insert(list, msg);
		}
		return list;
	}

	list = sp->internal_queue;
	sp->internal_queue = M_list_str_create(M_LIST_STR_NONE);
	return list;
}

M_bool M_net_smtp_queue_smtp(M_net_smtp_t *sp, const M_email_t *e)
{
	char   *msg;
	M_bool  is_success;

	if (sp == NULL || sp->is_external_queue_enabled) 
		return M_FALSE;

	msg = M_email_simple_write(e);
	is_success = M_net_smtp_queue_message(sp, msg);
	M_free(msg);

	return is_success;
}

M_bool M_net_smtp_queue_message(M_net_smtp_t *sp, const char *msg)
{
	if (sp == NULL || sp->is_external_queue_enabled)
		return M_FALSE;

	if (!M_list_str_insert(sp->internal_queue, msg))
		return M_FALSE;

	if (sp->status == M_NET_SMTP_STATUS_IDLE) {
		sp->status = M_NET_SMTP_STATUS_PROCESSING;
		M_event_queue_task(sp->el, process_internal_queue, sp);
	}
	return M_TRUE;
}

M_bool M_net_smtp_use_external_queue(M_net_smtp_t *sp, char *(*get_cb)(void))
{
	if (sp == NULL || get_cb == NULL || M_list_str_len(sp->internal_queue) != 0)
		return M_FALSE;

	sp->is_external_queue_enabled = M_TRUE;
	sp->external_queue_get_cb = get_cb;
	return M_TRUE;
}

void M_net_smtp_external_queue_have_messages(M_net_smtp_t *sp)
{
	sp->is_external_queue_pending = M_TRUE;
	if (sp->status == M_NET_SMTP_STATUS_IDLE) {
		sp->status = M_NET_SMTP_STATUS_PROCESSING;
		M_event_queue_task(sp->el, process_external_queue, sp);
	}
}
