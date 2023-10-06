#if 0
clang++ -o "/dev/shm/$0" cuff.cc "$0"
"/dev/shm/$0" "$*"
exit 0
#endif
#include "cuff.h"

void INIT() {}
void MAIN() {
	push(0x20); PUSH0; MSTORE;
	push(0x0c); push(0x20); MSTORE;
	PUSH12; verbatim((const unsigned char *)"Hello,World!", 12); push(8*(32-12)); SHL;
	push(0x40); MSTORE; push(0x60); PUSH0; RETURN;
}
