# ScraperPkg
A rewrite of the scraper_efi application in C.

## Wait, you re-wrote a *Rust* program in *C*? *Heresy!!* (Why?)

As it would turn out, LLVM does not have a COFF backend for RISC-V. This makes it incredibly difficult to compile UEFI applications written in Rust for RISC-V. As it would also turn out, trying to compile the UEFI application using an ELF target caused all of the code to get optimized out, lending `objcopy` totally useless.

## How to build and use

Clone the [EDK2 repository](https://github.com/tianocore/edk2), build the base tools, and source the `edksetup.sh` script. After that, clone this repository in the `edk2` source repository, and execute the following command to build with Clang...

`build -p ScraperPkg/ScraperPkg.dsc -a RISCV64 -t CLANGDWARF -b RELEASE`

## DISCLAIMER: I wrote this application with the help of Aider Chat Bot and Anthropic Claude 3.5 Sonnet. Why?

EDKII TianoCore is not very well documented. So, I ran Aider in the root of the EDKII repository in order to have it scan the repository and tell me what I needed to do in order to (1) find the largest contiguous chunk of conventional memory and (2) how to open a file on the current filesystem the UEFI application is running off of. I used Aider because documentation was scarce. I still wrote some of the code myself and was responsible for integrating the two components, as well as implementing the opening of multiple files, directly accessing memory, dumping memory to the correct files, handling file flushes, managing dependencies in the packages configuration files where Aider had missed them, etc.

## Why is Aider not in the commit logs?

Aider is not in the commit logs because I ran it from the root of the git repository for TianoCore in order to have it provide me with instructions on how to execute certain tasks. If it were to generate code itself, the corresponding commit history would have been commited to the EDKII TianoCore repository, not this one. So, Aider as it is currently designed would have not proven capable of handling this discrepancy itself.
