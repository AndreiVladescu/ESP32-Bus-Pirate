#include <unity.h>

#include <array>
#include <cstring>
#include <string>
#include <vector>

#include "Transformers/Bpio2Transformer.h"

namespace bpio2_transformer_tests {

uint16_t testReadU16(const uint8_t* p) {
    return static_cast<uint16_t>(p[0]) |
           (static_cast<uint16_t>(p[1]) << 8);
}

uint32_t testReadU32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) |
           (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) |
           (static_cast<uint32_t>(p[3]) << 24);
}

int32_t testReadI32(const uint8_t* p) {
    return static_cast<int32_t>(testReadU32(p));
}

void testWriteU16(uint8_t* p, uint16_t value) {
    p[0] = static_cast<uint8_t>(value & 0xFF);
    p[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
}

void testWriteU32(uint8_t* p, uint32_t value) {
    p[0] = static_cast<uint8_t>(value & 0xFF);
    p[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
    p[2] = static_cast<uint8_t>((value >> 16) & 0xFF);
    p[3] = static_cast<uint8_t>((value >> 24) & 0xFF);
}

struct TestTableHandle {
    size_t object = 0;
    size_t fieldCount = 0;
};

class TestFlatBufferWriter {
public:
    TestFlatBufferWriter(uint8_t* output, size_t capacity)
        : output_(output), capacity_(capacity) {
        if (output_ && capacity_ >= 4) {
            std::memset(output_, 0, capacity_);
            position_ = 4;
            valid_ = true;
        }
    }

    bool valid() const { return valid_; }
    size_t size() const { return valid_ ? position_ : 0; }

    TestTableHandle createRootTable(size_t fieldCount) {
        TestTableHandle table = createTable(fieldCount);
        if (valid_) testWriteU32(output_, static_cast<uint32_t>(table.object));
        return table;
    }

    TestTableHandle createTable(size_t fieldCount) {
        TestTableHandle table;
        if (!valid_ || fieldCount > 64) {
            fail();
            return table;
        }

        align(2);
        const size_t vtable = position_;
        const size_t vtableSize = 4 + fieldCount * 2;
        const size_t objectSize = 4 + fieldCount * 4;
        if (!reserve(vtableSize)) return table;
        testWriteU16(output_ + vtable, static_cast<uint16_t>(vtableSize));
        testWriteU16(output_ + vtable + 2, static_cast<uint16_t>(objectSize));

        align(4);
        const size_t object = position_;
        if (!reserve(objectSize)) return table;
        testWriteU32(output_ + object, static_cast<uint32_t>(object - vtable));

        table.object = object;
        table.fieldCount = fieldCount;
        return table;
    }

    void setU8(const TestTableHandle& table, size_t slot, uint8_t value, bool present = true) {
        if (!present) return;
        const size_t field = markField(table, slot);
        if (field != INVALID) output_[field] = value;
    }

    void setBool(const TestTableHandle& table, size_t slot, bool value, bool present = true) {
        setU8(table, slot, value ? 1 : 0, present);
    }

    void setU16(const TestTableHandle& table, size_t slot, uint16_t value, bool present = true) {
        if (!present) return;
        const size_t field = markField(table, slot);
        if (field != INVALID) testWriteU16(output_ + field, value);
    }

    void setU32(const TestTableHandle& table, size_t slot, uint32_t value, bool present = true) {
        if (!present) return;
        const size_t field = markField(table, slot);
        if (field != INVALID) testWriteU32(output_ + field, value);
    }

    void setOffset(const TestTableHandle& table, size_t slot, size_t target) {
        const size_t field = markField(table, slot);
        if (field == INVALID || target <= field || target - field > UINT32_MAX) {
            fail();
            return;
        }
        testWriteU32(output_ + field, static_cast<uint32_t>(target - field));
    }

    size_t createString(const char* value) {
        if (!value) value = "";
        const size_t length = std::strlen(value);
        align(4);
        const size_t target = position_;
        if (length > UINT32_MAX || !reserve(4 + length + 1)) return 0;
        testWriteU32(output_ + target, static_cast<uint32_t>(length));
        std::memcpy(output_ + target + 4, value, length);
        output_[target + 4 + length] = 0;
        return target;
    }

    size_t createByteVector(const uint8_t* data, size_t length) {
        align(4);
        const size_t target = position_;
        if (length > UINT32_MAX || !reserve(4 + length)) return 0;
        testWriteU32(output_ + target, static_cast<uint32_t>(length));
        if (length && data) std::memcpy(output_ + target + 4, data, length);
        return target;
    }

    size_t createU32Vector(const uint32_t* data, size_t length) {
        align(4);
        const size_t target = position_;
        if (length > UINT32_MAX || !reserve(4 + length * sizeof(uint32_t))) return 0;
        testWriteU32(output_ + target, static_cast<uint32_t>(length));
        for (size_t i = 0; i < length; ++i) {
            testWriteU32(output_ + target + 4 + i * sizeof(uint32_t), data ? data[i] : 0);
        }
        return target;
    }

private:
    static constexpr size_t INVALID = static_cast<size_t>(-1);

    void fail() {
        valid_ = false;
        position_ = 0;
    }

    bool reserve(size_t count) {
        if (!valid_ || count > capacity_ - position_) {
            fail();
            return false;
        }
        std::memset(output_ + position_, 0, count);
        position_ += count;
        return true;
    }

    void align(size_t alignment) {
        if (!valid_) return;
        reserve((alignment - (position_ % alignment)) % alignment);
    }

    size_t markField(const TestTableHandle& table, size_t slot) {
        if (!valid_ || slot >= table.fieldCount) {
            fail();
            return INVALID;
        }
        const size_t fieldOffset = 4 + slot * 4;
        const size_t vtable = table.object - testReadU32(output_ + table.object);
        const size_t entry = vtable + 4 + slot * 2;
        if (fieldOffset > UINT16_MAX || entry + 2 > capacity_) {
            fail();
            return INVALID;
        }
        testWriteU16(output_ + entry, static_cast<uint16_t>(fieldOffset));
        return table.object + fieldOffset;
    }

    uint8_t* output_ = nullptr;
    size_t capacity_ = 0;
    size_t position_ = 0;
    bool valid_ = false;
};

class TestTableReader {
public:
    bool open(const uint8_t* data, size_t length, size_t objectPosition) {
        data_ = data;
        length_ = length;
        object_ = objectPosition;
        if (!data_ || object_ + 4 > length_) return false;

        const int32_t backOffset = testReadI32(data_ + object_);
        if (backOffset <= 0 || static_cast<size_t>(backOffset) > object_) return false;

        vtable_ = object_ - static_cast<size_t>(backOffset);
        if (vtable_ + 4 > length_) return false;

        vtableSize_ = testReadU16(data_ + vtable_);
        objectSize_ = testReadU16(data_ + vtable_ + 2);
        return vtableSize_ >= 4 &&
               vtable_ + vtableSize_ <= length_ &&
               objectSize_ >= 4 &&
               object_ + objectSize_ <= length_;
    }

    bool has(size_t slot, size_t width = 1) const {
        const size_t entry = vtable_ + 4 + slot * 2;
        if (entry + 2 > vtable_ + vtableSize_) return false;
        const uint16_t offset = testReadU16(data_ + entry);
        return offset != 0 && offset + width <= objectSize_ && object_ + offset + width <= length_;
    }

    uint8_t getU8(size_t slot, uint8_t defaultValue = 0) const {
        const size_t pos = fieldPosition(slot, 1);
        return pos == INVALID ? defaultValue : data_[pos];
    }

    uint16_t getU16(size_t slot, uint16_t defaultValue = 0) const {
        const size_t pos = fieldPosition(slot, 2);
        return pos == INVALID ? defaultValue : testReadU16(data_ + pos);
    }

    uint32_t getU32(size_t slot, uint32_t defaultValue = 0) const {
        const size_t pos = fieldPosition(slot, 4);
        return pos == INVALID ? defaultValue : testReadU32(data_ + pos);
    }

    bool getTable(size_t slot, TestTableReader& table) const {
        size_t target = 0;
        return getOffsetTarget(slot, target) && table.open(data_, length_, target);
    }

    bool getString(size_t slot, std::string& value) const {
        value.clear();
        size_t target = 0;
        if (!getOffsetTarget(slot, target) || target + 4 > length_) return false;
        const uint32_t size = testReadU32(data_ + target);
        const size_t dataStart = target + 4;
        if (size > length_ - dataStart || dataStart + size >= length_) return false;
        if (data_[dataStart + size] != 0) return false;
        value.assign(reinterpret_cast<const char*>(data_ + dataStart), size);
        return true;
    }

    bool getStringVector(size_t slot, std::vector<std::string>& values) const {
        values.clear();
        size_t target = 0;
        if (!getOffsetTarget(slot, target) || target + 4 > length_) return false;
        const uint32_t count = testReadU32(data_ + target);
        const size_t dataStart = target + 4;
        if (count > (length_ - dataStart) / sizeof(uint32_t)) return false;

        for (uint32_t i = 0; i < count; ++i) {
            const size_t element = dataStart + i * sizeof(uint32_t);
            const uint32_t relative = testReadU32(data_ + element);
            if (relative == 0 || relative > length_ - element) return false;
            const size_t stringTarget = element + relative;
            if (stringTarget + 4 > length_) return false;

            const uint32_t size = testReadU32(data_ + stringTarget);
            const size_t stringStart = stringTarget + 4;
            if (size > length_ - stringStart || stringStart + size >= length_) return false;
            if (data_[stringStart + size] != 0) return false;
            values.emplace_back(reinterpret_cast<const char*>(data_ + stringStart), size);
        }
        return true;
    }

private:
    static constexpr size_t INVALID = static_cast<size_t>(-1);

    size_t fieldPosition(size_t slot, size_t width) const {
        if (!has(slot, width)) return INVALID;
        const size_t entry = vtable_ + 4 + slot * 2;
        return object_ + testReadU16(data_ + entry);
    }

    bool getOffsetTarget(size_t slot, size_t& target) const {
        const size_t field = fieldPosition(slot, 4);
        if (field == INVALID) return false;
        const uint32_t relative = testReadU32(data_ + field);
        if (relative == 0 || relative > length_ - field) return false;
        target = field + relative;
        return target < length_;
    }

    const uint8_t* data_ = nullptr;
    size_t length_ = 0;
    size_t object_ = 0;
    size_t vtable_ = 0;
    size_t vtableSize_ = 0;
    size_t objectSize_ = 0;
};

bool openTestRoot(const uint8_t* buffer, size_t length, TestTableReader& root) {
    if (!buffer || length < 8) return false;
    const uint32_t rootOffset = testReadU32(buffer);
    return rootOffset >= 4 && rootOffset < length && root.open(buffer, length, rootOffset);
}

struct BuiltPacket {
    std::vector<uint8_t> bytes;
    size_t size = 0;
    size_t rootObject = 0;
    size_t contentsObject = 0;
    size_t modeString = 0;
    size_t modeConfigField = 0;
    size_t ledColorVector = 0;
    size_t dataWriteVector = 0;
};

BuiltPacket makeStatusRequest(const std::vector<uint8_t>& query = {}) {
    BuiltPacket packet;
    packet.bytes.resize(256);
    TestFlatBufferWriter writer(packet.bytes.data(), packet.bytes.size());
    const auto root = writer.createRootTable(4);
    packet.rootObject = root.object;
    writer.setU8(root, 0, Bpio2::VERSION_MAJOR);
    writer.setU16(root, 1, Bpio2::VERSION_MINOR);
    writer.setU8(root, 2, static_cast<uint8_t>(Bpio2::RequestType::Status));

    const auto contents = writer.createTable(1);
    packet.contentsObject = contents.object;
    writer.setOffset(root, 3, contents.object);
    if (!query.empty()) {
        const size_t queryVector = writer.createByteVector(query.data(), query.size());
        writer.setOffset(contents, 0, queryVector);
    }
    packet.size = writer.size();
    packet.bytes.resize(packet.size);
    return packet;
}

BuiltPacket makeConfigurationRequest() {
    BuiltPacket packet;
    packet.bytes.resize(512);
    TestFlatBufferWriter writer(packet.bytes.data(), packet.bytes.size());
    const auto root = writer.createRootTable(4);
    packet.rootObject = root.object;
    writer.setU8(root, 0, Bpio2::VERSION_MAJOR);
    writer.setU16(root, 1, Bpio2::VERSION_MINOR);
    writer.setU8(root, 2, static_cast<uint8_t>(Bpio2::RequestType::Configuration));

    const auto contents = writer.createTable(20);
    packet.contentsObject = contents.object;
    writer.setOffset(root, 3, contents.object);

    packet.modeString = writer.createString("SPI");
    writer.setOffset(contents, 0, packet.modeString);

    const auto modeConfig = writer.createTable(13);
    packet.modeConfigField = contents.object + 4 + 1 * 4;
    writer.setOffset(contents, 1, modeConfig.object);
    writer.setU32(modeConfig, 0, 4000000);
    writer.setU8(modeConfig, 1, 8);
    writer.setBool(modeConfig, 2, true);
    writer.setU8(modeConfig, 3, 2);
    writer.setBool(modeConfig, 4, true);
    writer.setBool(modeConfig, 5, true);
    writer.setBool(modeConfig, 6, true);
    writer.setBool(modeConfig, 7, true);
    writer.setBool(modeConfig, 8, true);
    writer.setBool(modeConfig, 9, false);
    writer.setU8(modeConfig, 10, 3);
    writer.setU32(modeConfig, 11, 38000);
    writer.setU8(modeConfig, 12, 7);

    writer.setBool(contents, 2, true);
    writer.setBool(contents, 5, true);
    writer.setBool(contents, 9, true);
    writer.setU8(contents, 10, 0xF0);
    writer.setU8(contents, 11, 0xA0);
    writer.setU8(contents, 12, 0x0F);
    writer.setU8(contents, 13, 0x05);
    writer.setBool(contents, 14, true);

    const uint32_t colors[] = {0x00FF0000, 0x0000FF00};
    packet.ledColorVector = writer.createU32Vector(colors, 2);
    writer.setOffset(contents, 15, packet.ledColorVector);

    const size_t printString = writer.createString("hello");
    writer.setOffset(contents, 16, printString);
    writer.setBool(contents, 18, true);

    packet.size = writer.size();
    packet.bytes.resize(packet.size);
    return packet;
}

BuiltPacket makeDataRequest(const uint8_t* payload,
                            size_t payloadLength,
                            uint16_t bytesRead,
                            bool startAlt = false) {
    BuiltPacket packet;
    packet.bytes.resize(Bpio2::MAX_PACKET_SIZE);
    TestFlatBufferWriter writer(packet.bytes.data(), packet.bytes.size());
    const auto root = writer.createRootTable(4);
    packet.rootObject = root.object;
    writer.setU8(root, 0, Bpio2::VERSION_MAJOR);
    writer.setU16(root, 1, Bpio2::VERSION_MINOR);
    writer.setU8(root, 2, static_cast<uint8_t>(Bpio2::RequestType::Data));

    const auto contents = writer.createTable(6);
    packet.contentsObject = contents.object;
    writer.setOffset(root, 3, contents.object);
    writer.setBool(contents, 0, true);
    writer.setBool(contents, 1, startAlt, startAlt);
    if (payload || payloadLength == 0) {
        packet.dataWriteVector = writer.createByteVector(payload, payloadLength);
        writer.setOffset(contents, 2, packet.dataWriteVector);
    }
    writer.setU16(contents, 3, bytesRead);
    writer.setBool(contents, 4, true);
    writer.setBool(contents, 5, true);

    packet.size = writer.size();
    packet.bytes.resize(packet.size);
    return packet;
}

BuiltPacket makeRequestWithType(uint8_t rawType, bool includeContents = true) {
    BuiltPacket packet;
    packet.bytes.resize(128);
    TestFlatBufferWriter writer(packet.bytes.data(), packet.bytes.size());
    const auto root = writer.createRootTable(4);
    packet.rootObject = root.object;
    writer.setU8(root, 0, Bpio2::VERSION_MAJOR);
    writer.setU16(root, 1, Bpio2::VERSION_MINOR);
    writer.setU8(root, 2, rawType);
    if (includeContents) {
        const auto contents = writer.createTable(1);
        packet.contentsObject = contents.object;
        writer.setOffset(root, 3, contents.object);
    }
    packet.size = writer.size();
    packet.bytes.resize(packet.size);
    return packet;
}

bool decodeBuiltPacket(BuiltPacket& packet, Bpio2& request, const char*& error) {
    return Bpio2Transformer::decodeRequest(packet.bytes.data(), packet.size, request, error);
}

void test_cobs_encode_handles_zero_separated_payload() {
    const uint8_t input[] = {0x11, 0x22, 0x00, 0x33, 0x00, 0x00};
    uint8_t output[16] = {};
    size_t outputLength = 0;

    const bool ok = Bpio2Transformer::cobsEncode(input, sizeof(input), output, sizeof(output), outputLength);

    const uint8_t expected[] = {0x03, 0x11, 0x22, 0x02, 0x33, 0x01, 0x01};
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT32(sizeof(expected), outputLength);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, output, sizeof(expected));
}

void test_cobs_decode_roundtrips_encoded_payload() {
    const uint8_t input[] = {0x11, 0x00, 0x22, 0x33, 0x00};
    uint8_t encoded[16] = {};
    uint8_t decoded[16] = {};
    size_t encodedLength = 0;
    size_t decodedLength = 0;

    TEST_ASSERT_TRUE(Bpio2Transformer::cobsEncode(input, sizeof(input), encoded,
                                                  sizeof(encoded), encodedLength));
    TEST_ASSERT_TRUE(Bpio2Transformer::cobsDecode(encoded, encodedLength, decoded,
                                                  sizeof(decoded), decodedLength));

    TEST_ASSERT_EQUAL_UINT32(sizeof(input), decodedLength);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(input, decoded, sizeof(input));
}

void test_cobs_encode_empty_payload_emits_single_code_byte() {
    uint8_t output[4] = {};
    size_t outputLength = 0;

    const bool ok = Bpio2Transformer::cobsEncode(nullptr, 0, output, sizeof(output), outputLength);

    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT32(1, outputLength);
    TEST_ASSERT_EQUAL_HEX8(0x01, output[0]);
}

void test_cobs_decode_rejects_zero_code_and_small_output_buffer() {
    const uint8_t badCode[] = {0x00};
    const uint8_t needsThreeBytes[] = {0x04, 0xAA, 0xBB, 0xCC};
    uint8_t output[2] = {};
    size_t outputLength = 123;

    TEST_ASSERT_FALSE(Bpio2Transformer::cobsDecode(badCode, sizeof(badCode), output,
                                                   sizeof(output), outputLength));
    TEST_ASSERT_EQUAL_UINT32(0, outputLength);
    TEST_ASSERT_FALSE(Bpio2Transformer::cobsDecode(needsThreeBytes, sizeof(needsThreeBytes),
                                                   output, sizeof(output), outputLength));
}

void test_response_builders_return_zero_when_capacity_is_too_small() {
    uint8_t output[8] = {};

    TEST_ASSERT_EQUAL_UINT32(0, Bpio2Transformer::buildErrorResponse(output, sizeof(output), "boom"));
    TEST_ASSERT_EQUAL_UINT32(0, Bpio2Transformer::buildConfigurationResponse(output, sizeof(output)));
    TEST_ASSERT_EQUAL_UINT32(0, Bpio2Transformer::buildDataResponse(output, sizeof(output), nullptr, 0));
}

void test_data_response_accepts_empty_vector_and_payload_vector() {
    uint8_t output[256] = {};
    const uint8_t payload[] = {0xDE, 0xAD, 0xBE, 0xEF};

    const size_t emptySize = Bpio2Transformer::buildDataResponse(output, sizeof(output), nullptr, 0);
    const size_t payloadSize = Bpio2Transformer::buildDataResponse(output, sizeof(output),
                                                                   payload, sizeof(payload),
                                                                   nullptr, true);

    TEST_ASSERT_GREATER_THAN_UINT32(0, emptySize);
    TEST_ASSERT_GREATER_THAN_UINT32(emptySize, payloadSize);
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(Bpio2::MAX_PACKET_SIZE, payloadSize);
}

void test_decode_status_request_accepts_optional_query_vector() {
    auto packet = makeStatusRequest({1, 2, 3});
    Bpio2 request;
    const char* error = nullptr;

    TEST_ASSERT_TRUE(decodeBuiltPacket(packet, request, error));
    TEST_ASSERT_NULL(error);
    TEST_ASSERT_EQUAL_UINT8(Bpio2::VERSION_MAJOR, request.versionMajor);
    TEST_ASSERT_EQUAL_UINT16(Bpio2::VERSION_MINOR, request.minimumVersionMinor);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Bpio2::RequestType::Status), static_cast<int>(request.type));
}

