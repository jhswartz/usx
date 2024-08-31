#ifndef FRAME_H
#define FRAME_H

#include <stddef.h>
#include <stdint.h>

#define ERROR(message) fprintf(stderr, "%s: %s\n", __func__, message)

#define FRAME_DELIMITER '~' 
#define MINIMUM_FRAME_SIZE 8

struct Frame
{
	uint16_t  type;
	uint16_t  dataSize;
	uint8_t  *data;
	uint16_t  checksum;
};

enum FrameType
{
	Connect             = 0x00,
	StartDataTransfer   = 0x01,
	DataTransfer        = 0x02,
	EndDataTransfer     = 0x03,
	ExecuteData         = 0x04,
	Reset               = 0x05,
	ReadFlash           = 0x06,
	EraseFlash          = 0x0a,
	ReadFlashType       = 0x0c,
	ReadFlashInfo       = 0x0d,
	EnableFlash         = 0x1b,
	Acknowledgement     = 0x80,
	Banner              = 0x81,
	DestinationError    = 0x89,
	SizeError           = 0x8a,
	VerificationError   = 0x8b,
	ReadFlashResponse   = 0x93,
	VerificationFailure = 0xa6
};

void selectBootROMFraming(void);
void selectFDLFraming(void);

int transmitFrame(struct Frame *frame, int (*tx)(uint8_t *, size_t));
int receiveFrame(int (*rx)(uint8_t *, size_t, int *), struct Frame **frame);

void deallocateFrame(struct Frame *frame);
void dumpFrame(struct Frame *frame);

#endif
