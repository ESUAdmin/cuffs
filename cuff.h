#pragma once
#define HAS_PUSH0
#include "evm.h"
#include <initializer_list>

void emit(int);
void verbatim(const unsigned char *, int);
void verbatim(const char *);
void verbatim(std::initializer_list<unsigned char>);
void push(unsigned long);
void push(const char *);

void label(unsigned long);
void jumpdest(unsigned long);
void ref(unsigned long);
void pushRef(unsigned long);
void operator""_at(const char *, unsigned long);
void operator""_begin(const char *, unsigned long);
void operator""_v(const char *, unsigned long);
void operator""_p(const char *, unsigned long);

void func_sig(const char *, bool = true);
void event_sig(const char *, bool = true);

extern void MAIN();
extern void INIT();
