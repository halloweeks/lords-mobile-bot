#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <pthread.h>
#include "webgui.h"
#include "bots.h"

#include "connection.h"
#include "log.h"
#include "net_rw.h"
#include "packet_enum.h"


#include "map_point.h"
#include "items.h"

#include "config.h"

#include "version.h"

#include <sys/epoll.h>


#include "dispatcher.h"

#include "utility.h"

#include "bot/tick.h"

#include "packet_map.h"


#include "bot/bank.h"

#define BUFFER_SIZE 4096

int epoll_fd = -1;

void ProcessConnection(Connection *c)
{
	PacketStream *s = &c->stream;
	
	while (s->read_pos - s->parse_pos >= 4) {
		s->packet_size = read_u16(s->buffer + s->parse_pos);
		s->packet_type = read_u16(s->buffer + s->parse_pos + 2);
		
		if (s->packet_size < 4 || s->packet_size > BUFFER_SIZE) {
			break;
		}
			
		if (s->read_pos - s->parse_pos < s->packet_size) {
			break;
		}
		
		DispatchPacket(c, s->packet_type, s->buffer + s->parse_pos + 4);
		
		/*
		// Debugging purpose only 
		LOGI("PACKET TYPE: %s (0x%X), size=%u\n", 
			get_packet_name(s->packet_type),
			s->packet_type,
			s->packet_size
		);
		*/
		
		/*
		if (s->packet_type == _MSG_RESP_TD_INFO || s->packet_type == _MSG_RESP_TD_TRIGGERINFO) {
			DumpData(get_packet_name(s->packet_type), s->buffer + s->parse_pos + 4, s->packet_size + 4);
		}
		
		
		if (s->packet_type == _MSG_MAGIC_GATE_DATA) {
			DumpData(get_packet_name(s->packet_type), s->buffer + s->parse_pos + 4, s->packet_size + 4);
		}
		*/
		
		
		/*
		// dump_data(get_packet_name(s->packet_type), "", s->buffer + s->parse_pos + 4, s->packet_size + 4);
		*/
		
		s->parse_pos += s->packet_size;
	}
	
	if (s->parse_pos > 0) {
		memmove(s->buffer, s->buffer + s->parse_pos, s->read_pos - s->parse_pos);
		s->read_pos -= s->parse_pos;
		s->parse_pos = 0;
	}
}

void PrintUsage(void)
{
	printf(
		"Lords Mobile Bot\n"
		"\n"
		"Usage:\n"
		"  client [config_file]         Load and start the bots (default: program.cfg, created on first run).\n"
		"  client --create-config, -c   Create a default configuration file.\n"
		"  client --help, -h            Display this help message.\n"
		"  client --version, -v         Display version information.\n"
		"\n"
		"Project:\n"
		"  https://github.com/halloweeks/lords-mobile-bot\n"
	);
}

void PrintVersion(void)
{
    printf(
        "Lords Mobile Bot v%s\n"
        "Build: %s %s\n"
        "Project: https://github.com/halloweeks/lords-mobile-bot\n",
        VERSION,
        __DATE__,
        __TIME__);
}

