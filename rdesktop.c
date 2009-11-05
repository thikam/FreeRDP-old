/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   Entrypoint and utility functions
   Copyright (C) Matthew Chapman 1999-2008

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <unistd.h>		/* read close getuid getgid getpid getppid gethostname */
#include <fcntl.h>		/* open */
#include <pwd.h>		/* getpwuid */
#include <termios.h>		/* tcgetattr tcsetattr */
#include <sys/stat.h>		/* stat */
#include <sys/time.h>		/* gettimeofday */
#include <sys/times.h>		/* times */
#include <ctype.h>		/* toupper */
#include <errno.h>
#include "rdesktop.h"

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif
#ifdef HAVE_ICONV
#ifdef HAVE_LANGINFO_H
#include <langinfo.h>
#endif
#endif

#ifdef EGD_SOCKET
#include <sys/types.h>
#include <sys/socket.h>		/* socket connect */
#include <sys/un.h>		/* sockaddr_un */
#endif

#include "ssl.h"

#include "iso.h"
#include "channels.h"
#include "mcs.h"
#include "secure.h"
#include "rdp.h"
#include "cache.h"
#include "rdpset.h"
#include "lspci.h"
#include "mem.h"

rdpRdp * g_rdp = 0;
rdpSet g_settings;

char g_title[64] = "";

int g_xpos = 0;
int g_ypos = 0;
int g_pos = 0;			/* 0 position unspecified,
				   1 specified,
				   2 xpos neg,
				   4 ypos neg  */
int g_win_button_size = 0;	/* If zero, disable single app mode */
RD_BOOL g_sendmotion = True;
RD_BOOL g_packet_encryption = True;
RD_BOOL g_polygon_ellipse_orders = True;	/* polygon / ellipse orders */
RD_BOOL g_fullscreen = False;
RD_BOOL g_grab_keyboard = True;
RD_BOOL g_hide_decorations = False;
RD_BOOL g_rdpclip = True;
RD_BOOL g_numlock_sync = False;
RD_BOOL g_lspci_enabled = False;
RD_BOOL g_owncolmap = False;
RD_BOOL g_ownbackstore = True;	/* We can't rely on external BackingStore */
uint32 g_embed_wnd;

#ifdef WITH_RDPSND
RD_BOOL g_rdpsnd = False;
#endif

extern RDPDR_DEVICE g_rdpdr_device[];
extern uint32 g_num_devices;
extern char *g_rdpdr_clientname;

/* Display usage information */
static void
usage(char *program)
{
	fprintf(stderr, "rdesktop: A Remote Desktop Protocol client.\n");
	fprintf(stderr, "Version " VERSION ". Copyright (C) 1999-2008 Matthew Chapman.\n");
	fprintf(stderr, "See http://www.rdesktop.org/ for more information.\n\n");

	fprintf(stderr, "Usage: %s [options] server[:port]\n", program);
	fprintf(stderr, "   -u: user name\n");
	fprintf(stderr, "   -d: domain\n");
	fprintf(stderr, "   -s: shell\n");
	fprintf(stderr, "   -c: working directory\n");
	fprintf(stderr, "   -p: password (- to prompt)\n");
	fprintf(stderr, "   -n: client hostname\n");
	fprintf(stderr, "   -k: XKB keyboard type (ex: xfree86+aliases(qwerty))\n");
	fprintf(stderr, "   -g: desktop geometry (WxH)\n");
	fprintf(stderr, "   -f: full-screen mode\n");
	fprintf(stderr, "   -b: force bitmap updates\n");
#ifdef HAVE_ICONV
	fprintf(stderr, "   -L: local codepage\n");
#endif
	fprintf(stderr, "   -B: use BackingStore of X-server (if available)\n");
	fprintf(stderr, "   -e: disable encryption (French TS)\n");
	fprintf(stderr, "   -E: disable encryption from client to server\n");
	fprintf(stderr, "   -m: do not send motion events\n");
	fprintf(stderr, "   -C: use private colour map\n");
	fprintf(stderr, "   -D: hide window manager decorations\n");
	fprintf(stderr, "   -K: keep window manager key bindings\n");
	fprintf(stderr, "   -S: caption button size (single application mode)\n");
	fprintf(stderr, "   -T: window title\n");
	fprintf(stderr, "   -N: enable numlock syncronization\n");
	fprintf(stderr, "   -X: embed into another window with a given id.\n");
	fprintf(stderr, "   -a: connection colour depth\n");
	fprintf(stderr, "   -z: enable rdp compression\n");
	fprintf(stderr, "   -x: RDP5 experience (m[odem 28.8], b[roadband], l[an] or hex nr.)\n");
	fprintf(stderr, "   -P: use persistent bitmap caching\n");
	fprintf(stderr, "   -r: enable specified device redirection (this flag can be repeated)\n");
	fprintf(stderr,
		"         '-r comport:COM1=/dev/ttyS0': enable serial redirection of /dev/ttyS0 to COM1\n");
	fprintf(stderr, "             or      COM1=/dev/ttyS0,COM2=/dev/ttyS1\n");
	fprintf(stderr,
		"         '-r disk:floppy=/mnt/floppy': enable redirection of /mnt/floppy to 'floppy' share\n");
	fprintf(stderr, "             or   'floppy=/mnt/floppy,cdrom=/mnt/cdrom'\n");
	fprintf(stderr, "         '-r clientname=<client name>': Set the client name displayed\n");
	fprintf(stderr, "             for redirected disks\n");
	fprintf(stderr,
		"         '-r lptport:LPT1=/dev/lp0': enable parallel redirection of /dev/lp0 to LPT1\n");
	fprintf(stderr, "             or      LPT1=/dev/lp0,LPT2=/dev/lp1\n");
	fprintf(stderr, "         '-r printer:mydeskjet': enable printer redirection\n");
	fprintf(stderr,
		"             or      mydeskjet=\"HP LaserJet IIIP\" to enter server driver as well\n");
#ifdef WITH_RDPSND
	fprintf(stderr,
		"         '-r sound:[local[:driver[:device]]|off|remote]': enable sound redirection\n");
	fprintf(stderr, "                     remote would leave sound on server\n");
	fprintf(stderr, "                     available drivers for 'local':\n");
	rdpsnd_show_help();
#endif
	fprintf(stderr,
		"         '-r clipboard:[off|PRIMARYCLIPBOARD|CLIPBOARD]': enable clipboard\n");
	fprintf(stderr, "                      redirection.\n");
	fprintf(stderr,
		"                      'PRIMARYCLIPBOARD' looks at both PRIMARY and CLIPBOARD\n");
	fprintf(stderr, "                      when sending data to server.\n");
	fprintf(stderr, "                      'CLIPBOARD' looks at only CLIPBOARD.\n");
	fprintf(stderr, "   -0: attach to console\n");
	fprintf(stderr, "   -4: use RDP version 4\n");
	fprintf(stderr, "   -5: use RDP version 5 (default)\n");
}

