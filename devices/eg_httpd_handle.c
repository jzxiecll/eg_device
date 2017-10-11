




int httpd_handle_message(int conn)
{
	int err;
	int req_line_len;
	char msg_in[128];

	/* clear out the httpd_req structure */
	memset(&httpd_req, 0x00, sizeof(httpd_req));

	httpd_req.sock = conn;
	httpd_req.httpd_priv = httpd_get_priv_data(conn);
	httpd_req.free_priv_data_fn = httpd_get_free_priv_data_fn(conn);
	/* Read the first line of the HTTP header */
	req_line_len = htsys_getln_soc(conn, msg_in, sizeof(msg_in));
	if (req_line_len == 0)
		return HTTPD_DONE;

	if (req_line_len < 0) {
		httpd_e("Could not read from socket");
		return -WM_FAIL;
	}

	/* Parse the first line of the header */
	err = httpd_parse_hdr_main(msg_in, &httpd_req);
	if (err == -WM_E_HTTPD_NOTSUPP)
		/* Send 505 HTTP Version not supported */
		return httpd_send_error(conn, HTTP_505);
	else if (err != WM_SUCCESS) {
		/* Send 500 Internal Server Error */
		return httpd_send_error(conn, HTTP_500);
	}

	/* set a generic error that can be overridden by the wsgi handling. */
	httpd_d("Presetting error to: ");
	httpd_set_error("wsgi handler failed");

	/* Web Services Gateway Interface branch point:
	 * At this point we have the request type (httpd_req.type) and the path
	 * (httpd_req.filename) and all the headers waiting to be read from
	 * the socket.
	 *
	 * The call bellow will iterate through all the url patterns and
	 * invoke the handlers that match the request type and pattern.  If
	 * request type and url patern match, invoke the handler.
	 */
	err = httpd_wsgi(&httpd_req);

	if (err == HTTPD_DONE) {
		httpd_d("Done processing request.");
		return WM_SUCCESS;
	}

	/*
	 * We have not yet read the complete data from the current request, from
	 * the socket. We are in an error state and we wish to cancel this HTTP
	 * transaction. We read (flush) out all the pending data of the current
	 * request in the socket and send the appropriate error message to the
	 * client. We let the client close the socket for us, if necessary.
	 */
	httpd_purge_socket_data(&httpd_req, msg_in,
			sizeof(msg_in), conn);

	if (err == -WM_E_HTTPD_NO_HANDLER) {
		httpd_w("No handler for the given URL %s was found",
			httpd_req.filename);
		httpd_set_error("File %s not_found", httpd_req.filename);
		httpd_send_error(conn, HTTP_404);
		return WM_SUCCESS;
	} else {
		httpd_e("WSGI handler failed.");
		/* Send 500 Internal Server Error */
		return httpd_send_error(conn, HTTP_500);
	}

}

