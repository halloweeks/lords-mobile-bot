#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <pthread.h>

// For web based graphical interface 
#include "webgui.h"

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

int epoll_fd = 0;

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
		"  client <config_file>         Load and start the bot using the specified configuration file.\n"
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
			if (time(NULL) >= c->reconnect_time) {
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
					LOGI("Reconnecting gateway server!\n");
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

#define MAX_BOTS 20

int main(int argc, const char *argv[]) {
	DispatcherInit();
	
	if (argc < 2) {
		PrintUsage();
		return 0;
	}
	
	if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
		PrintUsage();
		return 0;
	}
	
	if (strcmp(argv[1], "--create-config") == 0 || strcmp(argv[1], "-c") == 0) {
		mkdir("configs", 0775);
		
		if (!CreateDefaultProgramConfig("program.cfg")) {
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
	
	if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
		PrintVersion();
		return 0;
	}
	
	if (argc != 2) {
        printf("Usage: %s <program.cfg>\n", argv[0]);
        return EXIT_FAILURE;
    }
	
	// Load main program configuration file 
	ProgramConfig program = LoadProgramConfig(argv[1]);
	
	if (program.bot_count == 0) {
		LOGE("No bot configurations found\n");
		return 1;
	}
	
	if (program.data_path[0] == '\0') {
		LOGE("Undefined data path!\n");
		return EXIT_FAILURE;
	}	
	
	LOGI("Found %zu bot configuration\n", program.bot_count);
	
	size_t memory_used = program.bot_count * sizeof(Connection);
	
	Connection *client = calloc(program.bot_count + MAX_BOTS, sizeof(Connection));
	
	if (client == NULL) {
		LOGE("Failed to allocate memory for %zu bot configurations\n", program.bot_count);
		return 1;
	}
	
	// Display the total memory allocated for all bot connections.
	if (memory_used >= 1024 * 1024) {
		LOGI("Allocated %zu bot configurations (%.2f MB)\n", program.bot_count, (double)memory_used / (1024 * 1024));
	} else {
		LOGI("Allocated %zu bot configurations (%.2f KB)\n", program.bot_count, (double)memory_used / 1024);
	}
	
	for (size_t i = 0; i < program.bot_count; i++) {
		if (!LoadConfig(&client[i], program.config_path[i])) {
			LOGE("Failed to load bot configuration: %s\n", program.config_path[i]);
			free(client);
			return EXIT_FAILURE;
		}
		
		strcpy(client[i].bot.data_path, program.data_path);
		
		client[i].app.version_major = program.version_major;
		client[i].app.version_minor = program.version_minor;
		client[i].app.version_patch = program.version_patch;
		client[i].app.language_code = program.language_code;
		
		strcpy(client[i].gateway_server.addr, program.server_addr);
		client[i].gateway_server.port = program.server_port;
		
		client[i].reconnect_max_attempts = program.reconnect_max_attempts;
		client[i].reconnect_attempt      = 0;
		
		client[i].state = CONN_DISCONNECTED;
		
		// LOGI("Loaded bot configuration: %s\n", program.config_path[i]);
	}
	
	
	for (int i = program.bot_count; i < (program.bot_count + MAX_BOTS); i++) {
		client[i].state = CONN_RESERVED_NEW;
	}
	
	WebGuiContext webgui_ctx = {0};
	
	if (program.webgui_enabled) {
		webgui_ctx.client = client;
		webgui_ctx.client_count = program.bot_count;
		webgui_ctx.port = program.webgui_port;
		
		webgui_ctx.auth_enabled = program.webgui_auth;
		
		snprintf(webgui_ctx.username, sizeof(webgui_ctx.username), "%s", program.webgui_user);
		snprintf(webgui_ctx.password, sizeof(webgui_ctx.password), "%s", program.webgui_pass);
		
		
		
		snprintf(
			webgui_ctx.host,
			sizeof(webgui_ctx.host),
			"%s",
			program.webgui_host
		);
		
		if (!WebGuiStart(&webgui_ctx)) {
			LOGE("Failed to start Web GUI\n");
		}
	}
	
	
	
	
	LOGI("data path: '%s'\n", program.data_path);
	
	LOGI("Client version: %u.%u.%u\n", program.version_major, program.version_minor, program.version_patch);
	
	/* Create epoll */
	epoll_fd = epoll_create1(0);
	
	if (epoll_fd == -1) {
		LOGE("Failed to create epoll instance\n");
		free(client);
		return EXIT_FAILURE;
	}
	
	struct epoll_event events[64];
	
	/* Start connecting every bot */
	for (size_t i = 0; i < program.bot_count; i++) {
		if (ConnectServer(&client[i], program.server_addr, program.server_port)) {
			client[i].state = CONN_CONNECTING_GATEWAY;
			
			struct epoll_event ev = {0};
			
			ev.events = EPOLLIN | EPOLLOUT;
			ev.data.ptr = &client[i];
			
			if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client[i].sock, &ev) == -1) {
				LOGE("Failed to register socket with epoll\n");
				close(client[i].sock);
				continue;
			}
			
			LOGI("Connecting gateway server!\n");
		} else {
			LOGE("Failed to start gateway connection for bot: %s\n", program.config_path[i]);
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
		
		for (int i = 0; i < n; i++) {
			Connection *c = events[i].data.ptr;
			
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
		
		for (size_t i = 0; i < program.bot_count; i++) {
			Connection *c = &client[i];
			
			ConnectionMaintain(c);
			
			if (c->state == CONN_CONNECTED) {
				BotTick(c);
			}
		}
		
	}
	
	WebGuiStop();
	free(client);
	return 0;
}
