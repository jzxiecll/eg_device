



int htsys_getln_soc(int sd, char *data_p, int buflen)
{
	int len = 0;
	char *c_p;
	int result;
	int serr;

	ASSERT(data_p != NULL);
	c_p = data_p;

	/* Read one byte at a time */
	while ((result = httpd_recv(sd, c_p, 1, 0)) != 0) {
		/* error on recv */
		if (result < 0) {
			*c_p = 0;
			serr = net_get_sock_error(sd);
			if ((serr == -WM_E_NOMEM) || (serr == -WM_E_AGAIN))
				continue;
			httpd_e("recv failed len: %d, err: %d", len, serr);
			return -WM_FAIL;
		}

		/* If new line... */
		if ((*c_p == ISO_nl) || (*c_p == ISO_cr)) {
			result = httpd_recv(sd, c_p, 1, 0);
			if ((*c_p != ISO_nl) && (*c_p != ISO_cr)) {
				httpd_e("should get double CR LF: %d, %d",
				      (int)*c_p, result);
			}
			break;
		}
		len++;
		c_p++;

		/* give up here since we'll at least need 3 more chars to
		 * finish off the line */
		if (len >= buflen - 1) {
			httpd_e("buf full: recv didn't read complete line.");
			break;
		}
	}

	*c_p = 0;
	return len;
}