static void
print_disconnect_reason(uint16 reason)
{
	char *text;

	switch (reason)
	{
		case exDiscReasonNoInfo:
			text = "No information available";
			break;

		case exDiscReasonAPIInitiatedDisconnect:
			text = "Server initiated disconnect";
			break;

		case exDiscReasonAPIInitiatedLogoff:
			text = "Server initiated logoff";
			break;

		case exDiscReasonServerIdleTimeout:
			text = "Server idle timeout reached";
			break;

		case exDiscReasonServerLogonTimeout:
			text = "Server logon timeout reached";
			break;

		case exDiscReasonReplacedByOtherConnection:
			text = "The session was replaced";
			break;

		case exDiscReasonOutOfMemory:
			text = "The server is out of memory";
			break;

		case exDiscReasonServerDeniedConnection:
			text = "The server denied the connection";
			break;

		case exDiscReasonServerDeniedConnectionFips:
			text = "The server denied the connection for security reason";
			break;

		case exDiscReasonLicenseInternal:
			text = "Internal licensing error";
			break;

		case exDiscReasonLicenseNoLicenseServer:
			text = "No license server available";
			break;

		case exDiscReasonLicenseNoLicense:
			text = "No valid license available";
			break;

		case exDiscReasonLicenseErrClientMsg:
			text = "Invalid licensing message";
			break;

		case exDiscReasonLicenseHwidDoesntMatchLicense:
			text = "Hardware id doesn't match software license";
			break;

		case exDiscReasonLicenseErrClientLicense:
			text = "Client license error";
			break;

		case exDiscReasonLicenseCantFinishProtocol:
			text = "Network error during licensing protocol";
			break;

		case exDiscReasonLicenseClientEndedProtocol:
			text = "Licensing protocol was not completed";
			break;

		case exDiscReasonLicenseErrClientEncryption:
			text = "Incorrect client license enryption";
			break;

		case exDiscReasonLicenseCantUpgradeLicense:
			text = "Can't upgrade license";
			break;

		case exDiscReasonLicenseNoRemoteConnections:
			text = "The server is not licensed to accept remote connections";
			break;

		default:
			if (reason > 0x1000 && reason < 0x7fff)
			{
				text = "Internal protocol error";
			}
			else
			{
				text = "Unknown reason";
			}
	}
	fprintf(stderr, "disconnect: %s.\n", text);
}

static void
rdesktop_reset_state(void)
{
	rdp_reset_state(g_rdp);
}