void test_decode_configuration_request_reads_strings_vectors_and_nested_table() {
    auto packet = makeConfigurationRequest();
    Bpio2 request;
    const char* error = nullptr;

    TEST_ASSERT_TRUE(decodeBuiltPacket(packet, request, error));
    TEST_ASSERT_NULL(error);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Bpio2::RequestType::Configuration), static_cast<int>(request.type));
    TEST_ASSERT_TRUE(request.configuration.hasMode);
    TEST_ASSERT_TRUE(request.configuration.mode.equalsIgnoreCase("spi"));
    TEST_ASSERT_TRUE(request.configuration.hasModeConfiguration);
    TEST_ASSERT_EQUAL_UINT32(4000000, request.configuration.modeConfiguration.speed);
    TEST_ASSERT_EQUAL_UINT8(8, request.configuration.modeConfiguration.dataBits);
    TEST_ASSERT_TRUE(request.configuration.modeConfiguration.parity);
    TEST_ASSERT_EQUAL_UINT8(2, request.configuration.modeConfiguration.stopBits);
    TEST_ASSERT_TRUE(request.configuration.modeConfiguration.flowControl);
    TEST_ASSERT_TRUE(request.configuration.modeConfiguration.signalInversion);
    TEST_ASSERT_TRUE(request.configuration.modeConfiguration.clockStretch);
    TEST_ASSERT_TRUE(request.configuration.modeConfiguration.clockPolarity);
    TEST_ASSERT_TRUE(request.configuration.modeConfiguration.clockPhase);
    TEST_ASSERT_FALSE(request.configuration.modeConfiguration.chipSelectIdle);
    TEST_ASSERT_EQUAL_UINT8(3, request.configuration.modeConfiguration.submode);
    TEST_ASSERT_EQUAL_UINT32(38000, request.configuration.modeConfiguration.txModulation);
    TEST_ASSERT_EQUAL_UINT8(7, request.configuration.modeConfiguration.rxSensor);
    TEST_ASSERT_TRUE(request.configuration.modeBitOrderMsb);
    TEST_ASSERT_TRUE(request.configuration.psuEnable);
    TEST_ASSERT_TRUE(request.configuration.pullupEnable);
    TEST_ASSERT_TRUE(request.configuration.hasIoDirectionMask);
    TEST_ASSERT_EQUAL_UINT8(0xF0, request.configuration.ioDirectionMask);
    TEST_ASSERT_EQUAL_UINT8(0xA0, request.configuration.ioDirection);
    TEST_ASSERT_TRUE(request.configuration.hasIoValueMask);
    TEST_ASSERT_EQUAL_UINT8(0x0F, request.configuration.ioValueMask);
    TEST_ASSERT_EQUAL_UINT8(0x05, request.configuration.ioValue);
    TEST_ASSERT_TRUE(request.configuration.hasLedResume);
    TEST_ASSERT_TRUE(request.configuration.hasLedColor);
    TEST_ASSERT_TRUE(request.configuration.hasPrintString);
    TEST_ASSERT_TRUE(request.configuration.hardwareReset);
}

