#if 0
clang++ -o "/dev/shm/$0" cuff.cc "$0"
"/dev/shm/$0" "$*"
exit 0
#endif
#include "cuff.h"

#define fs_transferFrom "transferFrom(address,address,uint256)"
#define fs_transfer "transfer(address,uint256)"
#define fs_batchTransfer "batchTransfer((address,uint256)[])"
#define fs_batchTransferERC20 "batchTransferERC20((address,uint256)[])"

#define ZERO PUSH0
#define RETURN_DATA_MEM 0x04

void INIT() {}

void CHECK_CALL_SUCCESS() {
	ISZERO; "global_revert"_p; JUMPI;
	push(0x1f); RETURNDATASIZE; GT;
	push(RETURN_DATA_MEM); MLOAD; ISZERO;
	AND; "global_revert"_p; JUMPI;
}

void MAIN() {
	ZERO; CALLDATALOAD; push(256-32); SHR;
	DUP1; func_sig(fs_batchTransfer); EQ; "transfer_ether"_p; JUMPI;
	func_sig(fs_batchTransferERC20); EQ; "transfer_erc20"_p; JUMPI;

	"global_revert"_begin;
	ZERO; ZERO; REVERT;

	"transfer_ether"_begin;
	push(0x44); // pointer
	"cont0"_begin;
	DUP1; CALLDATALOAD; // address[i] pointer
	DUP1; ISZERO; "stop"_p; JUMPI;
#ifdef PUSH0
	ZERO; ZERO; ZERO; ZERO; // 0 0 0 0 address[i] pointer
#else
	ZERO; DUP1; DUP1; DUP1;
#endif
	SWAP4; // address[i] 0 0 0 0 pointer
	DUP6; push(0x20); ADD; CALLDATALOAD; SWAP1;
	// address[i] value[i] 0 0 0 0 pointer
	GASLEFT; CALL; // success pointer
	ISZERO; "global_revert"_p; JUMPI;
	push(0x40); ADD; "cont0"_p; JUMP;

	"transfer_erc20"_begin;
	func_sig(fs_transferFrom); push(256-32); SHL; ZERO; MSTORE;
	CALLER; push(0x04); MSTORE;
	ADDRESS;push(0x24); MSTORE;
	push(0x64); CALLDATALOAD; push(0x44); MSTORE;
	push(0x44); CALLDATALOAD; // contract
	push(0x20); // retSz
	push(RETURN_DATA_MEM); // ret
	push(0x64); // argSz
	ZERO;       // arg
	ZERO;       // value
	DUP6;       // contract
	GASLEFT; CALL; // success contract
	CHECK_CALL_SUCCESS();
	// contract
	
	func_sig(fs_transfer); push(256-32); SHL; ZERO; MSTORE;
	push(0x84); // pointer contract
	"cont1"_begin;
	DUP1; CALLDATALOAD; // address[i] pointer contract
	ISZERO; "stop"_p; JUMPI; // pointer contract
	push(0x40); DUP2; push(0x04); // 4 pointer 0x40 pointer contract
	CALLDATACOPY; // pointer contract
	
	push(0x20); // retSz
	push(RETURN_DATA_MEM); // ret
	push(0x44); // argSz
	ZERO; // arg
	ZERO; // value
	DUP7; // contract
	GASLEFT; CALL; // success pointer contract
	CHECK_CALL_SUCCESS();
	// pointer contract
	push(0x40); ADD; "cont1"_p; JUMP;

	"stop"_begin; STOP;
}
