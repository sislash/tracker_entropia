#include "parser_thread.h"
#include "parser_engine.h"
#include "core_paths.h"
#include "chatlog_path.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

static volatile int	g_stop = 0;
static int			g_running = 0;
static int			g_mode_live = 1;

static void	safe_copy_local(char *dst, size_t dstsz, const char *src)
{
	size_t	n;
	
	if (!dst || dstsz == 0)
		return;
	if (!src)
	{
		dst[0] = '\0';
		return;
	}
	n = strlen(src);
	if (n >= dstsz)
		n = dstsz - 1;
	memcpy(dst, src, n);
	dst[n] = '\0';
}

static void	log_debug(const char *fmt, ...)
{
	FILE	*f;
	va_list	ap;
	
	f = fopen("logs/parser_debug.log", "ab");
	if (!f)
		return;
	va_start(ap, fmt);
	vfprintf(f, fmt, ap);
	va_end(ap);
	fputc('\n', f);
	fclose(f);
}

static int	run_engine(void)
{
	char	chatlog[1024];
	int		rc;
	
	if (tm_ensure_logs_dir() != 0)
		return (-1);
	
	memset(chatlog, 0, sizeof(chatlog));
	if (chatlog_build_path(chatlog, sizeof(chatlog)) != 0)
	{
		/* fallback local file */
		safe_copy_local(chatlog, sizeof(chatlog), "chat.log");
	}
	log_debug("run_engine start");
	log_debug("mode=%s", g_mode_live ? "LIVE" : "REPLAY");
	log_debug("chatlog=[%s]", chatlog);
	log_debug("csv=[%s]", tm_path_hunt_csv());
	
	#ifdef DEBUG
	printf("[DEBUG] Using chatlog: [%s]\n", chatlog);
	printf("[DEBUG] CSV path     : [%s]\n", tm_path_hunt_csv());
	fflush(stdout);
	#endif
	
	if (g_mode_live)
		rc = parser_run_live(chatlog, tm_path_hunt_csv(), &g_stop);
	else
		rc = parser_run_replay(chatlog, tm_path_hunt_csv(), &g_stop);
	
	log_debug("parser_run returned=%d", rc);
	
	#ifdef DEBUG
	printf("[DEBUG] parser_run returned: %d\n", rc);
	fflush(stdout);
	#endif
	
	return (rc);
}

#ifdef _WIN32

# include <windows.h>

static HANDLE	g_th = NULL;

static DWORD WINAPI	thread_fn(LPVOID p)
{
	(void)p;
	run_engine();
	g_running = 0;
	return (0);
}

static int	start_thread(void)
{
	DWORD	id;
	
	if (g_th)
		return (-1);
	g_stop = 0;
	g_running = 1;
	g_th = CreateThread(NULL, 0, thread_fn, NULL, 0, &id);
	if (!g_th)
	{
		g_running = 0;
		return (-1);
	}
	return (0);
}

static void	join_thread(void)
{
	if (g_th)
	{
		WaitForSingleObject(g_th, INFINITE);
		CloseHandle(g_th);
		g_th = NULL;
	}
}

#else

# include <pthread.h>

static pthread_t	g_th;
static int			g_th_created = 0;

static void	*thread_fn(void *p)
{
	(void)p;
	run_engine();
	g_running = 0;
	return (NULL);
}

static int	start_thread(void)
{
	if (g_th_created)
		return (-1);
	g_stop = 0;
	g_running = 1;
	if (pthread_create(&g_th, NULL, thread_fn, NULL) != 0)
	{
		g_running = 0;
		g_th_created = 0;
		return (-1);
	}
	g_th_created = 1;
	return (0);
}

static void	join_thread(void)
{
	if (g_th_created)
	{
		pthread_join(g_th, NULL);
		g_th_created = 0;
	}
}

#endif

int	parser_thread_start_live(void)
{
	if (g_running)
		return (0);
	g_mode_live = 1;
	return (start_thread());
}

int	parser_thread_start_replay(void)
{
	if (g_running)
		return (0);
	g_mode_live = 0;
	return (start_thread());
}

void	parser_thread_stop(void)
{
	if (!g_running)
		return;
	g_stop = 1;
	join_thread();
	g_running = 0;
	g_stop = 0;
}

int	parser_thread_is_running(void)
{
	return (g_running);
}