static RD_BOOL
read_password(char *password, int size)
{
	struct termios tios;
	RD_BOOL ret = False;
	int istty = 0;
	char *p;

	if (tcgetattr(STDIN_FILENO, &tios) == 0)
	{
		fprintf(stderr, "Password: ");
		tios.c_lflag &= ~ECHO;
		tcsetattr(STDIN_FILENO, TCSANOW, &tios);
		istty = 1;
	}

	if (fgets(password, size, stdin) != NULL)
	{
		ret = True;

		/* strip final newline */
		p = strchr(password, '\n');
		if (p != NULL)
			*p = 0;
	}

	if (istty)
	{
		tios.c_lflag |= ECHO;
		tcsetattr(STDIN_FILENO, TCSANOW, &tios);
		fprintf(stderr, "\n");
	}

	return ret;
}

static void
parse_server_and_port(char *server)
{
	char *p;
#ifdef IPv6
	int addr_colons;
#endif

#ifdef IPv6
	p = server;
	addr_colons = 0;
	while (*p)
		if (*p++ == ':')
			addr_colons++;
	if (addr_colons >= 2)
	{
		/* numeric IPv6 style address format - [1:2:3::4]:port */
		p = strchr(server, ']');
		if (*server == '[' && p != NULL)
		{
			if (*(p + 1) == ':' && *(p + 2) != '\0')
				g_settings.tcp_port_rdp = strtol(p + 2, NULL, 10);
			/* remove the port number and brackets from the address */
			*p = '\0';
			strncpy(server, server + 1, strlen(server));
		}
	}
	else
	{
		/* dns name or IPv4 style address format - server.example.com:port or 1.2.3.4:port */
		p = strchr(server, ':');
		if (p != NULL)
		{
			g_settings.tcp_port_rdp = strtol(p + 1, NULL, 10);
			*p = 0;
		}
	}
#else /* no IPv6 support */
	p = strchr(server, ':');
	if (p != NULL)
	{
		g_settings.tcp_port_rdp = strtol(p + 1, NULL, 10);
		*p = 0;
	}
#endif /* IPv6 */

}

