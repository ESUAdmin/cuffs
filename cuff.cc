#include <cstring>
#include <cstdlib>
#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <unistd.h>

#include "cuff.h"

const char Hex[] = "0123456789abcdef";
std::vector<unsigned char> buffer;
std::map<unsigned long, int> labels;
std::vector<std::pair<int, unsigned long>> writebacks;

int unhex(char c) {
	if ('0' <= c && c <= '9') return c - '0';
	else if ('A' <= c && c <= 'F') return c - 'A' + 10;
	else if ('a' <= c && c <= 'f') return c - 'a' + 10;
	else return -1;
}

void emit(int byte) {
	buffer.push_back(unsigned(byte));
}

void verbatim(const unsigned char *bytes, int sz) {
	for (int i = 0; i < sz; ++i)
		buffer.push_back(bytes[i]);
}

void verbatim(const char *hex) {
	int flag = 0, value = 0;
	for (; *hex; ++hex) {
		int u = unhex(*hex);
		if (u >= 0) {
			value = (value << 4) + u;
			flag = !flag;
		}
		if (flag == 0) buffer.push_back(value & 0xff);
	}
}

void verbatim(std::initializer_list<unsigned char> l) {
	buffer.insert(buffer.end(), l);
}

void push(unsigned long qword) {
	// clzll is undefined on 0
	if (!qword) { PUSH0; return; }
	int bitl = 64 - __builtin_clzll(qword);
	int bytel = bitl / 8 + (bitl & 7 ? 1 : 0);
	emit(0x5f + bytel);
	for (--bytel; bytel >= 0; --bytel) {
		emit((qword >> 8*bytel) & 0xFF);
	}
}

void push(const char *hex) {
	int l = std::strlen(hex),
	    flag = l % 2, value = 0;
	l = (l+1) / 2;
	if (l > 32) return;
	emit(0x5f + l);

	for (; *hex; ++hex) {
		value = (value << 4) + unhex(* hex);
		flag = !flag;
		if (flag == 0) buffer.push_back(value & 0xff);
	}
}

void assemble(bool print=true) {
	for (auto &wb : writebacks) {
		auto found = labels.find(wb.second);
		if (found == labels.end()) {
			std::cerr << "[WRITEBACK] cannot find label named " << wb.second << "\n";
			std::exit(1);
		} else {
			auto bytes2 = found->second;
			auto loc = wb.first;
			if (loc + 1 < buffer.size()) {
				buffer[loc] = bytes2 >> 8;
				buffer[loc+1] = bytes2 & 0xFF;
			} else {
				std::cerr << "[WRITEBACK] cannot write to offset " << loc
					<< " where size is " << buffer.size() << "\n";
				std::exit(1);
			}
		}
	}

	if (print) {
		for (unsigned char byte : buffer) {
			std::cout << Hex[byte>>4] << Hex[byte&15];
		}
	}
}

void label(unsigned long name) {
	labels[name] = buffer.size();
}
void jumpdest(unsigned long name) {
	label(name);
	JUMPDEST;
}
void ref(unsigned long name) {
	writebacks.push_back(std::make_pair(buffer.size(), name));
	emit(0xca); emit(0xfe);
}
void pushRef(unsigned long name) {
	PUSH2; ref(name);
}

void operator""_at(const char *name_str, unsigned long) {
	label(std::hash<std::string>{}(name_str));
}
void operator""_begin(const char *name_str, unsigned long) {
	jumpdest(std::hash<std::string>{}(name_str));
}
void operator""_v(const char *name_str, unsigned long) {
	ref(std::hash<std::string>{}(name_str));
}
void operator""_p(const char *name_str, unsigned long) {
	pushRef(std::hash<std::string>{}(name_str));
}

char *ffi(const char *exe, char * const argv[], int sz) {
	int rw[2];
	pipe(rw);
	auto pid = fork();
	if (pid) {
		char *buf = (char *)malloc(sz);
		close(rw[1]);
		read(rw[0], buf, sz);
		close(rw[0]);
		return buf;
	}
	else {
		close(rw[0]);
		dup2(rw[1], 1);
		std::exit(execvp(exe, argv));
	}
}

void func_sig(const char *func, bool push) {
	char *const argv[] = {
		strdup("cast"),
		strdup("sig"),
		strdup(func),
		nullptr
	};
	char *buf = ffi("cast", argv, 32);

	// 0x7830: little endian '0x'
	if (* reinterpret_cast<unsigned short *>(buf) == 0x7830) {
		buf[2+8] = 0;
		if (push) PUSH4;
		verbatim(&buf[2]);
	}
	else {
		std::cerr << "[FUNC_SIG] failed for " << func << "\n" << buf;
		std::exit(1);
	}
	free(buf);
	free(argv[0]);
	free(argv[1]);
	free(argv[2]);
}

void event_sig(const char *estr, bool push) {
	char *const argv[] = {
		strdup("cast"),
		strdup("sig-event"),
		strdup(estr),
		nullptr
	};
	char *buf = ffi("cast", argv, 80);

	if (* reinterpret_cast<unsigned short *>(buf) == 0x7830) {
		buf[2+64] = 0;
		if (push) PUSH32;
		verbatim(&buf[2]);
	}
	else {
		std::cerr << "[EVENT_SIG] failed for " << estr << "\n" << buf;
		std::exit(1);
	}
	free(buf);
	free(argv[0]);
	free(argv[1]);
	free(argv[2]);
}

void RUNTIME() {
	MAIN();
	assemble();
}

#define CODE_START (0xba5ed1488)
void CREATION() {
	MAIN();
	assemble(false);
	auto rt_code = buffer;

	buffer.clear(); labels.clear(); writebacks.clear();
	INIT();
	push(rt_code.size());
	pushRef(CODE_START);
	PUSH0; CODECOPY;
	push(rt_code.size());
	PUSH0; RETURN;
	label(CODE_START);
	assemble(true);

	labels.clear(); writebacks.clear(); buffer = rt_code;
	assemble(true);
}
#undef CODE_START

int main(int argc, const char *argv[]) {
	char type;
	if (argc == 1) type = 'c';
	else if (argc == 2) {
		type = argv[1][0];
	}
	else {
		std::cerr << "Must specify [c]reation or [r]untime\n";
		std::exit(1);
	}

	if (type == 'r') RUNTIME();
	else CREATION();
}