void test_decode_data_request_reads_payload_and_flags() {
    const uint8_t payload[] = {0xDE, 0xAD, 0xBE};
    auto packet = makeDataRequest(payload, sizeof(payload), 4, true);
    Bpio2 request;
    const char* error = nullptr;

    TEST_ASSERT_TRUE(decodeBuiltPacket(packet, request, error));
    TEST_ASSERT_NULL(error);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Bpio2::RequestType::Data), static_cast<int>(request.type));
    TEST_ASSERT_TRUE(request.data.startMain);
    TEST_ASSERT_TRUE(request.data.startAlt);
    TEST_ASSERT_EQUAL_UINT32(sizeof(payload), request.data.dataWriteLength);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(payload, request.data.dataWrite, sizeof(payload));
    TEST_ASSERT_EQUAL_UINT16(4, request.data.bytesRead);
    TEST_ASSERT_TRUE(request.data.stopMain);
    TEST_ASSERT_TRUE(request.data.stopAlt);
}

void test_decode_rejects_truncated_flatbuffer_and_bad_root_offset() {
    auto packet = makeDataRequest(nullptr, 0, 0);
    Bpio2 request;
    const char* error = nullptr;

    TEST_ASSERT_FALSE(Bpio2Transformer::decodeRequest(packet.bytes.data(), 4, request, error));
    TEST_ASSERT_EQUAL_STRING("Flatbuffer packet too short", error);

    std::array<uint8_t, 8> badRoot{};
    testWriteU32(badRoot.data(), 99);
    error = nullptr;
    TEST_ASSERT_FALSE(Bpio2Transformer::decodeRequest(badRoot.data(), badRoot.size(), request, error));
    TEST_ASSERT_EQUAL_STRING("Invalid flatbuffer root table", error);
}