/* Client program */
int
main(int argc, char *argv[])
{
	char fullhostname[64];
	RD_BOOL prompt_password, deactivated;
	struct passwd *pw;
	uint32 ext_disc_reason = 0;
	char *p;
	int c;
	char *locale = NULL;
	int username_option = 0;
	RD_BOOL geometry_option = False;
	int run_count = 0;	/* Session Directory support */
	RD_BOOL continue_connect = True;	/* Session Directory support */
	uint32 connect_flags;
#ifdef WITH_RDPSND
	char *rdpsnd_optarg = NULL;
#endif

#ifdef HAVE_LOCALE_H
	/* Set locale according to environment */
	locale = setlocale(LC_ALL, "");
	if (locale)
	{
		locale = xstrdup(locale);
	}

#endif

	/* setting defaults */
	memset(&g_settings, 0, sizeof(g_settings));
	g_settings.width = 1024;
	g_settings.height = 768;
	g_settings.keyboard_layout = 0x00000409; // US keyboard layout
	// g_settings.keyboard_layout = 0xE0010411; // Japanese IME
	g_settings.keyboard_type = KBD_TYPE_IBM_ENHANCED;
	// g_settings.keyboard_type = KBD_TYPE_JAPANESE;
	g_settings.keyboard_subtype = 0x0;
	// g_settings.keyboard_subtype = 0x2;
	g_settings.keyboard_functionkeys = 0xC;

	// sprintf(g_settings.keyboard_ime, "%s", "imjp81.ime");

	g_settings.encryption = True;
        g_settings.rdp_version = 5;
        g_settings.remote_app = 0;
	g_settings.server_depth = -1;
	g_settings.bitmap_cache = True;
	g_settings.bitmap_cache_precache = True;
	g_settings.bitmap_compression = True;
	g_settings.rdp5_performanceflags = RDP5_NO_WALLPAPER | RDP5_NO_FULLWINDOWDRAG |
					   RDP5_NO_MENUANIMATIONS;
	g_settings.desktop_save = True;
	g_settings.polygon_ellipse_orders = True;
	g_settings.tcp_port_rdp = TCP_PORT_RDP;

	connect_flags = RDP_LOGON_NORMAL;
	prompt_password = False;
	g_embed_wnd = 0;
	g_num_devices = 0;

	while ((c = getopt(argc, argv,
			   "Au:L:d:s:c:p:n:k:g:fbBeEmzCDKS:T:NX:a:x:Pr:045h?")) != -1)
	{
		switch (c)
		{
			case 'u':
				STRNCPY(g_settings.username, optarg, sizeof(g_settings.username));
				username_option = 1;
				break;

			case 'L':
#ifdef HAVE_ICONV
				STRNCPY(g_settings.codepage, optarg, sizeof(g_settings.codepage));
#else
				ui_error(NULL, "iconv support not available\n");
#endif
				break;

			case 'd':
				STRNCPY(g_settings.domain, optarg, sizeof(g_settings.domain));
				break;

			case 's':
				STRNCPY(g_settings.shell, optarg, sizeof(g_settings.shell));
				break;

			case 'c':
				STRNCPY(g_settings.directory, optarg, sizeof(g_settings.directory));
				break;

			case 'p':
				if ((optarg[0] == '-') && (optarg[1] == 0))
				{
					prompt_password = True;
					break;
				}

				STRNCPY(g_settings.password, optarg, sizeof(g_settings.password));
				connect_flags |= RDP_LOGON_AUTO;

				/* try to overwrite argument so it won't appear in ps */
				p = optarg;
				while (*p)
					*(p++) = 'X';
				break;

			case 'n':
				STRNCPY(g_settings.hostname, optarg, sizeof(g_settings.hostname));
				break;

			case 'k':
				p = strchr(optarg, ':');
				*p = '\0';
				p++;

				STRNCPY(g_settings.xkb_layout, optarg, sizeof(g_settings.xkb_layout));
				STRNCPY(g_settings.xkb_variant, p, sizeof(g_settings.xkb_variant));

				break;

			case 'g':
				geometry_option = True;
				g_fullscreen = False;
				if (!strcmp(optarg, "workarea"))
				{
					g_settings.width = g_settings.height = 0;
					break;
				}

				g_settings.width = strtol(optarg, &p, 10);
				if (g_settings.width <= 0)
				{
					ui_error(NULL, "invalid geometry\n");
					return 1;
				}

				if (*p == 'x')
					g_settings.height = strtol(p + 1, &p, 10);

				if (g_settings.height <= 0)
				{
					ui_error(NULL, "invalid geometry\n");
					return 1;
				}

				if (*p == '%')
				{
					g_settings.width = -g_settings.width;
					p++;
				}

				if (*p == '+' || *p == '-')
				{
					g_pos |= (*p == '-') ? 2 : 1;
					g_xpos = strtol(p, &p, 10);

				}
				if (*p == '+' || *p == '-')
				{
					g_pos |= (*p == '-') ? 4 : 1;
					g_ypos = strtol(p, NULL, 10);
				}

				break;

			case 'f':
				g_fullscreen = True;
				break;

			case 'b':
				g_settings.bitmap_cache = False;
				break;

			case 'B':
				g_ownbackstore = False;
				break;

			case 'e':
				g_settings.encryption = False;
				break;
			case 'E':
				g_packet_encryption = False;
				break;
			case 'm':
				g_sendmotion = False;
				break;

			case 'C':
				g_owncolmap = True;
				break;

			case 'D':
				g_hide_decorations = True;
				break;

			case 'K':
				g_grab_keyboard = False;
				break;

			case 'S':
				if (!strcmp(optarg, "standard"))
				{
					g_win_button_size = 18;
					break;
				}

				g_win_button_size = strtol(optarg, &p, 10);

				if (*p)
				{
					ui_error(NULL, "invalid button size\n");
					return 1;
				}

				break;

			case 'T':
				STRNCPY(g_title, optarg, sizeof(g_title));
				break;

			case 'N':
				g_numlock_sync = True;
				break;

			case 'X':
				g_embed_wnd = strtol(optarg, NULL, 0);
				break;

			case 'a':
				g_settings.server_depth = strtol(optarg, NULL, 10);
				if (g_settings.server_depth != 8 &&
				    g_settings.server_depth != 16 &&
				    g_settings.server_depth != 15 && g_settings.server_depth != 24
				    && g_settings.server_depth != 32)
				{
					ui_error(NULL, "Invalid server colour depth.\n");
					return 1;
				}
				break;

			case 'z':
				DEBUG("rdp compression enabled\n");
				connect_flags |= (RDP_LOGON_COMPRESSION |
					RDP_LOGON_COMPRESSION2);
				break;

			case 'x':
				if (str_startswith(optarg, "m"))	/* modem */
				{
					g_settings.rdp5_performanceflags =
						RDP5_NO_WALLPAPER | RDP5_NO_FULLWINDOWDRAG |
						RDP5_NO_MENUANIMATIONS | RDP5_NO_THEMING;
				}
				else if (str_startswith(optarg, "b"))	/* broadband */
				{
					g_settings.rdp5_performanceflags = RDP5_NO_WALLPAPER;
				}
				else if (str_startswith(optarg, "l"))	/* lan */
				{
					g_settings.rdp5_performanceflags = RDP5_DISABLE_NOTHING;
				}
				else
				{
					g_settings.rdp5_performanceflags = strtol(optarg, NULL, 16);
				}
				break;

			case 'P':
				g_settings.bitmap_cache_persist_enable = True;
				break;

			case 'r':

				if (str_startswith(optarg, "sound"))
				{
					optarg += 5;

					if (*optarg == ':')
					{
						optarg++;
						while ((p = next_arg(optarg, ',')))
						{
							if (str_startswith(optarg, "remote"))
								connect_flags |= RDP_LOGON_LEAVE_AUDIO;

							if (str_startswith(optarg, "local"))
#ifdef WITH_RDPSND
							{
								rdpsnd_optarg =
									next_arg(optarg, ':');
								g_rdpsnd = True;
							}

#else
								ui_warning(NULL, "Not compiled with sound support\n");
#endif

							if (str_startswith(optarg, "off"))
#ifdef WITH_RDPSND
								g_rdpsnd = False;
#else
								ui_warning(NULL, "Not compiled with sound support\n");
#endif

							optarg = p;
						}
					}
					else
					{
#ifdef WITH_RDPSND
						g_rdpsnd = True;
#else
						ui_warning(NULL, "Not compiled with sound support\n");
#endif
					}
				}
				else if (str_startswith(optarg, "disk"))
				{
					/* -r disk:h:=/mnt/floppy */
					disk_enum_devices(&g_num_devices, optarg + 4);
				}
				else if (str_startswith(optarg, "comport"))
				{
					serial_enum_devices(&g_num_devices, optarg + 7);
				}
				else if (str_startswith(optarg, "lspci"))
				{
					g_lspci_enabled = True;
				}
				else if (str_startswith(optarg, "lptport"))
				{
					parallel_enum_devices(&g_num_devices, optarg + 7);
				}
				else if (str_startswith(optarg, "printer"))
				{
					printer_enum_devices(&g_num_devices, optarg + 7);
				}
				else if (str_startswith(optarg, "clientname"))
				{
					g_rdpdr_clientname = xmalloc(strlen(optarg + 11) + 1);
					strcpy(g_rdpdr_clientname, optarg + 11);
				}
				else if (str_startswith(optarg, "clipboard"))
				{
					optarg += 9;

					if (*optarg == ':')
					{
						optarg++;

						if (str_startswith(optarg, "off"))
							g_rdpclip = False;
						else
							cliprdr_set_mode(optarg);
					}
					else
						g_rdpclip = True;
				}
				else if (strncmp("scard", optarg, 5) == 0)
				{
					ui_warning(NULL, "Not compiled with smartcard support\n");
				}
				else
				{
					ui_warning(NULL, "Unknown -r argument\n\n\tPossible arguments are: comport, disk, lptport, printer, sound, clipboard, scard\n");
				}
				break;

			case '0':
				g_settings.console_session = True;
				break;

			case '4':
                                g_settings.rdp_version = 4;
				break;

			case '5':
                                g_settings.rdp_version = 5;
				break;

			case '6':
                                g_settings.rdp_version = 6;
				break;

			case 'h':
			case '?':
			default:
				usage(argv[0]);
				return 1;
		}
	}

	if (argc - optind != 1)
	{
		usage(argv[0]);
		return 1;
	}

	STRNCPY(g_settings.server, argv[optind], sizeof(g_settings.server));
	parse_server_and_port(g_settings.server);

	if (!username_option)
	{
		int pwlen;
		pw = getpwuid(getuid());
		if ((pw == NULL) || (pw->pw_name == NULL))
		{
			ui_error(NULL, "could not determine username, use -u\n");
			return 1;
		}
		/* +1 for trailing \0 */
		pwlen = MIN(strlen(pw->pw_name) + 1, sizeof(g_settings.username));
		STRNCPY(g_settings.username, pw->pw_name, pwlen);
	}

#ifdef HAVE_ICONV
	if (g_settings.codepage[0] == 0)
	{
		if (setlocale(LC_CTYPE, ""))
		{
			STRNCPY(g_settings.codepage, nl_langinfo(CODESET),
				sizeof(g_settings.codepage));
		}
		else
		{
			STRNCPY(g_settings.codepage, DEFAULT_CODEPAGE,
				sizeof(g_settings.codepage));
		}
	}
#endif

	if (g_settings.hostname[0] == 0)
	{
		if (gethostname(fullhostname, sizeof(fullhostname)) == -1)
		{
			ui_error(NULL, "could not determine local hostname, use -n\n");
			return 1;
		}

		p = strchr(fullhostname, '.');
		if (p != NULL)
			*p = 0;

		STRNCPY(g_settings.hostname, fullhostname, sizeof(g_settings.hostname));
	}

	if (locale)
		xfree(locale);


	if (prompt_password && read_password(g_settings.password, sizeof(g_settings.password)))
		connect_flags |= RDP_LOGON_AUTO;

	if (g_title[0] == 0)
	{
		strcpy(g_title, "rdesktop - ");
		strncat(g_title, g_settings.server, sizeof(g_title) - sizeof("rdesktop - "));
	}
	g_rdp = rdp_setup(&g_settings);

	if (!ui_init())
		return 1;

#ifdef WITH_RDPSND
	if (g_rdpsnd)
	{
		if (!rdpsnd_init(rdpsnd_optarg))
		{
			ui_warning(NULL, "Initializing sound-support failed!\n");
		}
	}
#endif

	if (g_lspci_enabled)
		lspci_init();

	rdpdr_init();

	while (run_count < 2 && continue_connect)	/* add support for Session Directory; only reconnect once */
	{
		if (run_count == 0)
		{
			if (!rdp_connect(g_rdp, g_settings.server, connect_flags,
					 g_settings.domain, g_settings.password, g_settings.shell,
					 g_settings.directory, g_settings.tcp_port_rdp,
					 g_settings.username))
				return 1;
		}
		else if (!rdp_reconnect
			 (g_rdp, g_settings.server, connect_flags, g_settings.domain,
			  g_settings.password, g_settings.shell, g_settings.directory,
			  g_rdp->redirect_cookie, g_settings.tcp_port_rdp,
			  g_settings.username))
			return 1;

		/* By setting encryption to False here, we have an encrypted login 
		   packet but unencrypted transfer of other packets */
		if (!g_packet_encryption)
			g_settings.encryption = False;


		DEBUG("Connection successful.\n");
		memset(g_settings.password, 0, sizeof(g_settings.password));

		if (run_count == 0)
			if (!ui_create_window(g_rdp->inst))
				continue_connect = False;

		if (continue_connect)
			rdp_main_loop(g_rdp, &deactivated, &ext_disc_reason);

		DEBUG("Disconnecting...\n");
		rdp_disconnect(g_rdp);

		if ((g_rdp->redirect) && (run_count == 0))	/* Support for Session Directory */
		{
			/* reset state of major globals */
			rdesktop_reset_state();
			STRNCPY(g_settings.domain, g_rdp->redirect_domain, sizeof(g_settings.domain));
			STRNCPY(g_settings.username, g_rdp->redirect_username, sizeof(g_settings.username));
			STRNCPY(g_settings.password, g_rdp->redirect_password, sizeof(g_settings.password));
			STRNCPY(g_settings.server, g_rdp->redirect_server, sizeof(g_settings.server));
			connect_flags |= RDP_LOGON_AUTO;
			g_rdp->redirect= False;
		}
		else
		{
			continue_connect = False;
			ui_destroy_window();
			break;
		}

		run_count++;
	}

	cache_save_state(g_rdp->cache);
	ui_deinit(g_rdp->inst);

	if (ext_disc_reason >= 2)
		print_disconnect_reason(ext_disc_reason);

	if (deactivated)
	{
		/* clean disconnect */
		return 0;
	}
	else
	{
		if (ext_disc_reason == exDiscReasonAPIInitiatedDisconnect
		    || ext_disc_reason == exDiscReasonAPIInitiatedLogoff)
		{
			/* not so clean disconnect, but nothing to worry about */
			return 0;
		}
		else
		{
			/* return error */
			return 2;
		}
	}
}

