#include "libzbxsockets.h"

#define SO_ACCEPTCON    (1<<16)
#define SS_UNCONNECTED	1
#define SS_LISTENING	5

// supported state parameter values
static char *UNIX_STATE[] = {
	"FREE",
	"UNCONNECTED",
	"CONNECTING",
	"CONNECTED",
	"DISCONNECTING",
	"LISTEN",
};

int unix_count(AGENT_REQUEST *request, AGENT_RESULT *result)
{
	int				res = SYSINFO_RET_FAIL, count = 0;
	FILE			*f = NULL;
	char			buf[4096];

	char			*c, *param_state;
	int				i, ok, filter_state;

	void			*slot;
	int				state;
	unsigned long	flags, refcnt;

	if (2 < request->nparam) {
		SET_MSG_RESULT(result, strdup("Invalid number of parameters."));
		return res;
	}

	// validate state parameter
	filter_state = -1;
	if ((param_state = get_rparam(request, 1))) {
		for (c = param_state; c && *c; c++) {
			*c = toupper(*c);
		}
		for (i = 0; i < 6; i++) {
			if (0 == strncmp(UNIX_STATE[i], param_state, 14)) {
				filter_state = i;
				break;
			}			
			if (i == 5) {
				SET_MSG_RESULT(result, strdup("Invalid Unix socket state."));
				return res;
			}
		}
	}

	if (NULL == (f = fopen(_PATH_PROCNET_UNIX, "r"))) {
		SET_MSG_RESULT(result, zbx_dsprintf(NULL, "Failed to open " _PATH_PROCNET_UNIX ": %s", zbx_strerror(errno)));
		return res;
	}

	while (fgets(buf, sizeof(buf), f)) {
		if (4 == sscanf(buf, "%p: %lX %*X %lX %*X %X %*u %*s\n",
								&slot, &refcnt, &flags, &state)
		) {
			if (state == SS_UNCONNECTED && (flags & SO_ACCEPTCON)) {
				state = SS_LISTENING;
			}

			ok = 1;
			if (-1 < filter_state) {
				if (filter_state != state) {
					ok = 0;
				}
			}
			if (ok) {
				count++;
			}
		}
	}

	if (ferror(f)) {
		fclose(f);
		SET_MSG_RESULT(result, zbx_dsprintf(NULL, "Error reading " _PATH_PROCNET_UNIX ": %s", zbx_strerror(errno)));
		return res;
	}

	fclose(f);
	SET_UI64_RESULT(result, count);
	res = SYSINFO_RET_OK;
	return res;
}