void ConnectionMaintain(Connection *c)
{
	switch (c->state) {
		case CONN_CONNECTED_GATEWAY:
			LOGI("Gateway connected\n");
			
			RequestGuestLogIn(c);
			
			c->state = CONN_WAIT_GATEWAY_LOGIN;
			
			// remove epoll out writable , that trigger event continuously 
			struct epoll_event ev1 = {0};
			
			ev1.events = EPOLLIN;
			ev1.data.ptr = c;
			
			if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, c->sock, &ev1) == -1) {
				LOGE("Failed to remove EPOLLOUT\n");
			}
			break;
		case CONN_CONNECTED_GAME:
			LOGI("Game server connected\n");
			RequestLogIn(c);
			RequestClientInitOver(c);
			// c->state = CONN_WAIT_GAME_LOGIN;
			c->state = CONN_CONNECTED;
			
			c->action_state = 1; // start schedule tasks
			
			// remove epoll out writable , that trigger event continuously 
			struct epoll_event ev = {0};
			
			ev.events = EPOLLIN;
			ev.data.ptr = c;
			
			if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, c->sock, &ev) == -1) {
				LOGE("Failed to remove EPOLLOUT\n");
			}
			
			c->reconnect_attempt = 0;
			
			break;
		case CONN_GATEWAY_LOGIN_SUCCESS: 
			// 1. Tear down gateway fd
			epoll_ctl(epoll_fd, EPOLL_CTL_DEL, c->sock, NULL);
			close(c->sock);
			c->sock = -1;
			
			// 2. Reset the stream — gateway leftovers are meaningless to game parser
			memset(&c->stream, 0, sizeof(c->stream));
			
			// 3. Open new socket to game server (this sets c->sock internally)
			if (!ConnectServer(c, c->game_server.addr, c->game_server.port)) {
				LOGE("Failed to start game server connection\n");
				c->state = CONN_DISCONNECTED;
				return;
			}
			
			LOGI("Game server: %s:%u\n", c->game_server.addr, c->game_server.port);
			
			
			c->state = CONN_CONNECTING_GAME;
				
			// 4. Register the NEW fd with epoll, same Connection* as data.ptr
			if (!epoll_register(epoll_fd, c, EPOLLIN | EPOLLOUT)) {
				LOGE("Failed to register game socket with epoll\n");
				close(c->sock);
				c->sock = -1;
				c->state = CONN_DISCONNECTED;
				return;
			}
			
			break;
		case CONN_GATEWAY_LOGIN_FAILED: 
			epoll_ctl(epoll_fd, EPOLL_CTL_DEL, c->sock, NULL);
			close(c->sock);
			c->sock = -1;
			break;
		case CONN_DISCONNECTED:
			if (c->enabled && time(NULL) >= c->reconnect_time) {
				if (c->reconnect_max_attempts != 0 &&
					c->reconnect_attempt >= c->reconnect_max_attempts) {
					LOGE("Maximum reconnect attempts reached\n");
					break;
				}
				
				c->protocol.seq_id = 0;
				c->stream.read_pos = 0;
				c->stream.parse_pos = 0;
				
				memset(&c->stream, 0, sizeof(c->stream));
				
				if (ConnectServer(c, c->gateway_server.addr, c->gateway_server.port)) {
					struct epoll_event ev = {0};
					ev.events = EPOLLIN | EPOLLOUT;
					ev.data.ptr = c;
					
					if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, c->sock, &ev) == -1) {
						LOGE("Failed to register socket with epoll\n");
						close(c->sock);
						break;
					}
					
					c->state = CONN_CONNECTING_GATEWAY;
					LOGI("Connecting gateway server!\n");
				} else {
					LOGE("Failed to reconnect gateway\n");
					c->reconnect_time = time(NULL) + 15;
				}
			}
			break;
		default:
			break; // connected / logging in / running — nothing to do here 
    }
}

// For web based GUI currently under construction 
// #include "bot/http_server.h"