#ifdef EGD_SOCKET
/* Read 32 random bytes from PRNGD or EGD socket (based on OpenSSL RAND_egd) */
static RD_BOOL
generate_random_egd(uint8 * buf)
{
	struct sockaddr_un addr;
	RD_BOOL ret = False;
	int fd;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd == -1)
		return False;

	addr.sun_family = AF_UNIX;
	memcpy(addr.sun_path, EGD_SOCKET, sizeof(EGD_SOCKET));
	if (connect(fd, (struct sockaddr *) &addr, sizeof(addr)) == -1)
		goto err;

	/* PRNGD and EGD use a simple communications protocol */
	buf[0] = 1;		/* Non-blocking (similar to /dev/urandom) */
	buf[1] = 32;		/* Number of requested random bytes */
	if (write(fd, buf, 2) != 2)
		goto err;

	if ((read(fd, buf, 1) != 1) || (buf[0] == 0))	/* Available? */
		goto err;

	if (read(fd, buf, 32) != 32)
		goto err;

	ret = True;

      err:
	close(fd);
	return ret;
}
#endif

/* Generate a 32-byte random for the secure transport code. */
void
generate_random(uint8 * random)
{
	struct stat st;
	struct tms tmsbuf;
	SSL_MD5 md5;
	uint32 *r;
	int fd, n;

	/* If we have a kernel random device, try that first */
	if (((fd = open("/dev/urandom", O_RDONLY)) != -1)
	    || ((fd = open("/dev/random", O_RDONLY)) != -1))
	{
		n = read(fd, random, 32);
		close(fd);
		if (n == 32)
			return;
	}

#ifdef EGD_SOCKET
	/* As a second preference use an EGD */
	if (generate_random_egd(random))
		return;
#endif

	/* Otherwise use whatever entropy we can gather - ideas welcome. */
	r = (uint32 *) random;
	r[0] = (getpid()) | (getppid() << 16);
	r[1] = (getuid()) | (getgid() << 16);
	r[2] = times(&tmsbuf);	/* system uptime (clocks) */
	gettimeofday((struct timeval *) &r[3], NULL);	/* sec and usec */
	stat("/tmp", &st);
	r[5] = st.st_atime;
	r[6] = st.st_mtime;
	r[7] = st.st_ctime;

	/* Hash both halves with MD5 to obscure possible patterns */
	ssl_md5_init(&md5);
	ssl_md5_update(&md5, random, 16);
	ssl_md5_final(&md5, random);
	ssl_md5_update(&md5, random + 16, 16);
	ssl_md5_final(&md5, random + 16);
}