void test_decode_rejects_unknown_request_type_and_missing_contents() {
    auto unknown = makeRequestWithType(99);
    auto missingContents = makeRequestWithType(static_cast<uint8_t>(Bpio2::RequestType::Status), false);
    Bpio2 request;
    const char* error = nullptr;

    TEST_ASSERT_FALSE(decodeBuiltPacket(unknown, request, error));
    TEST_ASSERT_EQUAL_STRING("Unknown BPIO request type", error);

    error = nullptr;
    TEST_ASSERT_FALSE(decodeBuiltPacket(missingContents, request, error));
    TEST_ASSERT_EQUAL_STRING("BPIO request contents missing", error);
}

void test_decode_rejects_unsupported_versions() {
    auto badMajor = makeStatusRequest();
    auto badMinor = makeStatusRequest();
    Bpio2 request;
    const char* error = nullptr;

    badMajor.bytes[badMajor.rootObject + 4] = 3;
    TEST_ASSERT_FALSE(decodeBuiltPacket(badMajor, request, error));
    TEST_ASSERT_EQUAL_STRING("Unsupported BPIO version, expected 2.x", error);

    error = nullptr;
    testWriteU16(badMinor.bytes.data() + badMinor.rootObject + 8, Bpio2::VERSION_MINOR + 1);
    TEST_ASSERT_FALSE(decodeBuiltPacket(badMinor, request, error));
    TEST_ASSERT_EQUAL_STRING("BPIO minimum minor version not supported", error);
}

