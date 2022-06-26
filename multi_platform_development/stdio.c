#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "so_stdio.h"
#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>

typedef struct _so_file {
	// Buffer that stores the characters returned by read
	char *read_buffer;
	int isEmpty_read;
	// Where is the next character to be read from buffer
	int cursor_read;
	// Where to stop reading from buffer
	int read_limit;

	// Buffer that stores the characters that are to be written
	char *write_buffer;
	int cursor_write;
	int write_limit;

	// last operation: 1 for write, 0 for read
	int last_op;
	// Which mode from r, w, a etc
	char *mode;
	// File descriptor
	int fd;
	// Cursor of the file descriptor
	int offset;
	int error_code;
	int eof;

	// Whose process this file is;
	int pid;
} SO_FILE;

SO_FILE *so_fopen(const char *pathname, const char *mode)
{
	SO_FILE *new_file;
	int fd;

	// Open file descriptor with the specified mode
	if (strcmp(mode, "r") == 0) {
		fd = open(pathname, O_RDONLY);
		if (fd < 0)
			return NULL;
	} else if (strcmp(mode, "r+") == 0) {
		fd = open(pathname, O_RDWR);
		if (fd < 0)
			return NULL;
	} else if (strcmp(mode, "w") == 0) {
		fd = open(pathname, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	} else if (strcmp(mode, "w+") == 0) {
		fd = open(pathname, O_RDWR | O_CREAT | O_TRUNC, 0644);
	} else if (strcmp(mode, "a") == 0) {
		fd = open(pathname, O_APPEND | O_WRONLY | O_CREAT, 0644);
	} else if (strcmp(mode, "a+") == 0) {
		fd = open(pathname, O_RDWR | O_APPEND | O_CREAT, 0644);
	} else {
		return NULL;
	}

	// Declare, initialize and alocate memory for SO_FILE's variables;
	new_file = malloc(sizeof(SO_FILE));
	new_file->mode = malloc((strlen(mode) + 1) * sizeof(char));
	new_file->read_buffer = malloc(4096 * sizeof(char));
	new_file->write_buffer = malloc(4096 * sizeof(char));
	new_file->isEmpty_read = 1;
	new_file->cursor_read = 0;
	new_file->cursor_write = 0;
	new_file->offset = 0;
	new_file->error_code = 0;
	new_file->eof = 0;
	new_file->read_limit = 0;
	new_file->write_limit = 0;
	new_file->last_op = 5;
	strcpy(new_file->mode, mode);
	new_file->fd = fd;
	return new_file;
}

int so_fclose(SO_FILE *stream)
{
	// If there are characters remained in the write_buffer,
	// flush them to output;
	int rvflush = so_fflush(stream);
	int cursor_write = stream->cursor_write;
	int rv = close(stream->fd);

	// Deallocate memory for so_file
	free(stream->mode);
	free(stream->read_buffer);
	free(stream->write_buffer);
	free(stream);
	// If flush or close have failed, return SO_EOF. Else, return 0;
	if (rvflush < 0 && cursor_write != 0) {
		stream->error_code = SO_EOF;
		return SO_EOF;
	}
	if (rv == 0)
		return 0;

	stream->error_code = SO_EOF;
	return SO_EOF;
}

int so_fileno(SO_FILE *stream)
{
	return stream->fd;
}

int so_fgetc(SO_FILE *stream)
{
	unsigned char c;
	int rv;

	stream->last_op = 0;
	// If read_buffer is empty, fill it from input;
	if (stream->isEmpty_read == 1) {
		rv = read(stream->fd, stream->read_buffer, 4096);

		stream->isEmpty_read  = 0;
		stream->read_limit = rv;
		// If read failed or reached EOF, return SO_EOF;
		if (rv <= 0) {
			stream->error_code = SO_EOF;
			stream->eof = 1;
			return SO_EOF;
		}
	}

	// If there characters in buffer, read until the limit;
	if (stream->cursor_read < stream->read_limit) {

		c = stream->read_buffer[stream->cursor_read++];

		// If it reached the limit, reset buffer;
		if (stream->cursor_read == stream->read_limit) {
			stream->isEmpty_read  = 1;
			stream->cursor_read = 0;
		}
		// Move the fd's cursor;
		stream->offset++;
	} else {
		// If it reached EOF, return EOF and reset read buffer;
		stream->isEmpty_read  = 1;
		stream->cursor_read = 0;
		stream->error_code = SO_EOF;
		return SO_EOF;
	}

	return (int)(c);
}

size_t so_fread(void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
	int aux_nmemb = nmemb;
	int cursor_ptr = 0;
	int index = 0, aux;
	char d = 0;

	stream->last_op = 0;
	if (stream->last_op == 1)
		so_fflush(stream);

	stream->last_op = 0;
	// For each element, read "size" bytes with so_fgetc;
	while (nmemb != 0) {
		for (index = 0; index < size; index++) {
			aux = so_fgetc(stream);
			// If there weren't read nmemb elements, return how
			// many elements have been read until this point;
			if (aux < 0) {
				stream->error_code = aux;
				return aux_nmemb - nmemb;
			}
			// Put byte in ptr buffer.
			d = (char)aux;
			*((char *) ptr + cursor_ptr) = d;
			cursor_ptr++;
		}
		nmemb--;
	}
	return aux_nmemb;
}

int so_fputc(int c, SO_FILE *stream)
{
	char d = (char)c;
	int rv;

	// If write buffer is full, flush it;
	if (stream->cursor_write == 4096)
		rv = so_fflush(stream);

	// Put byte in write buffer
	stream->write_buffer[stream->cursor_write++] = d;
	// Last operation was a write;
	stream->last_op = 1;
	// Move fd's cursor;
	stream->offset++;
	return c;
}

size_t so_fwrite(const void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
	int aux_nmemb = nmemb;
	int cursor_ptr = 0;
	int i, rv;

	// For each element, write "size" bytes with so_fputc;
	while (nmemb != 0) {
		for (i = 0; i < size; i++) {
			rv = so_fputc(*((int *)(ptr + cursor_ptr)), stream);
			cursor_ptr++;
		}
		nmemb--;
	}
	stream->last_op = 1;
	return aux_nmemb;
}

int so_fflush(SO_FILE *stream)
{
	// Allow flush to write only after a write operation was done;
	if (stream->last_op == 1) {
		int rv = write(stream->fd, stream->write_buffer, stream->cursor_write);

		stream->offset = lseek(stream->fd, 0, SEEK_CUR);
		stream->cursor_write = 0;
		if (rv < 0) {
			stream->error_code = SO_EOF;
			return SO_EOF;
		}
	}
	return 0;
}

int so_fseek(SO_FILE *stream, long offset, int whence)
{
	int rv;

	// Before moving cursor, flush the write buffer
	if (stream->last_op == 1)
		so_fflush(stream);

	rv = lseek(stream->fd, offset, whence);
	if (rv < 0) {
		stream->error_code = -1;
		return -1;
	}

	// If we move the cursor more than empty spaces
	//in the read buffer, reset it
	if (offset > 4096 - stream->cursor_read) {
		stream->cursor_read = 0;
		stream->isEmpty_read = 1;
	}

	stream->offset = rv;
	return 0;
}

long so_ftell(SO_FILE *stream)
{
	return stream->offset;
}

int so_feof(SO_FILE *stream)
{
	if (stream->eof == 1)
		return SO_EOF;
	else
		return 0;
}
int so_ferror(SO_FILE *stream)
{
	return stream->error_code;
}

SO_FILE *so_popen(const char *command, const char *type)
{
	SO_FILE *new_file;
	int pid;
	int *status;
	int error = 0;
	int fds[2];
	// create a unidirectional pipe
	int rv = pipe(fds);

	pid = fork();

	// If fork fails, return NULL;
	if (pid == -1)
		return NULL;

	// If popen() has "r" type, parent process's will read from the read-end of pipe
	// and the child process's standard output will be the write-end of pipe;
	// If popen() has "w" type, parent process's will write to the write-end of pipe
	// and the child process's standard input will be the rear-end of pipe;
	if (strcmp(type, "r") == 0) {
		if (pid) {
			close(fds[1]);
		} else {
			dup2(fds[1], 1);
			close(fds[0]);
		}
	} else if (strcmp(type, "w") == 0) {
		if (pid) {
			close(fds[0]);
		} else {
			dup2(fds[0], 0);
			close(fds[1]);
		}
	} else
		return NULL;
	// Let the child process exec the command;
	if (pid == 0) {
		error = execl("/bin/sh", "sh", "-c", command, NULL);
		if (error == -1)
			return NULL;
	}

	// The parent creates the SO_FILE like fopen();
	if (pid) {
		new_file = malloc(sizeof(SO_FILE));
		new_file->read_buffer = malloc(4096 * sizeof(char));
		new_file->write_buffer = malloc(4096 * sizeof(char));
		new_file->isEmpty_read = 1;
		new_file->cursor_read = 0;
		new_file->cursor_write = 0;
		new_file->offset = 0;
		new_file->error_code = 0;
		new_file->eof = 0;
		new_file->read_limit = 0;
		new_file->write_limit = 0;
		new_file->last_op = -1;
		new_file->pid = pid;

		if (strcmp(type, "r") == 0)
			new_file->fd = fds[0];
		else if (strcmp(type, "w") == 0)
			new_file->fd = fds[1];
		return new_file;
	}
	return NULL;
}
int so_pclose(SO_FILE *stream)
{
	int status;
	// Flush the remaining write buffer;
	int rv_fflush = so_fflush(stream);
	int rv = close(stream->fd);
	int pid = stream->pid;

	// Deallocate memory for so_file;
	free(stream->read_buffer);
	free(stream->write_buffer);
	free(stream);
	if (rv < 0)
		return -1;

	// Wait for child process to end;
	if (waitpid(pid, &status, WNOHANG) == -1)
		return -1;
	return 0;
}