/* Exit on NULL pointer. Use to verify result from XGetImage etc */
void
exit_if_null(void *ptr)
{
	if (ptr == NULL)
	{
		ui_error(NULL, "unexpected null pointer. Out of memory?\n");
		exit(1);
	}
}

/* produce a hex dump */
void
hexdump(unsigned char *p, unsigned int len)
{
	unsigned char *line = p;
	int i, thisline, offset = 0;

	while (offset < len)
	{
		printf("%04x ", offset);
		thisline = len - offset;
		if (thisline > 16)
			thisline = 16;

		for (i = 0; i < thisline; i++)
			printf("%02x ", line[i]);

		for (; i < 16; i++)
			printf("   ");

		for (i = 0; i < thisline; i++)
			printf("%c", (line[i] >= 0x20 && line[i] < 0x7f) ? line[i] : '.');

		printf("\n");
		offset += thisline;
		line += thisline;
	}
}

/*
  input: src is the string we look in for needle.
  	 Needle may be escaped by a backslash, in
	 that case we ignore that particular needle.
  return value: returns next src pointer, for
  	succesive executions, like in a while loop
	if retval is 0, then there are no more args.
  pitfalls:
  	src is modified. 0x00 chars are inserted to
	terminate strings.
	return val, points on the next val chr after ins
	0x00

	example usage:
	while( (pos = next_arg( optarg, ',')) ){
		printf("%s\n",optarg);
		optarg=pos;
	}

*/
char *
next_arg(char *src, char needle)
{
	char *nextval;
	char *p;
	char *mvp = 0;

	/* EOS */
	if (*src == (char) 0x00)
		return 0;

	p = src;
	/*  skip escaped needles */
	while ((nextval = strchr(p, needle)))
	{
		mvp = nextval - 1;
		/* found backslashed needle */
		if (*mvp == '\\' && (mvp > src))
		{
			/* move string one to the left */
			while (*(mvp + 1) != (char) 0x00)
			{
				*mvp = *(mvp + 1);
				mvp++;
			}
			*mvp = (char) 0x00;
			p = nextval;
		}
		else
		{
			p = nextval + 1;
			break;
		}

	}

	/* more args available */
	if (nextval)
	{
		*nextval = (char) 0x00;
		return ++nextval;
	}

	/* no more args after this, jump to EOS */
	nextval = src + strlen(src);
	return nextval;
}


