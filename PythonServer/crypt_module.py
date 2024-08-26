import array

class Decipher:

    ONE_BYTE = 0b00000001
    DEFAULT_BYTE = 0b01010101
    REVERSE_BYTE = 0b10101010
    BIT_MAP = {
        "/a": 0,
        "/b": 1,
        "TCP_A": 0,
        "TCP_B": 1
    }

    def __init__(self):
        self.mode = "DFX"
        self.message = ''
        self.currByte = array.array('i', [0] * 2)
        self.currIndex = 0
        self.part = 0

    def receive_packet(self, channel, logger):
        self.currByte[self.part] |= (Decipher.BIT_MAP[channel] << (7 - self.currIndex))
        real_index = self.currIndex + self.part * 8
        if real_index == 7:
            self.part = 1
        elif real_index == 15:
            self.message += self.covert_mirrored_xor(self.currByte[0], self.currByte[1])
            self.currByte[0] = 0
            self.currByte[1] = 0
            self.part = 0
            logger.update_covert_message(self.message)
            if self.message != '\0': logger.program_log("Covert message updated.", "notice")
        self.currIndex = (self.currIndex + 1) % 8

    def covert_mirrored_xor(self, left, right):
        res = (left & Decipher.REVERSE_BYTE) | (right & Decipher.DEFAULT_BYTE)
        return chr(res ^ Decipher.DEFAULT_BYTE)

    def clear_message(self):
        self.message = ''
        self.currByte = array.array('i', [0] * 2)
        self.currIndex = 0
        self.part = 0

    @property
    def is_message(self):
        return len(self.message) > 0