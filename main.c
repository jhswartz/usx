#include <libusb-1.0/libusb.h>
#include <netinet/in.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "command.h"
#include "parse.h"
#include "frame.h"

libusb_device_handle *Handle = NULL;

uint32_t Timeout = 3000;
uint16_t BlockSize = 512;
uint16_t Vendor = 0;
uint16_t Product = 0;
uint16_t Interface = 0;
uint16_t Input = 0;
uint16_t Output = 0;
uint32_t Partition = 0;
uint32_t BaseAddress = 0;

bool Interactive = true;
bool Verbose = true;

static int initialise(void);
static void interact(void);
static void cleanup(void);

static void serveCommandsRequest();
static void serveSilentRequest();
static void serveVerboseRequest();
static void serveQuitRequest();
static void serveDeviceRequest(char *);
static void serveDeviceShowRequest();
static void serveOpenRequest();
static void serveCloseRequest();
static void serveGreetRequest();
static void serveConnectRequest();
static void serveResetRequest();
static void serveFramingRequest(char *);
static void serveSendRequest(char *);
static void serveExecuteRequest();

static int sendFile(char *, uint32_t);
static long determineFileSize(FILE *);
static int startDataTransfer(uint32_t, uint32_t);
static int transferData(uint8_t *, size_t);
static int endDataTransfer(void);

static int exchange(struct Frame *, struct Frame **);
static int transmit(uint8_t *, size_t);
static int receive(uint8_t *, size_t, int *);
static void dump(uint8_t *, size_t, FILE *);

static struct Command Commands[] = 
{
	{ "?\n",        serveCommandsRequest },
	{ "silent\n",   serveSilentRequest },
	{ "verbose\n",  serveVerboseRequest },
	{ "quit\n",     serveQuitRequest },
	{ "device ",    serveDeviceRequest },
	{ "device?\n",  serveDeviceShowRequest },
	{ "open\n",     serveOpenRequest },
	{ "close\n",    serveCloseRequest },
	{ "greet\n",    serveGreetRequest },
	{ "connect\n",  serveConnectRequest },
	{ "reset\n",    serveResetRequest },
	{ "framing ",   serveFramingRequest },
	{ "send ",      serveSendRequest },
	{ "execute\n",  serveExecuteRequest },
};

static const size_t CommandCount = sizeof(Commands) / sizeof(*Commands);

int main(int argc, char *argv[])
{
	if (initialise() == -1)
	{
		return EXIT_FAILURE;
	}

	interact();

	return EXIT_SUCCESS;
}

static int initialise(void)
{
	int result = libusb_init(NULL);

	if (result < 0)
	{
		fprintf(stderr, "%s\n\n", libusb_strerror(result));
		return -1;
	}

	return 0;
}

static void interact(void)
{
	char buffer[BUFSIZ];

	while (Interactive)
	{
		prompt("usx");

		if (readCommand(buffer, sizeof(buffer)) == -1)
		{
			break;
		}

		parseCommand(Commands, CommandCount, buffer);
	}

	cleanup();
}

static void serveCommandsRequest(void)
{
	printf("  verbose                     Be verbose\n"
	       "  silent                      Be silent\n"
	       "  quit                        Quit usx\n"
	       "\n"
	       "  device VID PID IF IN OUT    Set device parameters\n"
	       "  device?                     Show device parameters\n"
	       "  open                        Open device\n"
	       "  close                       Close device\n"
	       "  greet                       Greet device\n"                      
	       "  connect                     Connect to device\n"
	       "  reset                       Reset device\n"
	       "\n"
	       "  framing MODE                Select bootrom or fdl mode\n"
	       "  send FILE ADDRESS           Send file to address\n"
	       "  execute ADDRESS             Execute code at address\n\n");
}

static void serveSilentRequest()
{
	Verbose = false;
}

static void serveVerboseRequest()
{
	Verbose = true;
}

static void serveQuitRequest()
{
	Interactive = false;
}

static void serveDeviceRequest(char *cursor)
{
	uint16_t vendor = 0;
	uint16_t product = 0;
	uint16_t interface = 0;
	uint16_t input = 0;
	uint16_t output = 0;

	if (parseUInt16(&cursor, &vendor) == -1)
	{
		fprintf(stderr, "Invalid USB vendor identifier\n\n");
	}

	if (parseUInt16(&cursor, &product) == -1)
	{
		fprintf(stderr, "Invalid USB product identifier\n\n");
	}

	if (parseUInt16(&cursor, &interface) == -1)
	{
		fprintf(stderr, "Invalid device interface\n\n");
	}

	if (parseUInt16(&cursor, &input) == -1)
	{
		fprintf(stderr, "Invalid input endpoint\n\n");
	}

	if (parseUInt16(&cursor, &output) == -1)
	{
		fprintf(stderr, "Invalid output endpoint\n\n");
	}

	Vendor = vendor;
	Product = product;
	Interface = interface;
	Input = input;
	Output = output;
}