void
toupper_str(char *p)
{
	while (*p)
	{
		if ((*p >= 'a') && (*p <= 'z'))
			*p = toupper((int) *p);
		p++;
	}
}


RD_BOOL
str_startswith(const char *s, const char *prefix)
{
	return (strncmp(s, prefix, strlen(prefix)) == 0);
}


/* Split input into lines, and call linehandler for each
   line. Incomplete lines are saved in the rest variable, which should
   initially point to NULL. When linehandler returns False, stop and
   return False. Otherwise, return True.  */
RD_BOOL
str_handle_lines(const char *input, char **rest, str_handle_lines_t linehandler, void *data)
{
	char *buf, *p;
	char *oldrest;
	size_t inputlen;
	size_t buflen;
	size_t restlen = 0;
	RD_BOOL ret = True;

	/* Copy data to buffer */
	inputlen = strlen(input);
	if (*rest)
		restlen = strlen(*rest);
	buflen = restlen + inputlen + 1;
	buf = (char *) xmalloc(buflen);
	buf[0] = '\0';
	if (*rest)
		STRNCPY(buf, *rest, buflen);
	strncat(buf, input, inputlen);
	p = buf;

	while (1)
	{
		char *newline = strchr(p, '\n');
		if (newline)
		{
			*newline = '\0';
			if (!linehandler(p, data))
			{
				p = newline + 1;
				ret = False;
				break;
			}
			p = newline + 1;
		}
		else
		{
			break;

		}
	}

	/* Save in rest */
	oldrest = *rest;
	restlen = buf + buflen - p;
	*rest = (char *) xmalloc(restlen);
	STRNCPY((*rest), p, restlen);
	xfree(oldrest);

	xfree(buf);
	return ret;
}

/* Execute the program specified by argv. For each line in
   stdout/stderr output, call linehandler. Returns false on failure. */
RD_BOOL
subprocess(char *const argv[], str_handle_lines_t linehandler, void *data)
{
	pid_t child;
	int fd[2];
	int n = 1;
	char output[256];
	char *rest = NULL;

	if (pipe(fd) < 0)
	{
		perror("pipe");
		return False;
	}

	if ((child = fork()) < 0)
	{
		perror("fork");
		return False;
	}

	/* Child */
	if (child == 0)
	{
		/* Close read end */
		close(fd[0]);

		/* Redirect stdout and stderr to pipe */
		dup2(fd[1], 1);
		dup2(fd[1], 2);

		/* Execute */
		execvp(argv[0], argv);
		perror("Error executing child");
		_exit(128);
	}

	/* Parent. Close write end. */
	close(fd[1]);
	while (n > 0)
	{
		n = read(fd[0], output, 255);
		output[n] = '\0';
		str_handle_lines(output, &rest, linehandler, data);
	}
	xfree(rest);

	return True;
}


