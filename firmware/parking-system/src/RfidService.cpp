#include "RfidService.h"

bool RfidService::isWhitelisted(const char* uid) const {
    for (size_t i = 0; i < RFID_WHITELIST_SIZE; ++i) {
        if (strcasecmp(uid, RFID_WHITELIST[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool RfidService::takeEvent(RfidEvent& out) {
    if (!_hasPending) {
        return false;
    }
    out         = _pending;
    _hasPending = false;
    return true;
}

#if ENABLE_RFID

void RfidService::begin() {
    SPI.begin(PIN_RFID_SCK, PIN_RFID_MISO, PIN_RFID_MOSI, PIN_RFID_SS);
    _reader.PCD_Init();

    // 读版本寄存器确认硬件在线（常见值 0x91/0x92；0x00/0xFF 说明接线有问题）
    const byte version = _reader.PCD_ReadRegister(MFRC522::VersionReg);
    if (version == 0x00 || version == 0xFF) {
        LOG_PRINTF("[RFID] WARNING: RC522 not responding (version=0x%02X), check wiring\n", version);
    } else {
        LOG_PRINTF("[RFID] init OK, RC522 firmware version 0x%02X, whitelist size %u\n",
                   version, (unsigned)RFID_WHITELIST_SIZE);
    }
}

void RfidService::update(uint32_t now) {
    if (!_reader.PICC_IsNewCardPresent() || !_reader.PICC_ReadCardSerial()) {
        return;
    }

    // UID 格式化为大写十六进制、冒号分隔，如 "A1:B2:C3:D4"
    char uid[RFID_UID_MAX_LEN] = {0};
    size_t pos = 0;
    for (byte i = 0; i < _reader.uid.size && pos + 3 < sizeof(uid); ++i) {
        pos += snprintf(uid + pos, sizeof(uid) - pos, i == 0 ? "%02X" : ":%02X",
                        _reader.uid.uidByte[i]);
    }

    _reader.PICC_HaltA();
    _reader.PCD_StopCrypto1();

    // 同一张卡在冷却时间内重复出现则忽略
    if (strcmp(uid, _lastUid) == 0 && now - _lastReadMs < RFID_READ_COOLDOWN_MS) {
        return;
    }
    strlcpy(_lastUid, uid, sizeof(_lastUid));
    _lastReadMs = now;

    const bool authorized = isWhitelisted(uid);
    LOG_PRINTF("[RFID] New card UID: %s -> %s\n", uid, authorized ? "AUTHORIZED" : "DENIED");
    if (!authorized) {
        LOG_PRINTLN("[RFID] 提示: 如需放行此卡, 请把上面的 UID 加入 config/Settings.h 的 RFID_WHITELIST");
    }

    _pending.authorized = authorized;
    strlcpy(_pending.uid, uid, sizeof(_pending.uid));
    _hasPending = true;
}

#else  // !ENABLE_RFID — 空实现，保证关闭功能后仍可编译

void RfidService::begin() {
    LOG_PRINTLN("[RFID] disabled by ENABLE_RFID=0");
}

void RfidService::update(uint32_t now) {
    (void)now;
}

#endif  // ENABLE_RFID