static void serveDeviceShowRequest()
{
	printf("  Vendor     %04x\n",   Vendor);
	printf("  Product    %04x\n",   Product);
	printf("  Interface  %02x\n",   Interface);
	printf("  Input      %02x\n",   Input);
	printf("  Output     %02x\n\n", Output);
}

static void serveOpenRequest()
{
	int result = -1;

	if (Handle != NULL)
	{
		fprintf(stderr, "Device already open\n\n");
		return;
	}

	Handle = libusb_open_device_with_vid_pid(NULL, Vendor, Product);

	if (Handle == NULL)
	{
		fprintf(stderr, "Failed to open device\n\n");
		return;
	}

	result = libusb_claim_interface(Handle, Interface);

	if (result < 0)
	{
		fprintf(stderr, "%s\n\n", libusb_strerror(result));
		libusb_close(Handle);
		Handle = NULL;
		return;
	}

	result = libusb_control_transfer(Handle, 0x21, 34,
	                                         Output << 8 | 1, 0,
	                                         NULL, 0, Timeout);

	if (result < 0)
	{
		fprintf(stderr, "%s\n\n", libusb_strerror(result));
		libusb_release_interface(Handle, 0);
		libusb_close(Handle);
		Handle = NULL;
		return;
	}
}

static void serveCloseRequest()
{
	if (Handle == NULL)
	{
		fprintf(stderr, "Device not open\n\n");
		return;
	}

	libusb_release_interface(Handle, 0);
	libusb_close(Handle);
	Handle = NULL;
}

static void serveGreetRequest()
{
	uint8_t request[] = { FRAME_DELIMITER };
	struct Frame *response = NULL;

	if (transmit(request, sizeof(request)) == -1)
	{
		return;
	}

	if (receiveFrame(receive, &response) == -1)
	{
		return;
	}

	dumpFrame(response);

	if (response->type != Banner)
	{
		deallocateFrame(response);
		return;
	}

	deallocateFrame(response);
}

static void serveConnectRequest()
{
	struct Frame  request  = { .type = Connect };
	struct Frame *response = NULL;

	if (Handle == NULL)
	{
		fprintf(stderr, "Device not open\n\n");
		return;
	}

	if (exchange(&request, &response) == -1)
	{
		return;
	}

	if (response->type != Acknowledgement)
	{
		deallocateFrame(response);
		return;
	}

	deallocateFrame(response);
}

static void serveResetRequest()
{
	struct Frame  request  = { .type = Reset };
	struct Frame *response = NULL;

	if (Handle == NULL)
	{
		fprintf(stderr, "Device not open\n\n");
		return;
	}

	if (exchange(&request, &response) == -1)
	{
		return;
	}

	if (response->type != Acknowledgement)
	{
		deallocateFrame(response);
		return;
	}

	deallocateFrame(response);
}

static void serveFramingRequest(char *cursor)
{
	if (matchToken(&cursor, "bootrom") == 0)
	{
		selectBootROMFraming();
	}

	else if (matchToken(&cursor, "fdl") == 0)
	{
		selectFDLFraming();
	}

	else
	{
		fprintf(stderr, "Invalid framing mode\n\n");
	}
}

static void serveSendRequest(char *cursor)
{
	char *filename = NULL;
	uint32_t address = 0;

	if (parseFilename(&cursor, &filename) == -1)
	{
		fprintf(stderr, "Invalid filename\n\n");
		return;
	}

	if (parseUInt32(&cursor, &address) == -1)
	{
		fprintf(stderr, "Invalid address\n\n");
		return;
	}

	sendFile(filename, address);
}

static void serveExecuteRequest()
{
	struct Frame  request  = { .type = ExecuteData, };
	struct Frame *response = NULL;

	if (exchange(&request, &response) == -1)
	{
		return;
	}

	if (response->type != Acknowledgement)
	{
		deallocateFrame(response);
		return;
	}

	deallocateFrame(response);
}

static int sendFile(char *filename, uint32_t address)
{
	FILE *stream = NULL;
	long remaining = 0;
	uint8_t buffer[BlockSize * 2];
	size_t length = 0;

	stream = fopen(filename, "r");

	if (stream == NULL)
	{
		fprintf(stderr, "%s\n\n", strerror(errno));
		return -1;
	}

	remaining = determineFileSize(stream);

	if (remaining == -1)
	{
		fclose(stream);
		return -1;
	}

	if (startDataTransfer(address, remaining) == -1)
	{
		fclose(stream);
		return -1;
	}

	while (remaining > 0)
	{
		length = fread(buffer, 1, sizeof(buffer),stream);

		if (ferror(stream))
		{
			fprintf(stderr, "%s\n\n", strerror(errno));
			fclose(stream);
			return -1;
		}

		if (transferData(buffer, length) == -1)
		{
			fclose(stream);
			return -1;
		}

		remaining -= length;
	}

	if (endDataTransfer() == -1)
	{
		fclose(stream);
		return -1;
	}

	fclose(stream);
	return 0;
}

