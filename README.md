# JVM
C code is compiled directly into machine code as it is the job of a compiler like clang to take in text and output bytes. On the other hand, Java is interpreted. Instead of being run directly on the machine, a program called JVM (Java Virtual Machine) interprets the program.

This project implements a (simplified) JVM (called MiniJVM) which can handle all the Java bytecode integer instructions, and so is able to run Java programs that compute over the integers on MiniJVM.
