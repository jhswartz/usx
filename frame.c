#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "frame.h"

static void checkBootROMData(uint8_t *data, size_t length, uint16_t *checksum)
{
	static uint16_t lookup[] =
	{
		0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
		0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
		0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
		0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
		0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
		0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
		0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
		0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
	
		0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
		0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
		0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
		0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
		0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
		0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
		0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
		0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
	
		0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
		0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
		0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
		0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
		0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
		0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
		0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
		0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
	
		0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
		0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
		0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
		0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
		0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
		0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
		0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
		0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
	};

	for (size_t index = 0; index < length; index++)
	{
		uint8_t byte = data[index];
		*checksum = (*checksum << 8) ^ lookup[*checksum >> 8 ^ byte];
	}
}

static
void checkFDLData(uint8_t *data, size_t length, uint32_t *checksum, bool final)
{
	for (size_t index = 0; index < length; index += 2)
	{
		if (index < length - 1)
		{
			*checksum += (data[index] << 8) | data[index + 1];
		}

		else
		{
			*checksum += data[index];
		}
	}

	if (final)
	{
		*checksum  = (*checksum >> 16) + (*checksum & 0xffff);
		*checksum += (*checksum >> 16);
		*checksum  = ~*checksum & 0xffff;
	}
}

static void checkBootROMFrame(struct Frame *frame)
{
	uint8_t type[]     = {frame->type     >> 8, frame->type};
	uint8_t dataSize[] = {frame->dataSize >> 8, frame->dataSize};

	checkBootROMData(type, sizeof(type), &frame->checksum);
	checkBootROMData(dataSize, sizeof(dataSize), &frame->checksum);
	checkBootROMData(frame->data, frame->dataSize, &frame->checksum);
}

static void checkFDLFrame(struct Frame *frame)
{
	uint32_t checksum   = 0;
	uint8_t  type[]     = {frame->type     >> 8, frame->type};
	uint8_t  dataSize[] = {frame->dataSize >> 8, frame->dataSize};

	checkFDLData(type, sizeof(type), &checksum, false);
	checkFDLData(dataSize, sizeof(dataSize), &checksum, false);
	checkFDLData(frame->data, frame->dataSize, &checksum, true);

	frame->checksum = checksum;
}

static void (*checkFrame)(struct Frame *) = checkBootROMFrame;

void selectBootROMFraming(void)
{
	checkFrame = checkBootROMFrame;
}

void selectFDLFraming(void)
{
	checkFrame = checkFDLFrame;
}

static void serialiseByte(uint8_t byte, uint8_t **cursor)
{
	if (byte == 0x7d || byte == 0x7e)
	{
		*(*cursor)++ = 0x7d;
		*(*cursor)++ = byte ^ 0x20;
	}

	else
	{
		*(*cursor)++ = byte;
	}
}

static void serialiseUInt16(uint16_t integer, uint8_t **cursor)
{
	serialiseByte(integer >> 8, cursor);
	serialiseByte(integer, cursor);
}

static void serialiseData(uint8_t *data, uint16_t dataSize, uint8_t **cursor)
{
	for (size_t index = 0; index < dataSize; index++)
	{
		serialiseByte(data[index], cursor);
	}
}

static void serialiseChecksum(struct Frame *frame, uint8_t **cursor)
{
	checkFrame(frame);
	serialiseUInt16(frame->checksum, cursor);
}

static int serialiseFrame(struct Frame *frame, uint8_t **buffer, int *length)
{
	uint8_t *cursor = NULL;

	if (frame == NULL)
	{
		ERROR("NULL frame");
		*length = 0;
		return -1;
	}

	*buffer = calloc(1, (MINIMUM_FRAME_SIZE + frame->dataSize) * 2);
	cursor = *buffer;

	if (cursor == NULL)
	{
		ERROR(strerror(errno));
		*length = 0;
		return -1;
	}

	*cursor++ = FRAME_DELIMITER;
	serialiseUInt16(frame->type, &cursor);
	serialiseUInt16(frame->dataSize, &cursor);
	serialiseData(frame->data, frame->dataSize, &cursor);
	serialiseChecksum(frame, &cursor);
	*cursor++ = FRAME_DELIMITER;

	*length = cursor - *buffer;
	return 0;
}

static void deserialiseByte(uint8_t **cursor, uint8_t *byte)
{
	if (**cursor == 0x7d)
	{
		(*cursor)++;
		*byte = *(*cursor)++ ^ 0x20;
	}

	else
	{
		*byte = *(*cursor)++;
	}
}