void test_decode_rejects_invalid_mode_string_vector_and_table_offsets() {
    Bpio2 request;
    const char* error = nullptr;

    auto badMode = makeConfigurationRequest();
    testWriteU32(badMode.bytes.data() + badMode.modeString, 9999);
    TEST_ASSERT_FALSE(decodeBuiltPacket(badMode, request, error));
    TEST_ASSERT_EQUAL_STRING("Invalid mode string", error);

    error = nullptr;
    auto badModeConfig = makeConfigurationRequest();
    testWriteU32(badModeConfig.bytes.data() + badModeConfig.modeConfigField, 0x7FFFFFFF);
    TEST_ASSERT_FALSE(decodeBuiltPacket(badModeConfig, request, error));
    TEST_ASSERT_EQUAL_STRING("Invalid mode configuration", error);

    error = nullptr;
    auto badLedVector = makeConfigurationRequest();
    testWriteU32(badLedVector.bytes.data() + badLedVector.ledColorVector, 9999);
    TEST_ASSERT_FALSE(decodeBuiltPacket(badLedVector, request, error));
    TEST_ASSERT_EQUAL_STRING("Invalid LED color vector", error);
}

void test_decode_rejects_incoherent_data_vector_and_read_sizes() {
    const uint8_t payload[] = {0xAA, 0xBB};
    Bpio2 request;
    const char* error = nullptr;

    auto badVector = makeDataRequest(payload, sizeof(payload), 0);
    testWriteU32(badVector.bytes.data() + badVector.dataWriteVector, 9999);
    TEST_ASSERT_FALSE(decodeBuiltPacket(badVector, request, error));
    TEST_ASSERT_EQUAL_STRING("Invalid data_write vector", error);

    error = nullptr;
    std::vector<uint8_t> tooLargeWrite(Bpio2::MAX_WRITE_SIZE + 1, 0x55);
    auto tooLargeWritePacket = makeDataRequest(tooLargeWrite.data(), tooLargeWrite.size(), 0);
    TEST_ASSERT_FALSE(decodeBuiltPacket(tooLargeWritePacket, request, error));
    TEST_ASSERT_EQUAL_STRING("Data write vector too long", error);

    error = nullptr;
    auto tooLargeReadPacket = makeDataRequest(nullptr, 0, static_cast<uint16_t>(Bpio2::MAX_READ_SIZE + 1));
    TEST_ASSERT_FALSE(decodeBuiltPacket(tooLargeReadPacket, request, error));
    TEST_ASSERT_EQUAL_STRING("Data read size too large", error);
}