/* not all clibs got ltoa */
#define LTOA_BUFSIZE (sizeof(long) * 8 + 1)

char *
l_to_a(long N, int base)
{
	static char ret[LTOA_BUFSIZE];

	char *head = ret, buf[LTOA_BUFSIZE], *tail = buf + sizeof(buf);

	register int divrem;

	if (base < 36 || 2 > base)
		base = 10;

	if (N < 0)
	{
		*head++ = '-';
		N = -N;
	}

	tail = buf + sizeof(buf);
	*--tail = 0;

	do
	{
		divrem = N % base;
		*--tail = (divrem <= 9) ? divrem + '0' : divrem + 'a' - 10;
		N /= base;
	}
	while (N);

	strcpy(head, tail);
	return ret;
}


int
load_licence(unsigned char **data)
{
	char *home, *path;
	struct stat st;
	int fd, length;

	home = getenv("HOME");
	if (home == NULL)
		return -1;

	path = (char *) xmalloc(strlen(home) + strlen(g_settings.hostname) + sizeof("/.rdesktop/licence."));
	sprintf(path, "%s/.rdesktop/licence.%s", home, g_settings.hostname);

	fd = open(path, O_RDONLY);
	if (fd == -1)
		return -1;

	if (fstat(fd, &st))
		return -1;

	*data = (uint8 *) xmalloc(st.st_size);
	length = read(fd, *data, st.st_size);
	close(fd);
	xfree(path);
	return length;
}

void
save_licence(unsigned char *data, int length)
{
	char *home, *path, *tmppath;
	int fd;

	home = getenv("HOME");
	if (home == NULL)
		return;

	path = (char *) xmalloc(strlen(home) + strlen(g_settings.hostname) + sizeof("/.rdesktop/licence."));

	sprintf(path, "%s/.rdesktop", home);
	if ((mkdir(path, 0700) == -1) && errno != EEXIST)
	{
		perror(path);
		return;
	}

	/* write licence to licence.hostname.new, then atomically rename to licence.hostname */

	sprintf(path, "%s/.rdesktop/licence.%s", home, g_settings.hostname);
	tmppath = (char *) xmalloc(strlen(path) + sizeof(".new"));
	strcpy(tmppath, path);
	strcat(tmppath, ".new");

	fd = open(tmppath, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	if (fd == -1)
	{
		perror(tmppath);
		return;
	}

	if (write(fd, data, length) != length)
	{
		perror(tmppath);
		unlink(tmppath);
	}
	else if (rename(tmppath, path) == -1)
	{
		perror(path);
		unlink(tmppath);
	}

	close(fd);
	xfree(tmppath);
	xfree(path);
}

/* Create the bitmap cache directory */
RD_BOOL
rd_pstcache_mkdir(void)
{
	char *home;
	char bmpcache_dir[256];

	home = getenv("HOME");

	if (home == NULL)
		return False;

	sprintf(bmpcache_dir, "%s/%s", home, ".rdesktop");

	if ((mkdir(bmpcache_dir, S_IRWXU) == -1) && errno != EEXIST)
	{
		perror(bmpcache_dir);
		return False;
	}

	sprintf(bmpcache_dir, "%s/%s", home, ".rdesktop/cache");

	if ((mkdir(bmpcache_dir, S_IRWXU) == -1) && errno != EEXIST)
	{
		perror(bmpcache_dir);
		return False;
	}

	return True;
}

/* open a file in the .rdesktop directory */
int
rd_open_file(char *filename)
{
	char *home;
	char fn[256];
	int fd;

	home = getenv("HOME");
	if (home == NULL)
		return -1;
	sprintf(fn, "%s/.rdesktop/%s", home, filename);
	fd = open(fn, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	if (fd == -1)
		perror(fn);
	return fd;
}

/* close file */
void
rd_close_file(int fd)
{
	close(fd);
}

/* read from file*/
int
rd_read_file(int fd, void *ptr, int len)
{
	return read(fd, ptr, len);
}

/* write to file */
int
rd_write_file(int fd, void *ptr, int len)
{
	return write(fd, ptr, len);
}

/* move file pointer */
int
rd_lseek_file(int fd, int offset)
{
	return lseek(fd, offset, SEEK_SET);
}

/* do a write lock on a file */
RD_BOOL
rd_lock_file(int fd, int start, int len)
{
	struct flock lock;

	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = start;
	lock.l_len = len;
	if (fcntl(fd, F_SETLK, &lock) == -1)
		return False;
	return True;
}
