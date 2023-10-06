// SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.13;

import {Test, console2} from "../lib/forge-std/src/Test.sol";

struct Transfer {
	address to;
	uint256 value;
}

interface IBatch {
	function batchTransfer(Transfer[] calldata xfs) external payable;
	function batchTransferERC20(Transfer[] calldata xfs) external;
}

interface IERC20 {
    function name() external view returns (string memory);
    function symbol() external view returns (string memory);
    function decimals() external view returns (uint8);
    function totalSupply() external view returns (uint);
    function balanceOf(address owner) external view returns (uint);
    function allowance(address owner, address spender) external view returns (uint);

//    function approve(address spender, uint value) external returns (bool);
//  USDT is not ERC20
//  it does not return bool
    function approve(address spender, uint value) external;
    function transfer(address to, uint value) external returns (bool);
    function transferFrom(address from, address to, uint value) external returns (bool);
}

contract BatchTest is Test {
	function compile() internal returns (bytes memory) {
		string[] memory args = new string[](3);
		args[0] = "sh";
		args[1] = "-c";
		args[2] = "bash batch.cc";
		return vm.ffi(args);
	}

	function deploy() internal returns (address) {
		bytes memory code = compile();
		address a;
		assembly {
			a := create(0, add(0x20, code), mload(code))
		}
		console2.log("Address:", a);
		console2.log("Size:", a.code.length);
		return a;
	}

	function testBatch() external {
		IBatch batch = IBatch(deploy());
		Transfer[] memory X = new Transfer[](10);
		uint256 t = 0;
		for (uint i = 0; i < 10; ++i) {
			X[i].to = vm.addr(114514 + i);
			X[i].value = 100 + i;
			t += 100 + i;
		}
		vm.deal(address(this), t);
		batch.batchTransfer{value: t}(X);

		for (uint i = 0; i < 10; ++i) {
			assertEq(vm.addr(114514 + i).balance, 100 + i);
		}
	}

	function testBatchERC20() external {
		// must support PUSH0
		vm.createSelectFork("https://cloudflare-eth.com");
		IERC20 USDT = IERC20(0xdAC17F958D2ee523a2206206994597C13D831ec7);
		address rich = 0xF977814e90dA44bFA03b6295A0616a897441aceC;
		/* change to USDC
		USDT = IERC20(0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48);
		rich = 0x47ac0Fb4F2D84898e4D9E7b4DaB3C24507a6D503;
		*/

		IBatch batch = IBatch(deploy());
		Transfer[] memory X = new Transfer[](11);
		uint256 t = 0;
		for (uint i = 1; i < 11; ++ i) {
			X[i].to = vm.addr(1919 + i);
			X[i].value = 114514 + i;
			t += 114514 + i;
		}
		X[0].to = address(USDT);
		X[0].value = t;

		//vm.prank(rich);
		//USDT.approve(address(batch), 0);
		vm.prank(rich);
		USDT.approve(address(batch), ~uint256(0));

		vm.prank(rich);
		batch.batchTransferERC20(X);
		for (uint i = 1; i < 11; ++i) {
			assertEq(USDT.balanceOf(vm.addr(1919 + i)), 114514 + i);
		}
	}
}