void test_build_configuration_response_contains_type_and_error_string() {
    uint8_t output[256] = {};

    const size_t size = Bpio2Transformer::buildConfigurationResponse(output, sizeof(output), "bad config");

    TEST_ASSERT_GREATER_THAN_UINT32(0, size);
    TestTableReader packet;
    TestTableReader response;
    std::string error;
    TEST_ASSERT_TRUE(openTestRoot(output, size, packet));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Bpio2::ResponseType::Configuration), packet.getU8(1));
    TEST_ASSERT_TRUE(packet.getTable(2, response));
    TEST_ASSERT_TRUE(response.getString(0, error));
    TEST_ASSERT_EQUAL_STRING("bad config", error.c_str());
}

void test_build_status_response_contains_strings_vectors_and_numeric_fields() {
    uint8_t output[1024] = {};
    const char* modes[] = {"HiZ", "SPI", "I2C"};
    const char* pins[] = {"IO0", "IO1"};
    Bpio2::StatusSnapshot status;
    status.hardwareMajor = 2;
    status.hardwareMinor = 1;
    status.firmwareMajor = 9;
    status.firmwareMinor = 8;
    status.firmwareGitHash = "abc123";
    status.firmwareDate = "2026-07-16";
    status.modesAvailable = modes;
    status.modesAvailableCount = 3;
    status.modeCurrent = "SPI";
    status.pinLabels = pins;
    status.pinLabelCount = 2;
    status.bitOrderMsb = false;
    status.maxWrite = 1234;
    status.maxRead = 4321;
    status.ioDirection = 0xA5;
    status.ioValue = 0x5A;

    const size_t size = Bpio2Transformer::buildStatusResponse(output, sizeof(output), status, nullptr);

    TEST_ASSERT_GREATER_THAN_UINT32(0, size);
    TestTableReader packet;
    TestTableReader response;
    std::string text;
    std::vector<std::string> strings;
    TEST_ASSERT_TRUE(openTestRoot(output, size, packet));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Bpio2::ResponseType::Status), packet.getU8(1));
    TEST_ASSERT_TRUE(packet.getTable(2, response));
    TEST_ASSERT_EQUAL_UINT8(Bpio2::VERSION_MAJOR, response.getU8(1));
    TEST_ASSERT_EQUAL_UINT16(Bpio2::VERSION_MINOR, response.getU16(2));
    TEST_ASSERT_EQUAL_UINT8(2, response.getU8(3));
    TEST_ASSERT_EQUAL_UINT8(1, response.getU8(4));
    TEST_ASSERT_EQUAL_UINT8(9, response.getU8(5));
    TEST_ASSERT_EQUAL_UINT8(8, response.getU8(6));
    TEST_ASSERT_TRUE(response.getString(7, text));
    TEST_ASSERT_EQUAL_STRING("abc123", text.c_str());
    TEST_ASSERT_TRUE(response.getString(8, text));
    TEST_ASSERT_EQUAL_STRING("2026-07-16", text.c_str());
    TEST_ASSERT_TRUE(response.getStringVector(9, strings));
    TEST_ASSERT_EQUAL_UINT32(3, strings.size());
    TEST_ASSERT_EQUAL_STRING("SPI", strings[1].c_str());
    TEST_ASSERT_TRUE(response.getString(10, text));
    TEST_ASSERT_EQUAL_STRING("SPI", text.c_str());
    TEST_ASSERT_TRUE(response.getStringVector(11, strings));
    TEST_ASSERT_EQUAL_UINT32(2, strings.size());
    TEST_ASSERT_EQUAL_STRING("IO1", strings[1].c_str());
    TEST_ASSERT_EQUAL_UINT32(Bpio2::MAX_PACKET_SIZE, response.getU32(13));
    TEST_ASSERT_EQUAL_UINT32(1234, response.getU32(14));
    TEST_ASSERT_EQUAL_UINT32(4321, response.getU32(15));
    TEST_ASSERT_EQUAL_UINT8(0xA5, response.getU8(24));
    TEST_ASSERT_EQUAL_UINT8(0x5A, response.getU8(25));
}

}  // namespace bpio2_transformer_tests