int main(int argc, const char *argv[]) {
	DispatcherInit();
	
	const char *arg = argc >= 2 ? argv[1] : "";
	
	if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0) {
		PrintUsage();
		return 0;
	}
	
	if (strcmp(arg, "--create-config") == 0 || strcmp(arg, "-c") == 0) {
		mkdir("configs", 0775);
		
		if (!CreateDefaultProgramConfig("program.cfg", true)) {
			LOGE("Failed to create 'program.cfg'\n");
			return EXIT_FAILURE;
		}
		
		if (!CreateDefaultConfig("configs/bank123.cfg")) {
			LOGE("Failed to create 'configs/bank123.cfg'\n");
			return EXIT_FAILURE;
		}
		
		LOGI("Created 'program.cfg'\n");
		LOGI("Created 'configs/bank123.cfg'\n");
		
		return EXIT_SUCCESS;
	}
	
	if (strcmp(arg, "--version") == 0 || strcmp(arg, "-v") == 0) {
		PrintVersion();
		return 0;
	}
	
	if (argc > 2) {
        printf("Usage: %s [program.cfg]\n", argv[0]);
        return EXIT_FAILURE;
    }
	
	const char *program_path = argc == 2 ? argv[1] : "program.cfg";
	
	// First run: no program.cfg yet. Create one so the Web GUI can start
	// and bots can be added from the browser.
	if (argc < 2 && access(program_path, F_OK) != 0) {
		mkdir("configs", 0775);
		
		if (!CreateDefaultProgramConfig(program_path, false)) {
			LOGE("Failed to create '%s'\n", program_path);
			return EXIT_FAILURE;
		}
		
		LOGI("No configuration found, created '%s'\n", program_path);
	}
	
	// Load main program configuration file 
	ProgramConfig program = LoadProgramConfig(program_path);
	
	if (program.bot_count == 0 && !program.webgui_enabled) {
		LOGE("No bot configurations found in '%s' (set webgui.enabled = true to add bots from the browser)\n", program_path);
		return 1;
	}
	
	if (program.data_path[0] == '\0') {
		LOGE("Undefined data path!\n");
		return EXIT_FAILURE;
	}
	
	LOGI("Found %u bot configuration\n", (unsigned)program.bot_count);
	
	BotList bots;
	BotsInit(&bots, &program, program_path);
	
	for (size_t i = 0; i < program.bot_count; i++) {
		if (!BotsLoad(&bots, program.config_path[i])) {
			LOGE("Failed to load bot configuration: %s\n", program.config_path[i]);
			BotsDestroy(&bots);
			return EXIT_FAILURE;
		}
	}
	
	LOGI("data path: '%s'\n", program.data_path);
	
	LOGI("Client version: %u.%u.%u\n", program.version_major, program.version_minor, program.version_patch);
	
	/* Create epoll */
	epoll_fd = epoll_create1(0);
	
	if (epoll_fd == -1) {
		LOGE("Failed to create epoll instance\n");
		BotsDestroy(&bots);
		return EXIT_FAILURE;
	}
	
	struct epoll_event events[64];
	
	/* Bots with enabled=true are connected by ConnectionMaintain() below. */
	
	WebGuiContext webgui_ctx = {0};
	
	if (program.webgui_enabled) {
		webgui_ctx.bots = &bots;
		webgui_ctx.port = program.webgui_port;
		webgui_ctx.auth_enabled = program.webgui_auth;
		
		snprintf(webgui_ctx.username, sizeof(webgui_ctx.username), "%s", program.webgui_user);
		snprintf(webgui_ctx.password, sizeof(webgui_ctx.password), "%s", program.webgui_pass);
		snprintf(webgui_ctx.host, sizeof(webgui_ctx.host), "%s", program.webgui_host);
		
		if (!WebGuiStart(&webgui_ctx)) {
			LOGE("Failed to start Web GUI\n");
			
			if (bots.count == 0) {
				BotsDestroy(&bots);
				return EXIT_FAILURE;
			}
		}
	}
	
	while (1) {
		int n = epoll_wait(epoll_fd, events, 64, 1000);
		
		if (n == -1) {
			if (errno == EINTR)
				continue;
			
			/* Unexpected error. */
			LOGE("epoll_wait() failed\n");
			break;
		}
		
		// The Web GUI thread may add/stop/remove bots while we wait above,
		// so everything below runs with the bot list locked.
		BotsLock(&bots);
		
		for (int i = 0; i < n; i++) {
			Connection *c = events[i].data.ptr;
			
			// Event for a bot that was stopped or removed from the Web GUI
			if (c->sock < 0) {
				continue;
			}
			
			if (events[i].events & (EPOLLERR | EPOLLHUP)) {
				LOGE("Socket disconnected\n");
				
				c->reconnect_time = time(NULL) + 15;			
				
				epoll_ctl(epoll_fd, EPOLL_CTL_DEL, c->sock, NULL);
				close(c->sock);
				c->sock = -1;
				c->state = CONN_DISCONNECTED;
				continue;
			}
			
			if (events[i].events & EPOLLOUT) {
				int err = 0;
				socklen_t len = sizeof(err);
				
				if (getsockopt(c->sock,
					SOL_SOCKET,
					SO_ERROR,
					&err,
					&len) == -1 || err != 0) {
					
					LOGE("Connection failed\n");
					
					c->reconnect_time = time(NULL) + 15;
					
					epoll_ctl(epoll_fd, EPOLL_CTL_DEL, c->sock, NULL);
					close(c->sock);
					c->sock = -1;
					c->state = CONN_DISCONNECTED;
					
					continue;
				}
				
				switch (c->state) {
					case CONN_CONNECTING_GATEWAY:
						c->state = CONN_CONNECTED_GATEWAY;
						break;
					case CONN_CONNECTING_GAME:
						c->state = CONN_CONNECTED_GAME;
						break;
					default:
						break;
				}
			}
			
			
			/* Incoming packets */
			if (events[i].events & EPOLLIN) {
				ssize_t ret = recv(c->sock,
					c->stream.buffer + c->stream.read_pos,
					BUFFER_SIZE - c->stream.read_pos,
				0);
				
				if (ret <= 0) {
					LOGE("Connection closed");
					
					epoll_ctl(epoll_fd, EPOLL_CTL_DEL, c->sock, NULL);
					close(c->sock);
					c->sock = -1;
					c->state = CONN_DISCONNECTED;
					
					c->reconnect_time = time(NULL) + 15;
					
					continue;
				}
				
				c->stream.read_pos += ret;
				
				ProcessConnection(c);
			}
		}
		
		for (size_t i = 0; i < bots.count; i++) {
			Connection *c = bots.bot[i];
			
			ConnectionMaintain(c);
			
			if (c->state == CONN_CONNECTED) {
				BotTick(c);
			}
		}
		
		BotsReap(&bots);
		BotsUnlock(&bots);
	}
	
	WebGuiStop();
	BotsDestroy(&bots);
	return 0;
}
