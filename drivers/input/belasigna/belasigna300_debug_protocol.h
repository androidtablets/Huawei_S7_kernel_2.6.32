#ifndef BELASIGNA300_DEBUG_PROTOCOL_H
#define BELASIGNA300_DEBUG_PROTOCOL_H

// Commands
enum Commands {
    CMD_GET_STATUS = 'S',
    CMD_READ_AND_RESET_CRC = 'M',
    CMD_WRITE_MEMORY = 'W',
    CMD_WRITE_NORMAL_REGISTERS = 'F',
    CMD_WRITE_SPECIAL_REGISTERS = '2',
    CMD_EXECUTE_INSTRUCTION = 'O',
    CMD_START_CORE = 'G',
    CMD_STOP_CORE = 'P',
};

enum NormalRegisters {
    NORMAL_REGISTER_SR = 50,
};

enum SpecialRegisters {
    SPECIAL_REGISTER_LP = 12,
};
/*
enum Commands {
    CMD_NOP = 0x00,
    CMD_RESET_TWSS = 'Q',
    CMD_ACQUIRE_ADDRESS = 0x04,
    CMD_RESET_AND_ACQUIRE_ADDRESS = 0x06,
    CMD_SET_ADDRESS = 'A',
    CMD_GET_STATUS = 'S',
    CMD_EXECUTE_JUMPROM_FUNCTION = 'J',
    CMD_SET_MEMORY_BLOCK_POINTER = 'M',
    CMD_WRITE_MEMORY = 'W',
    CMD_READ_MEMORY = 'R',
    CMD_CALCULATE_CHECKSUM = 'K',
    CMD_READ_CHECKSUM = 'C',
    CMD_SET_CLOCKING_FREQUENCY = 'F',
    CMD_START_APPLICATION = 'G',
};
*/

// Status byte fields and values
enum StatusByte {
    STATUS_SECURITY_MODE            = 0x0800,
    STATUS_SECURITY_RESTRICTED      = 0x0800,
    STATUS_SECURITY_UNRESTRICTED    = 0x0000,
};

// Transfer mode byte for memory transfers
enum TransferMode {
    TRANSFER_MODE_MULTIPLE_WORDS    = 0x00,
    TRANSFER_MODE_SINGLE_WORD       = 0x10,

    TRANSFER_MODE_MEMORY_SPACE_X    = 0x04,
    TRANSFER_MODE_MEMORY_SPACE_Y    = 0x08,
    TRANSFER_MODE_MEMORY_SPACE_P    = 0x0C,

    TRANSFER_MODE_DATA_WIDTH_8      = 0x00,
    TRANSFER_MODE_DATA_WIDTH_16     = 0x01,
    TRANSFER_MODE_DATA_WIDTH_24     = 0x02,
    TRANSFER_MODE_DATA_WIDTH_32     = 0x03,

    TRANSFER_MODE_X                 = TRANSFER_MODE_MULTIPLE_WORDS |
                                      TRANSFER_MODE_MEMORY_SPACE_X |
                                      TRANSFER_MODE_DATA_WIDTH_24,
    TRANSFER_MODE_Y                 = TRANSFER_MODE_MULTIPLE_WORDS |
                                      TRANSFER_MODE_MEMORY_SPACE_Y |
                                      TRANSFER_MODE_DATA_WIDTH_24,
    TRANSFER_MODE_P                 = TRANSFER_MODE_MULTIPLE_WORDS |
                                      TRANSFER_MODE_MEMORY_SPACE_P |
                                      TRANSFER_MODE_DATA_WIDTH_32,
};

#endif