static long determineFileSize(FILE *stream)
{
	long size = 0;

	if (fseek(stream, 0, SEEK_END) == -1)
	{
		ERROR(strerror(errno));
		return -1;
	}

	size = ftell(stream);

	if (size == -1)
	{
		ERROR(strerror(errno));
		return -1;
	}

	if (fseek(stream, 0, SEEK_SET) == -1)
	{
		ERROR(strerror(errno));
		return -1;
	}

	return size;
}

static int startDataTransfer(uint32_t destination, uint32_t size)
{
	uint32_t data[] = { htonl(destination), htonl(size) };

	struct Frame request =
	{
		.type     = StartDataTransfer,
		.dataSize = sizeof(data),
		.data     = (uint8_t *)data
	};

	struct Frame *response = NULL;

	if (exchange(&request, &response) == -1)
	{
		return -1;
	}

	if (response->type != Acknowledgement)
	{
		deallocateFrame(response);
		return -1;
	}

	deallocateFrame(response);
	return 0;
}

static int transferData(uint8_t *buffer, size_t size)
{
	struct Frame request =
	{
		.type = DataTransfer,
		.dataSize = size,
		.data = buffer
	};

	struct Frame *response = NULL;

	if (exchange(&request, &response) == -1)
	{
		return -1;
	}

	if (response->type != Acknowledgement)
	{
		deallocateFrame(response);
		return -1;
	}

	deallocateFrame(response);
	return 0;
}

static int endDataTransfer(void)
{
	struct Frame  request  = { .type = EndDataTransfer };
	struct Frame *response = NULL;

	if (exchange(&request, &response) == -1)
	{
		return -1;
	}

	if (response->type != Acknowledgement)
	{
		deallocateFrame(response);
		return -1;
	}

	deallocateFrame(response);
	return 0;
}

static int exchange(struct Frame *request, struct Frame **response)
{
	if (transmitFrame(request, transmit) == -1)
	{
		return -1;
	}

	if (Verbose)
	{
		dumpFrame(request);
	}

	if (receiveFrame(receive, response) == -1)
	{
		return -1;
	}

	if (Verbose)
	{
		dumpFrame(*response);
	}

	return 0;
}

static int transmit(uint8_t *buffer, size_t length)
{
	int result = 0;
	int count = 0;

	if (Handle == NULL)
	{
		fprintf(stderr, "Device not open\n\n");
		return -1;
	}

	if (Verbose)
	{
		printf("TX\n");
		dump(buffer, length, stdout);
	}

	while (count < length)
	{
		result = libusb_bulk_transfer(Handle, Output,
		                              buffer + count,
		                              length - count,
		                              &count, Timeout);

		if (result < 0)
		{
			fprintf(stderr, "%s\n\n", libusb_strerror(result));
			return -1;
		}
	}

	return 0;
}

static int receive(uint8_t *buffer, size_t size, int *length)
{
	if (Handle == NULL)
	{
		fprintf(stderr, "Device not open\n\n");
		return -1;
	}

	memset(buffer, 0, size);

	int result = libusb_bulk_transfer(Handle, Input,
	                                  buffer, size,
	                                  length, Timeout);

	if (result < 0)
	{
		fprintf(stderr, "%s\n\n", libusb_strerror(result));
		return -1;
	}

	if (Verbose)
	{
		printf("RX\n");
		dump(buffer, *length, stdout);
	}	

	return 0;
}

static void dump(uint8_t *buffer, size_t length, FILE *stream)
{
	for (int offset = 0; offset < length; offset += 16)
	{
		int remaining = 16;

		if (offset + 16 >= length)
		{
			remaining = length - offset;
		}

		fprintf(stream, "  %08x: ", offset);

		for (int index = 0; index < 16; index++)
		{
			if (index < remaining)
			{
				uint8_t byte = buffer[offset + index];
				fprintf(stream, "%02x", byte);
			}

			else
			{
				fprintf(stream, "  ");
			}

			if (index % 2)
			{
				fputc(' ', stream);
			}
		}

		fputc(' ', stream);

		for (int index = 0; index < remaining; index++)
		{
			uint8_t byte = buffer[offset + index];
			fputc(isprint(byte) ? byte : '.', stream);
		}

		fputc('\n', stream);
	}

	fputc('\n', stream);
}

static void cleanup(void)
{
	if (Handle != NULL)
	{
		libusb_release_interface(Handle, 0);
		libusb_close(Handle);
		Handle = NULL;
	}

	libusb_exit(NULL);
}
