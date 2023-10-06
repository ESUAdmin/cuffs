# cuffs

rUST-free extensible EVM assembler. You do NOT have to learn *编译原理*

example: [token multisender](batch.cc)

## use

[hello.cc](hello.cc)

```
void INIT() {}
void MAIN() {
    push(0x20); PUSH0; MSTORE;
    push(0x0c); push(0x20); MSTORE;
    PUSH12; verbatim((const unsigned char *)"Hello,World!", 12); push(8*(32-12)); SHL;
    push(0x40); MSTORE; push(0x60); PUSH0; RETURN;
}
```

`bash YourCode.cc c` or without c prints contract creation code

`bash YourCode.cc r` prints runtime bytecode

## testing

```
forge install --no-git https://github.com/foundry-rs/forge-std
forge test
```

## license

wtfpl - do whatever you want
