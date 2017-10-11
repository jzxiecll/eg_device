#ifdef CONFIG_HTTPD_DEBUG
#define httpd_d(...)				\
	printf("httpd", ##__VA_ARGS__)
#else
#define httpd_d(...)
#endif /* ! CONFIG_HTTPD_DEBUG */



#ifdef CONFIG_ENABLE_ASSERTS
#define ASSERT(_cond_) if(!(_cond_)) \
		_wm_assert(__FILE__, __LINE__, #_cond_)
#else /* ! CONFIG_ENABLE_ASSERTS */
#define ASSERT(_cond_)
#endif /* CONFIG_ENABLE_ASSERTS */

void _wm_assert(const char *filename, int lineno, 
	    const char* fail_cond)
{
	//ll_printf("\n\n\r*************** PANIC *************\n\r");
	//ll_printf("Filename  : %s ( %d )\n\r", filename, lineno);
	//ll_printf("Condition : %s\n\r", fail_cond);
	//ll_printf("***********************************\n\r");
	//os_enter_critical_section();
	for( ;; );
}




static void httpd_handle_connection()
{
	int activefds_cnt, status, i;
	int max_sockfd = -1, main_sockfd = -1;
	fd_set readfds, active_readfds;
	if (httpd_stop_req) {
		httpd_d("HTTPD stop request received");
		httpd_stop_req = FALSE;
		httpd_suspend_thread(false);
	}
	FD_ZERO(&readfds);
	for (i = 0; i < MAX_HTTP_CLIENT_SOCKETS; i++) {
		if (client_sock_data[i].sockfd != -1)
			FD_SET(client_sock_data[i].sockfd, &readfds);
	}
	/* If there is no space available to hold the data for new sockets,
	 * there is no point in waiting on the http socket to accept new
	 * client connections. Else, there will be an unnecesary accept
	 * followed immediately by a close.
	 * So, we first check if the database is full and only then add
	 * the http sockets to the readfs.
	 * However, if CONFIG_ENABLE_HTTPD_HTTPD_LRU is enabled, old sockets
	 * will get purged if the database is full. So the check is
	 * done only if this config option is not set
	 */
#ifndef CONFIG_ENABLE_HTTPD_PURGE_LRU
	if (is_socket_db_full()) {
		max_sockfd = get_max_sockfd(-1);
	} else
#endif /* !CONFIG_ENABLE_HTTPD_PURGE_LRU */
	{
#ifdef CONFIG_ENABLE_HTTP_SERVER
		FD_SET(http_sockfd, &readfds);
#ifdef CONFIG_IPV6
		FD_SET(http_ipv6_sockfd, &readfds);
		max_sockfd = get_max_sockfd(http_ipv6_sockfd);
#else
		max_sockfd = get_max_sockfd(http_sockfd);
#endif /* CONFIG_IPV6 */
#endif /* CONFIG_ENABLE_HTTP_SERVER */

#ifdef CONFIG_ENABLE_HTTPS_SERVER
		FD_SET(https_sockfd, &readfds);
#ifdef CONFIG_IPV6
		FD_SET(https_ipv6_sockfd, &readfds);
		max_sockfd = get_max_sockfd(https_ipv6_sockfd);
#else
		max_sockfd = get_max_sockfd(https_sockfd);
#endif /* CONFIG_IPV6 */
#endif /* CONFIG_ENABLE_HTTPS_SERVER */
	}
	/* A negative value for timeout means that the select will never
	 * timeout. However, it will return on one of these
	 * 1) A new request received on the http server socket
	 * 2) New data received on an accepted client socket
	 * 3) Accepted client socket timed out after the keep-alive mechanism
	 * failed
	 * 4) HTTPD stop request received.
	 */
	activefds_cnt = httpd_select(max_sockfd, &readfds, &active_readfds,
						-1);
	httpd_d("Active FDs count = %d", activefds_cnt);
	if (httpd_stop_req) {
		httpd_d("HTTPD stop request received");
		httpd_stop_req = FALSE;
		httpd_suspend_thread(false);
	}
	/* If activefds_cnt < 0 , it means that httpd_select() returned an
	 * error. There is no need for any further checks and we can directly
	 * return from here.
	 */
	if (activefds_cnt < 0)
		return;
	/* If a TIMEOUT is received it means that all sockets have timed out.
	 * Hence, we close all the client sockets.
	 */
	if (activefds_cnt == HTTPD_TIMEOUT_EVENT) {
		/* Timeout has occurred */
		if (httpd_close_all_client_sockets() != WM_SUCCESS)
			httpd_suspend_thread(true);
		return;
	}
	/* First check if there is any activity on the main HTTP sockets
	 * If yes, we need to first accept the new client connection.
	 */
#ifdef CONFIG_ENABLE_HTTP_SERVER
	if (FD_ISSET(http_sockfd, &active_readfds)) {
		main_sockfd = http_sockfd;
		int sockfd = httpd_accept_client_socket(main_sockfd,
							HTTP_CONN);
		if (sockfd > 0) {
			httpd_d("Client socket accepted: %d", sockfd);
		}
	}
#ifdef CONFIG_IPV6
	if (FD_ISSET(http_ipv6_sockfd, &active_readfds)) {
		main_sockfd = http_ipv6_sockfd;
		int sockfd = httpd_accept_client_socket(main_sockfd,
							HTTP_CONN);
		if (sockfd > 0) {
			httpd_d("Client socket accepted: %d", sockfd);
		}
	}
#endif /* CONFIG_IPV6 */
#endif /* CONFIG_ENABLE_HTTP_SERVER */

#ifdef CONFIG_ENABLE_HTTPS_SERVER
	if (FD_ISSET(https_sockfd, &active_readfds) && (main_sockfd == -1)) {
		main_sockfd = https_sockfd;
		int sockfd = httpd_accept_client_socket(main_sockfd,
							HTTPS_CONN);
		if (sockfd > 0) {
			httpd_d("Client socket accepted: %d", sockfd);
		}
	}
#ifdef CONFIG_IPV6
	if (FD_ISSET(https_ipv6_sockfd, &active_readfds) &&
		(main_sockfd == -1)) {
		main_sockfd = https_ipv6_sockfd;
		int sockfd = httpd_accept_client_socket(main_sockfd,
							HTTPS_CONN);
		if (sockfd > 0) {
			httpd_d("Client socket accepted: %d", sockfd);
		}
	}
#endif /* CONFIG_IPV6 */
#endif /* CONFIG_ENABLE_HTTPS_SERVER */


	/* Check for activity on all client sockets */
	for (i = 0; i < MAX_HTTP_CLIENT_SOCKETS; i++) {
		if (client_sock_data[i].sockfd == -1)
			continue;
		if (FD_ISSET(client_sock_data[i].sockfd, &active_readfds)) {
			int client_sock = client_sock_data[i].sockfd;
			/* Take the mutex before handling the data on the
			 * socket, so that no other entity will try to use it
			 */
			httpd_get_sock_mutex(client_sock, OS_WAIT_FOREVER);
			httpd_d("Handling %d", client_sock);
			/* Note:
			 * Connection will be handled with call to
			 * httpd_handle_message twice, first for
			 * handling request (WM_SUCCESS) and second
			 * time as there is no more data to receive
			 * (client closed connection) and hence
			 * will return with status HTTPD_DONE
			 * closing socket.
			 */
			status = httpd_handle_message(client_sock);
			/* All data has been handled. Release the mutex */
			httpd_put_sock_mutex(client_sock);
			if (status == WM_SUCCESS) {
#ifdef CONFIG_ENABLE_HTTPD_PURGE_LRU
				/* We could have used
				 * httpd_update_sock_timestamp() here.
				 * But since we have access to
				 * client_sock_data[i].timestamp directly,
				 * there is no need to call the API and
				 * introduce a delay.
				 *
				 * TODO: Do something similar for
				 * httpd_put_sock_mutex and
				 * httpd_get_sock_mutex to speed up the socket
				 * handling
				 */
				client_sock_data[i].timestamp =
					os_total_ticks_get();
#endif /* !CONFIG_ENABLE_HTTPD_PURGE_LRU */
				/* The handlers are expecting more data on the
				   socket */
				continue;
			}

			/* Either there was some error or everything went well*/
			httpd_d("Close socket %d.  %s: %d", client_sock,
					status == HTTPD_DONE ?
					"Handler done" : "Handler failed",
					status);
			status = httpd_net_close(client_sock);
			if (status != WM_SUCCESS) {
				status = net_get_sock_error(client_sock);
				httpd_e("Failed to close socket %d", status);
				httpd_suspend_thread(true);
			}
			continue;
		}
	}
}
static int httpd_close_sockets()
{
	int ret, status = WM_SUCCESS;

#ifdef CONFIG_ENABLE_HTTP_SERVER
	if (http_sockfd != -1) {
		ret = net_close(http_sockfd);
		if (ret != 0) {
			httpd_d("failed to close http socket: %d",
				net_get_sock_error(http_sockfd));
			status = -WM_FAIL;
		}
		http_sockfd = -1;
	}
#ifdef CONFIG_IPV6
	if (http_ipv6_sockfd != -1) {
		ret = net_close(http_ipv6_sockfd);
		if (ret != 0) {
			httpd_d("failed to close http socket: %d",
				net_get_sock_error(http_ipv6_sockfd));
			status = -WM_FAIL;
		}
		http_ipv6_sockfd = -1;
	}
#endif /* CONFIG_IPV6 */
#endif /* CONFIG_ENABLE_HTTP_SERVER */

#ifdef CONFIG_ENABLE_HTTPS_SERVER
	if (https_sockfd != -1) {
		ret = net_close(https_sockfd);
		if (ret != 0) {
			httpd_d("failed to close https socket: %d",
				net_get_sock_error(https_sockfd));
			status = -WM_FAIL;
		}
		https_sockfd = -1;
	}
#ifdef CONFIG_IPV6
	if (https_ipv6_sockfd != -1) {
		ret = net_close(https_ipv6_sockfd);
		if (ret != 0) {
			httpd_d("failed to close http socket: %d",
				net_get_sock_error(https_ipv6_sockfd));
			status = -WM_FAIL;
		}
		https_ipv6_sockfd = -1;
	}
#endif /* CONFIG_IPV6 */
#endif /* CONFIG_ENABLE_HTTPS_SERVER */
	status = httpd_close_all_client_sockets();
	httpd_delete_all_client_mutex();
	return status;
}

static void httpd_suspend_thread(bool warn)
{
	if (warn) {
		httpd_w("Suspending thread");
	} else {
		httpd_d("Suspending thread");
	}

	httpd_close_sockets();
	httpd_state = HTTPD_THREAD_SUSPENDED;
	os_thread_self_complete(NULL);
}

static void httpd_main(os_thread_arg_t data)
{
	int status;

	status = httpd_setup_main_sockets();
	if (status != WM_SUCCESS)
		httpd_suspend_thread(true);

	while (1) {
		httpd_d("Waiting on sockets");
		httpd_handle_connection();
	}

	/*
	 * Thread will never come here. The functions called from the above
	 * infinite loop will cleanly shutdown this thread when situation
	 * demands so.
	 */
}



int httpd_start(void)
{
	int status;

	if (httpd_state != HTTPD_INIT_DONE) {
		httpd_d("Already started");
		return WM_SUCCESS;
	}

#ifdef CONFIG_ENABLE_HTTPS_SERVER
	if (!ssl_ctx) {
		httpd_d("Please set the certificates by calling "
			"httpd_use_tls_certificates()");
		return -WM_FAIL;
	}
	tls_lib_init();
#endif /* CONFIG_ENABLE_HTTPS_SERVER */

	status = os_thread_create(&httpd_main_thread, "httpd", httpd_main, 0,
			       &httpd_stack, OS_PRIO_3);
	if (status != WM_SUCCESS) {
		httpd_d("Failed to create httpd thread: %d", status);
		return -WM_FAIL;
	}

	httpd_state = HTTPD_THREAD_RUNNING;
	return WM_SUCCESS;
}

/* This pairs with httpd_start() */
int httpd_stop(void)
{
	ASSERT(httpd_state >= HTTPD_THREAD_RUNNING);
	return httpd_thread_cleanup();
}

/* This pairs with httpd_init() */
int httpd_shutdown(void)
{
	int ret;
	ASSERT(httpd_state >= HTTPD_INIT_DONE);

	httpd_d("Shutting down.");

	ret = httpd_thread_cleanup();
	if (ret != WM_SUCCESS)
		httpd_e("Thread cleanup failed");

	httpd_state = HTTPD_INACTIVE;

	return ret;
}

/* This pairs with httpd_shutdown() */
int httpd_init()
{
	int status;

	if (httpd_state != HTTPD_INACTIVE)
		return WM_SUCCESS;

	httpd_d("Initializing");
	int i;
	for (i = 0; i < MAX_HTTP_CLIENT_SOCKETS ; i++) {
		client_sock_data[i].sockfd = -1;
#ifdef CONFIG_ENABLE_HTTPD_PURGE_LRU
		client_sock_data[i].timestamp = 0;
#endif /* !CONFIG_ENABLE_HTTPD_PURGE_LRU */
		client_sock_data[i].sock_data_ptr = NULL;
		client_sock_data[i].free_priv_data_fn = NULL;
		snprintf(client_sock_data[i].mutex_name,
				sizeof(client_sock_data[i].mutex_name),
				"m%d", i);
		if (os_mutex_create(&client_sock_data[i].sock_mutex,
					client_sock_data[i].mutex_name,
					 1) != WM_SUCCESS) {
			/* If a mutex creation fails, delete all the mutexes
			 * created earlier and then return an error.
			 */
			int j;
			for (j = 0; j < i; j++) {
				os_mutex_delete
					(&client_sock_data[j].sock_mutex);
			}
			return -WM_E_NOMEM;
		}
	}
#ifdef CONFIG_ENABLE_HTTP_SERVER
	http_sockfd  = -1;
#ifdef CONFIG_IPV6
	http_ipv6_sockfd  = -1;
#endif /* CONFIG_IPV6 */
#endif /* CONFIG_ENABLE_HTTP_SERVER */
#ifdef CONFIG_ENABLE_HTTPS_SERVER
	https_sockfd  = -1;
#ifdef CONFIG_IPV6
	https_ipv6_sockfd  = -1;
#endif /* CONFIG_IPV6 */
#endif /* CONFIG_ENABLE_HTTPS_SERVER */

	status = httpd_wsgi_init();
	if (status != WM_SUCCESS) {
		httpd_d("Failed to initialize WSGI!");
		return status;
	}

	status = httpd_ssi_init();
	if (status != WM_SUCCESS) {
		httpd_d("Failed to initialize SSI!");
		return status;
	}

	httpd_state = HTTPD_INIT_DONE;

	return WM_SUCCESS;
}






























