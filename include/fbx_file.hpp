#ifndef QUICKFBX_fbx_file
#define QUICKFBX_fbx_file

#include <cstdint>
#include <stdexcept>
#include <vector>
#include <fstream>
#include <istream>
#include <ostream>
#include <iostream>
#include <string>

#pragma warning(disable:4996)

namespace quickfbx {

class fbx_file;

inline std::uint8_t u1(const char *p) {
    const unsigned char *q = (const unsigned char *)p;
    std::uint32_t res = q[0];
    return res;
}

inline std::uint16_t u2(const char *p) {
    const unsigned char *q = (const unsigned char *)p;
    std::uint32_t res = q[0] + q[1] * 0x100;
    return res;
}

inline std::uint32_t u4(const char *p) {
    const unsigned char *q = (const unsigned char *)p;
    std::uint32_t res = q[0] + q[1] * 0x100 + q[2] * 0x10000 + q[3] * 0x1000000;
    return res;
}

inline std::uint64_t u8(const char *p) {
    return ((std::uint64_t)u4(p+4) << 32) | u4(p);
}

class prop;

class props {
public:
    props(const char *begin=nullptr, size_t offset=0) : begin_(begin), offset(offset) {}
    prop begin() const;
    prop end() const;

private:
    size_t end_offset() const { return u4(begin_ + offset); }
    size_t num_properties() const { return u4(begin_ + offset + 4); }
    size_t property_list_len() const { return u4(begin_ + offset + 8); }
    std::uint8_t len() const { return u1(begin_ + offset + 12); }

    size_t offset;
    const char *begin_;
};

// http://code.blender.org/2013/08/fbx-binary-file-format-specification/
class node {
public:
    node(const char *begin=nullptr, size_t offset=0) : begin_(begin), offset_(offset) {}
    bool operator !=(node &rhs) { return offset_ != rhs.offset_; }
    node &operator++() { offset_ = end_offset(); return *this; }
    std::string name() const { return std::string(begin_ + offset_ + 13, begin_ + offset_ + 13 + len()); }
    node begin() const { size_t new_offset = offset_ + 13 + property_list_len() + len(), end = end_offset(); return node(begin_, new_offset == end ? end-13 : new_offset); }
    node end() const { return node(begin_, end_offset() - 13); }
    node &operator*() { return *this; }
    quickfbx::props props() { return quickfbx::props(begin_, offset_); }

    size_t offset() const { return offset_; }
    size_t end_offset() const { return u4(begin_ + offset_); }
    size_t num_properties() const { return u4(begin_ + offset_ + 4); }
    size_t property_list_len() const { return u4(begin_ + offset_ + 8); }
    std::uint8_t len() const { return u1(begin_ + offset_ + 12); }

private:
    size_t offset_;
    const char *begin_;
};

class prop {
public:
    prop(const char *begin=nullptr, size_t offset=0) : begin_(begin), offset(offset) {}
    bool operator !=(prop &rhs) { return offset != rhs.offset; }
    prop &operator++();
    prop &operator*() { return *this; }
    char kind() { return begin_[offset]; }
    operator std::string();
private:
    size_t offset;
    const char *begin_;
};

class scene : public node {
public:
    scene() {
    }
private:
};

class fbx_file {
public:
    fbx_file(const std::string &filename);
    fbx_file(const char *begin, const char *end) { init(begin, end); }

    quickfbx::scene *scene() { return &the_scene; }
    node begin() const { return node(begin_, 27); }
    node end() const { return node(begin_, end_offset); }

private:
    void init(const char *begin, const char *end);
    void parse_objects(node &p);
    void dump(node &n, int depth);

    void bad_fbx() { throw std::runtime_error("bad fbx"); }

