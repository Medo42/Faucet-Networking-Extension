/*
 * A small command line smoke test for tls_connect.
 *
 * It loads the Faucet Networking DLL and uses it through the exported API,
 * the same way Game Maker does, to perform a simple HTTPS request. The call
 * pattern is deliberately the same as the one Faucet HTTP uses: write the
 * whole request right after connecting, then once per "step" check for
 * errors, receive what is available, and keep going until end of stream.
 *
 * Usage: tlstest [--plain] <host> [port] [path]
 *
 *   --plain  use tcp_connect instead of tls_connect (default port 80)
 *
 * Put the DLL next to the executable as faucetnet.dll, or set the
 * environment variable FAUCETNET_DLL to its path.
 *
 * Build with e.g.:  gcc -o tlstest.exe tlstest.c
 *
 * Exit code: 0 if a complete HTTP response was received, 1 otherwise.
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef double (__cdecl *fn_d)(void);
typedef double (__cdecl *fn_dd)(double);
typedef double (__cdecl *fn_ddd)(double, double);
typedef double (__cdecl *fn_dddd)(double, double, double);
typedef double (__cdecl *fn_dsd)(const char *, double);
typedef double (__cdecl *fn_dds)(double, const char *);
typedef const char *(__cdecl *fn_sd)(double);
typedef const char *(__cdecl *fn_sdd)(double, double);

static HMODULE dll;

static void *get(const char *name) {
	FARPROC result = GetProcAddress(dll, name);
	if (!result) {
		fprintf(stderr, "The DLL does not export %s\n", name);
		exit(1);
	}
	return (void *) result;
}

int main(int argc, char **argv) {
	int plain = 0;
	int arg = 1;
	if (arg < argc && strcmp(argv[arg], "--plain") == 0) {
		plain = 1;
		arg++;
	}
	if (arg >= argc) {
		fprintf(stderr, "Usage: %s [--plain] <host> [port] [path]\n", argv[0]);
		return 1;
	}
	const char *host = argv[arg++];
	double port = (arg < argc) ? atof(argv[arg++]) : (plain ? 80 : 443);
	const char *path = (arg < argc) ? argv[arg++] : "/";

	const char *dllPath = getenv("FAUCETNET_DLL");
	dll = LoadLibraryA(dllPath ? dllPath : "faucetnet.dll");
	if (!dll) {
		fprintf(stderr, "Could not load the DLL (error %lu)\n", GetLastError());
		return 1;
	}

	fn_d dllStartup = (fn_d) get("dllStartup");
	fn_d dllShutdown = (fn_d) get("dllShutdown");
	fn_dsd connect = (fn_dsd) get(plain ? "tcp_connect" : "tls_connect");
	fn_dds write_string = (fn_dds) get("write_string");
	fn_dd socket_send = (fn_dd) get("socket_send");
	fn_dd socket_has_error = (fn_dd) get("socket_has_error");
	fn_sd socket_error = (fn_sd) get("socket_error");
	fn_dd tcp_receive_available = (fn_dd) get("tcp_receive_available");
	fn_dd tcp_eof = (fn_dd) get("tcp_eof");
	fn_sdd read_string = (fn_sdd) get("read_string");
	fn_d buffer_create = (fn_d) get("buffer_create");
	fn_dd buffer_size = (fn_dd) get("buffer_size");
	fn_ddd write_buffer = (fn_ddd) get("write_buffer");
	fn_dddd write_buffer_part = (fn_dddd) get("write_buffer_part");
	fn_dd socket_destroy = (fn_dd) get("socket_destroy");
	fn_dd read_ubyte = (fn_dd) get("read_ubyte");

	dllStartup();

	double socket = connect(host, port);
	double body = buffer_create();

	size_t requestSize = strlen(path) + strlen(host) + 128;
	char *request = malloc(requestSize);
	snprintf(request, requestSize,
			"GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\nAccept-Encoding:\r\n\r\n",
			path, host);
	write_string(socket, request);
	socket_send(socket);
	free(request);

	char line[4096];
	size_t lineLength = 0;
	int lineNumber = 0;
	int inBody = 0;
	int done = 0;
	int steps;

	/* 30 "steps" per second like a Game Maker game, give up after 30 seconds */
	for (steps = 0; steps < 900 && !done; steps++) {
		if (socket_has_error(socket)) {
			printf("Socket error: %s\n", socket_error(socket));
			break;
		}

		double available = tcp_receive_available(socket);
		if (!inBody) {
			double i;
			for (i = 1; i <= available && !inBody; i++) {
				char c = read_string(socket, 1)[0];
				if (c != '\n') {
					if (c != '\r' && lineLength < sizeof(line) - 1) {
						line[lineLength++] = c;
					}
					continue;
				}
				line[lineLength] = 0;
				if (lineNumber == 0) {
					printf("Status line: %s\n", line);
				} else if (lineLength == 0) {
					inBody = 1;
					write_buffer_part(body, socket, available - i);
				}
				lineLength = 0;
				lineNumber++;
			}
			if (!inBody && !available && tcp_eof(socket)) {
				printf("Unexpected EOF while receiving the headers\n");
				break;
			}
		} else {
			write_buffer(body, socket);
		}

		if (inBody && tcp_eof(socket)) {
			done = 1;
		}
		Sleep(33);
	}

	if (done) {
		printf("Header lines: %d\n", lineNumber - 2);
		printf("Body bytes: %.0f\n", buffer_size(body));
		/* FNV-1a, so that the content can be compared with what the server sent */
		unsigned int hash = 2166136261u;
		double remaining;
		for (remaining = buffer_size(body); remaining > 0; remaining--) {
			hash = (hash ^ (unsigned int) read_ubyte(body)) * 16777619u;
		}
		printf("Body checksum: %08x\n", hash);
		printf("Steps: %d\n", steps);
	} else if (steps >= 900) {
		printf("Timed out\n");
	}

	socket_destroy(socket);
	/* Give the library a moment to send the TLS close notification */
	Sleep(200);
	dllShutdown();
	return done ? 0 : 1;
}
