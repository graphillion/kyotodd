#ifndef KYOTODD_SRC_BDD_IO_COMMON_H
#define KYOTODD_SRC_BDD_IO_COMMON_H

// Internal header shared by the bdd_io_*.cpp implementation files.
// Declares stream I/O helpers, little-endian encode/decode, and the
// import node-creation function pointer type. All helpers are marked
// static inline so each translation unit gets its own copy — this file
// is intentionally NOT installed as a public header.

#include "bdd.h"
#include "bdd_internal.h"
#include <cinttypes>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <istream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace kyotodd {

// Max token length for arc string representations (node IDs, "F", "T", "N").
// ARC_BUF_SIZE includes the null terminator.
#define ARC_MAX_LEN 31
#define ARC_BUF_SIZE (ARC_MAX_LEN + 1)
#define XSTR_(x) #x
#define XSTR(x) XSTR_(x)

static const unsigned IMPORT_MAX_LEVEL_LIMIT = 1000000;
static const unsigned IMPORT_MAX_NODE_COUNT = 10000000;
static const unsigned IMPORT_MAX_OUTPUT_COUNT = 1000000;

typedef bddp (*import_nodefn_t)(bddvar, bddp, bddp);

// Stream output helpers for FILE* and std::ostream.
static inline void write_str(FILE* strm, const char* s) { std::fputs(s, strm); }
static inline void write_str(std::ostream& strm, const char* s) { strm << s; }

static inline bool stream_valid(FILE* strm) { return strm != nullptr; }
static inline bool stream_valid(std::ostream&) { return true; }

static inline bool stream_has_error(FILE* strm) { return std::ferror(strm) != 0; }
static inline bool stream_has_error(std::ostream& strm) { return !strm.good(); }

static inline void write_bytes(FILE* strm, const void* data, size_t n) {
    std::fwrite(data, 1, n, strm);
}
static inline void write_bytes(std::ostream& strm, const void* data, size_t n) {
    strm.write(static_cast<const char*>(data), n);
}
static inline bool read_bytes(FILE* strm, void* data, size_t n) {
    return std::fread(data, 1, n, strm) == n;
}
static inline bool read_bytes(std::istream& strm, void* data, size_t n) {
    strm.read(static_cast<char*>(data), n);
    return !strm.fail();
}

// Tags ("_i", "_o", "_n") are at most 2 chars; ARC_MAX_LEN is more than enough.
static inline bool read_tag_uint(FILE* strm, const char* tag, unsigned& val) {
    char buf[ARC_BUF_SIZE];
    if (std::fscanf(strm, " %" XSTR(ARC_MAX_LEN) "s %u", buf, &val) != 2) return false;
    return std::string(buf) == tag;
}
static inline bool read_tag_uint(std::istream& strm, const char* tag, unsigned& val) {
    std::string s;
    if (!(strm >> s >> val)) return false;
    return s == tag;
}

static inline bool read_node_line(FILE* strm, uint64_t& id, unsigned& level,
                                  char* s0, char* s1) {
    return std::fscanf(strm, " %" SCNu64 " %u %" XSTR(ARC_MAX_LEN) "s %" XSTR(ARC_MAX_LEN) "s",
                       &id, &level, s0, s1) == 4;
}
static inline bool read_node_line(std::istream& strm, uint64_t& id, unsigned& level,
                                  char* s0, char* s1) {
    std::string ss0, ss1;
    if (!(strm >> id >> level >> ss0 >> ss1)) return false;
    if (ss0.size() > ARC_MAX_LEN || ss1.size() > ARC_MAX_LEN) return false;
    std::snprintf(s0, ARC_BUF_SIZE, "%s", ss0.c_str());
    std::snprintf(s1, ARC_BUF_SIZE, "%s", ss1.c_str());
    return true;
}

static inline bool read_token(FILE* strm, char* buf) {
    return std::fscanf(strm, " %" XSTR(ARC_MAX_LEN) "s", buf) == 1;
}
static inline bool read_token(std::istream& strm, char* buf) {
    std::string s;
    if (!(strm >> s)) return false;
    if (s.size() > ARC_MAX_LEN) return false;
    std::snprintf(buf, ARC_BUF_SIZE, "%s", s.c_str());
    return true;
}

// Read one line from FILE* or istream. Returns false on EOF.
static inline bool read_line(FILE* strm, char* buf, size_t bufsize) {
    if (std::fgets(buf, static_cast<int>(bufsize), strm) == NULL) return false;
    // Strip trailing newline
    size_t len = std::strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
    return true;
}
static inline bool read_line(std::istream& strm, char* buf, size_t bufsize) {
    std::string s;
    if (!std::getline(strm, s)) return false;
    std::snprintf(buf, bufsize, "%s", s.c_str());
    return true;
}

// Little-endian encode/decode
static inline void encode_le16(uint8_t* buf, uint16_t v) {
    buf[0] = v & 0xFF; buf[1] = (v >> 8) & 0xFF;
}
static inline void encode_le32(uint8_t* buf, uint32_t v) {
    for (int i = 0; i < 4; i++) buf[i] = (v >> (8*i)) & 0xFF;
}
static inline void encode_le64(uint8_t* buf, uint64_t v) {
    for (int i = 0; i < 8; i++) buf[i] = (v >> (8*i)) & 0xFF;
}
static inline uint16_t decode_le16(const uint8_t* buf) {
    return static_cast<uint16_t>(buf[0]) | (static_cast<uint16_t>(buf[1]) << 8);
}
static inline uint32_t decode_le32(const uint8_t* buf) {
    uint32_t v = 0;
    for (int i = 0; i < 4; i++) v |= static_cast<uint32_t>(buf[i]) << (8*i);
    return v;
}
static inline uint64_t decode_le64(const uint8_t* buf) {
    uint64_t v = 0;
    for (int i = 0; i < 8; i++) v |= static_cast<uint64_t>(buf[i]) << (8*i);
    return v;
}

// Write/read a single ID in LE with the given byte width
template<typename Stream>
static inline void write_id_le(Stream& strm, uint64_t id, int bytes) {
    uint8_t buf[8] = {0};
    for (int i = 0; i < bytes; i++) buf[i] = (id >> (8*i)) & 0xFF;
    write_bytes(strm, buf, bytes);
}
template<typename Stream>
static inline uint64_t read_id_le(Stream& strm, int bytes) {
    uint8_t buf[8] = {0};
    if (!read_bytes(strm, buf, bytes))
        throw std::runtime_error("binary import: unexpected end of input");
    uint64_t id = 0;
    for (int i = 0; i < bytes; i++) id |= static_cast<uint64_t>(buf[i]) << (8*i);
    return id;
}

// Determine minimum byte-aligned bits to represent max_id
static inline uint8_t bits_for_max_id(uint64_t max_id) {
    if (max_id <= 0xFF) return 8;
    if (max_id <= 0xFFFF) return 16;
    if (max_id <= 0xFFFFFFFFULL) return 32;
    return 64;
}

} // namespace kyotodd

#endif