static void deserialiseUInt16(uint8_t **cursor, uint16_t *integer)
{
	deserialiseByte(cursor, (uint8_t *)integer + 1);
	deserialiseByte(cursor, (uint8_t *)integer);
}

static void deserialiseData(uint8_t **cursor, uint16_t dataSize, uint8_t *data)
{
	for (size_t index = 0; index < dataSize; index++)
	{
		deserialiseByte(cursor, data + index);
	}
}

static int deserialiseFrame(uint8_t *buffer, int length, struct Frame **frame)
{
	uint16_t checksum = 0;
	uint8_t *cursor = buffer;

	if (buffer == NULL)
	{
		ERROR("NULL buffer");
		return -1;
	}

	if (length < MINIMUM_FRAME_SIZE)
	{
		ERROR("length < MINIMUM_FRAME_SIZE");
		return -1;
	}

	if (*cursor++ != FRAME_DELIMITER)
	{
		ERROR("Missing first FRAME_DELIMITER");
		return -1;
	}

	*frame = calloc(1, sizeof(struct Frame));

	if (*frame == NULL)
	{
		ERROR(strerror(errno));
		return -1;
	}

	deserialiseUInt16(&cursor, &(*frame)->type);
	deserialiseUInt16(&cursor, &(*frame)->dataSize);

	if ((*frame)->dataSize > 0)
	{
		if ((*frame)->dataSize > length - MINIMUM_FRAME_SIZE)
		{
			ERROR("Data underflow");
			deallocateFrame(*frame);
			return -1;
		}

		(*frame)->data = calloc(1, (*frame)->dataSize);

		if ((*frame)->data == NULL)
		{
			ERROR(strerror(errno));
			deallocateFrame(*frame);
			return -1;
		}

		deserialiseData(&cursor, (*frame)->dataSize, (*frame)->data);
	}

	checkFrame(*frame);
	deserialiseUInt16(&cursor, &checksum);

	if (checksum != (*frame)->checksum)
	{
		ERROR("checksum mismatch");
		deallocateFrame(*frame);
		return -1;
	}

	if (*cursor != FRAME_DELIMITER)
	{
		ERROR("Missing last FRAME_DELIMITER");
		deallocateFrame(*frame);
		return -1;
	}

	return 0;
}

void deallocateFrame(struct Frame *frame)
{
	if (frame)
	{
		if (frame->data)
		{
			free(frame->data);
		}

		free(frame);
	}
}

int transmitFrame(struct Frame *frame, int (*tx)(uint8_t *, size_t))
{
	int result = 0;
	int length = 0;
	uint8_t *buffer = NULL;

	if (serialiseFrame(frame, &buffer, &length) == -1)
	{
		return -1;
	}

	result = tx(buffer, length);
	free(buffer);
	return result;
}

int receiveFrame(int (*rx)(uint8_t *, size_t, int *), struct Frame **frame)
{
	int length = 0;
	uint8_t buffer[BUFSIZ];

	if (rx(buffer, sizeof(buffer), &length) == -1)
	{
		return -1;
	}

	return deserialiseFrame(buffer, length, frame);
}

void dumpFrame(struct Frame *frame)
{
	char *label = "Unknown";

	static char *frameTypeLabels[] =
	{
		[Connect]             = "Connect",
		[StartDataTransfer]   = "Start Data Transfer",
		[DataTransfer]        = "Data Transfer",
		[EndDataTransfer]     = "End Data Transfer",
		[ExecuteData]         = "Execute Data",
		[Reset]               = "Reset",
		[ReadFlash]           = "Read Flash",
		[EraseFlash]          = "Erase Flash",
		[ReadFlashType]       = "Read Flash Type",
		[ReadFlashInfo]       = "Read Flash Info",
		[EnableFlash]         = "Enable Flash",
		[Acknowledgement]     = "Acknowledgement",
		[Banner]              = "Banner",
		[DestinationError]    = "Destination Error",
		[SizeError]           = "Size Error",
		[VerificationError]   = "Verification Error",
		[ReadFlashResponse]   = "Read Flash Response",
		[VerificationFailure] = "Verification Failure"
	};

	if (frame)
	{
		if (frameTypeLabels[frame->type] != NULL)
		{
			label = frameTypeLabels[frame->type];
		}

		printf("  Frame Type:    %04x (%s)\n", frame->type, label);
		printf("  Data Size:     %04x\n",   frame->dataSize);
		printf("  Checksum:      %04x\n\n", frame->checksum);
	}
}
