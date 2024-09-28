/* SPDX-License-Identifier: MIT */

/* StACSOS - shell
 *
 * Copyright (c) University of St Andrews 2024
 * Tom Spink <tcs6@st-andrews.ac.uk>
 */
#include <stacsos/console.h>
#include <stacsos/memops.h>
#include <stacsos/process.h>
using namespace stacsos;

const int MAX_HISTORY = 10;
const int MAX_COMMAND_SIZE = 128;
static void run_command(const char *cmd)
{
	// printf("Running Command: %s\n", cmd);

	char prog[64];
	int n = 0;
	while (*cmd && *cmd != ' ' && n < 63) {
		prog[n++] = *cmd++;
	}

	prog[n] = 0;

	if (*cmd)
		cmd++;

	auto pcmd = process::create(prog, cmd);
	if (!pcmd) {
		console::get().writef("error: unable to run program '%s'\n", prog);
	} else {
		pcmd->wait_for_exit();
	}
}

int main(void)
{
	console::get().write("This is the StACSOS shell.  Path resolution is \e\x0fnot-yet-implemented\e\x07, so you\n"
						 "must type the command EXACTLY.\n\n");

	console::get().write("Use the cat program to view the README: /usr/cat /docs/README\n\n");

	char command_history[MAX_HISTORY][MAX_COMMAND_SIZE];
	int history_len = 0;

	while (true) {
		console::get().write("> ");
		char command_buffer[MAX_COMMAND_SIZE];
		int n = 0;
		int history_pos = -1;
		while (n < MAX_COMMAND_SIZE - 1) {
			unsigned char c = console::get().read_char();
			if (c == 0)
				continue;
			if (c == '\n')
				break;
			if (c == 0x80) {
				continue;
			} // left
			if (c == 0x81) {
				continue;
			} // right
			if (c == 0x82 || c == 0x83) { // up and down
				if (history_pos < history_len - 1 && c == 0x82)
					history_pos++; // up
				if (history_pos > 0 && c == 0x83)
					history_pos--; // down
				while (n != 0) { // Clear the last line
					console::get().write("\b");
					n--;
				}
				software_based_memops::memcpy(((char *)command_buffer), (char *)command_history[history_len - history_pos - 1], MAX_COMMAND_SIZE);
				n = memops::strlen(command_buffer);
				console::get().write(command_buffer);
				continue;
			}

			if (c == '\b') {
				if (n > 0) {
					command_buffer[--n] = 0;
					console::get().write("\b");
				}
			} else {
				command_buffer[n++] = c;
				console::get().writef("%c", c);
			}
		}

		console::get().write("\n");
		if (n == 0)
			continue;

		command_buffer[n] = 0;

		if (memops::strcmp("exit", command_buffer) == 0)
			break;

		if (history_len < MAX_HISTORY) {
			software_based_memops::memcpy(((char *)command_history[history_len]), (char *)command_buffer, MAX_COMMAND_SIZE);
			history_len++;
		} else { //If history full, shuffle array to drop the last element and add new element
			software_based_memops::memcpy(((char *)command_history[0]), (char *)command_history[1], MAX_COMMAND_SIZE * (MAX_HISTORY - 1));
			software_based_memops::memcpy(((char *)command_history[history_len - 1]), (char *)command_buffer, MAX_COMMAND_SIZE);
		}
		run_command(command_buffer);
	}

	return 0;
}