void runBpio2TransformerTests() {
    using namespace bpio2_transformer_tests;
    RUN_TEST(test_cobs_encode_handles_zero_separated_payload);
    RUN_TEST(test_cobs_decode_roundtrips_encoded_payload);
    RUN_TEST(test_cobs_encode_empty_payload_emits_single_code_byte);
    RUN_TEST(test_cobs_decode_rejects_zero_code_and_small_output_buffer);
    RUN_TEST(test_response_builders_return_zero_when_capacity_is_too_small);
    RUN_TEST(test_data_response_accepts_empty_vector_and_payload_vector);
    RUN_TEST(test_decode_status_request_accepts_optional_query_vector);
    RUN_TEST(test_decode_configuration_request_reads_strings_vectors_and_nested_table);
    RUN_TEST(test_decode_data_request_reads_payload_and_flags);
    RUN_TEST(test_decode_rejects_truncated_flatbuffer_and_bad_root_offset);
    RUN_TEST(test_decode_rejects_unknown_request_type_and_missing_contents);
    RUN_TEST(test_decode_rejects_unsupported_versions);
    RUN_TEST(test_decode_rejects_invalid_mode_string_vector_and_table_offsets);
    RUN_TEST(test_decode_rejects_incoherent_data_vector_and_read_sizes);
    RUN_TEST(test_build_configuration_response_contains_type_and_error_string);
    RUN_TEST(test_build_status_response_contains_strings_vectors_and_numeric_fields);
}