    std::vector<std::uint8_t> bytes;
    quickfbx::scene the_scene;
    size_t end_offset;
    const char *begin_;
    const char *end_;
};

inline fbx_file::fbx_file(const std::string &filename) {
    std::ifstream f(filename, std::ios_base::binary);
    if (!f.bad()) {
      f.seekg(0, std::ios_base::end);
      bytes.resize((size_t)f.tellg());
      f.seekg(0, std::ios_base::beg);
      f.read((char*)bytes.data(), bytes.size());
      quickfbx::fbx_file p((char*)bytes.data(), (char*)bytes.data() + f.gcount());
    }
}

inline void fbx_file::init(const char *begin, const char *end) {
    begin_ = begin;
    end_ = end;

    if (end < begin + 27+4 || memcmp(begin, "Kaydara FBX Binary  ", 20)) {
        bad_fbx();
    }

    const char *p = begin + 23;
    std::string text;

    std::vector <const char *> ends;
    std::uint32_t version = u4(p);
    p += 4;

    while (u4(p)) {
        p = begin + u4(p);
    }
    end_offset = p - begin;

    // http://wiki.blender.org/index.php/User:Mont29/Foundation/FBX_File_Structure
    for (auto p : *this) {
        dump(p, 0);
    }
}

inline void fbx_file::dump(node &n, int depth) {
    printf("%*s%08zx..%08zx %s\n", depth*2, "", n.offset(), n.end_offset(), n.name().c_str());
    for (auto p : n.props()) {
        printf("%*s   %c %s\n", depth*2, "", p.kind(), ((std::string)p).c_str());
    }
    for (auto child : n) {
        dump(child, depth+1);
    }
}

inline void fbx_file::parse_objects(node &p) {
    for (auto o : p) {
        std::cout << "  " << o.name() << "\n";
        if (o.name() == "NodeAttribute") {
            for (auto a : o) {
                std::cout << "    " << a.name() << "\n";
                for (auto p : a.props()) {
                    std::cout << "      p " << (std::string)p << "\n";
                }
            }
        } else if (o.name() == "Geometry") {
            for (auto a : o) {
                std::cout << "    " << a.name() << "\n";
                for (auto p : a.props()) {
                    std::cout << "      p " << (std::string)p << "\n";
                }
            }
        } else if (o.name() == "Model") {
            for (auto a : o) {
                std::cout << "    " << a.name() << "\n";
                for (auto p : a.props()) {
                    std::cout << "      p " << (std::string)p << "\n";
                }
            }
        } else if (o.name() == "Material") {
            for (auto a : o) {
                std::cout << "    " << a.name() << "\n";
                for (auto p : a.props()) {
                    std::cout << "      p " << (std::string)p << "\n";
                }
            }
        }
    }
}

inline prop &prop::operator++() {
    const char *p = begin_ + offset;
    size_t al, enc, cl;
    switch(*p++) {
        case 'Y': p += 2; break;
        case 'C': p += 1; break;
        case 'I': p += 4; break;
        case 'F': p += 4; break;
        case 'D': p += 8; break;
        case 'L': p += 8; break;
        case 'f': al = u4(p); enc = u4(p+4); cl = u4(p+8); p += 12; p += !enc ? al * 4 : cl; break;
        case 'd': al = u4(p); enc = u4(p+4); cl = u4(p+8); p += 12; p += !enc ? al * 8 : cl; break;
        case 'l': al = u4(p); enc = u4(p+4); cl = u4(p+8); p += 12; p += !enc ? al * 8 : cl; break;
        case 'i': al = u4(p); enc = u4(p+4); cl = u4(p+8); p += 12; p += !enc ? al * 4 : cl; break;
        case 'b': al = u4(p); enc = u4(p+4); cl = u4(p+8); p += 12; p += !enc ? al * 1 : cl; break;
        case 'S': al = u4(p); p += 4 + al; break;
        case 'R': al = u4(p); p += 4 + al; break;
        default: throw std::runtime_error("bad fbx property"); break;
    }
    offset = p - begin_;
    return *this;
}

inline prop::operator std::string() {
    const char *p = begin_ + offset;
    size_t al;
    static char tmp[65536];
    int fv; std::uint64_t dv;
    switch(*p++) {
        case 'Y': sprintf(tmp, "%d", (short)u2(p)); break;
        case 'C': sprintf(tmp, *p ? "true" : "false"); break;
        case 'I': sprintf(tmp, "%d", (std::int32_t)u4(p)); break;
        case 'F': fv = u4(p); sprintf(tmp, "%8f", (float&)(fv)); break;
        case 'D': dv = u8(p); sprintf(tmp, "%10f", (double&)(dv)); break;
        case 'L': sprintf(tmp, "%lld", (std::int64_t)u8(p)); break;
        case 'f': return "<array>"; break;
        case 'd': return "<array>"; break;
        case 'l': return "<array>"; break;
        case 'i': return "<array>"; break;
        case 'b': return "<array>"; break;
        case 'S': al = u4(p); return std::string(p+4, p + 4 + al);
        case 'R': al = u4(p); return "<raw>";
        default: throw std::runtime_error("bad fbx property"); break;
    }
    return tmp;
}

inline prop props::begin() const { return quickfbx::prop(begin_, offset + 13 + len()); }
inline prop props::end() const { return quickfbx::prop(begin_, offset + 13 + property_list_len() + len()); }

}

#endif


