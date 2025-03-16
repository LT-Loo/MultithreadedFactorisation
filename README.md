## Introduction
This is a multithreaded client-server system designed for multiprocessing integer factorisation, where each thread processes a number derived from the input by bit rotation. This program leverages advanced multithreading, inter-process communication (IPC) and synchronisation to optimise performance and resource management.

## Program Structure
The program consists of a multithreaded server and a multithreaded client process. The communication between both processes is established using ***shared memory***, where they are able to access the data stored in it. However, due to large amount of running threads trying to access data in the shared memory, a ***handshaking protocol*** is implemented to ensure secure and seamless data transfer. Besides, ***pthread mutexes*** and condition variables are used to create ***semaphore*** to control access of the threads to the shared memory to prevent data override.

### Server System
The flow diagram below shows the work flow of the server system.
!(/ServerFlowDiagram.png)

### Client System
The flow diagram below shows the work flow of the client system.
!(/ClientFlowDiagram.png)

## Requirements
Have Linux/Cygwin environment installed in your machine.

## How To Run Program
1. Launch two terminals in Linux/Cygwin environment.
2. Compile source code via makefile by calling `make` command.
3. Execute `client.o` output file on one terminal and `server.o` on another.
4. User uses only the client program to communicate with the server.
5. Enter 32-bit integer when prompted to start the factorisation process.
6. While the factorisation is processing, user can enter a new input anytime. User will be informed if the input entered is accepted by the program.
7. User should read warning or error messages displayed carefully so that they can carry out the appropriate next action.
8. User can choose to leave the program anytime by using the `q` command.

## Developer
Loo<br>
loo.workspace@gmail.com