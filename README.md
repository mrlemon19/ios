# 2. project to Operating Systems (IOS) - H2O process synchronization
## Author: Jakub Lukas, xlukas18

# 1. Introduction
This project is a solution to the synchronization problem of the H2O process from the book The Little Book of Semaphores by  Allen B. Downey. The task is to create a program that will simulate the production of water molecules (H) and oxygen molecules (O) and their subsequent combination into water (H2O). The program must be able to handle the following conditions:
- The H2O molecule must be created from 2 hydrogen molecules and 1 oxygen molecule.
- The H2O molecule must be created in the correct order (H-H-O).
- The H2O molecule must be created as soon as possible.

# 2. Solution
The project is implemented in c language. The solution is based on the use of semaphores and shared memory. The program is divided into 3 parts:
- The first part is the main program, which is responsible for the creation of processes and the correct order of their execution.
- The second part is the process of creating hydrogen molecules (H).
- The third part is the process of creating oxygen molecules (O).

# 3. Compilation and execution
Project is provided with Makefile. To compile project use standard command "make". To clean any files created during compilation use "make clean". To run project run "./proj2 NO NH TI TB". Where NO is the number of oxigen molecules and NH is the number of hydrogen molecules. TI is The maximum time, in milliseconds, that an oxygen/hydrogen atom waits after its creation before it queue up to create molecules. TB is The minimum time, in milliseconds, needed to create a molecule.

# 4. Project structure
```
proj2
├── Makefile
├── README.md
├── proj2.c
```

# 5. Evaluation
The project was evaluated by the teacher of the subject. The project received a grade of 14/15.
