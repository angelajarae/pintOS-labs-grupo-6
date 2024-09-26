# ---- GROUP ----

**>> Fill in the names and email addresses of your group members.**

- Angela Jara <angela.jara@unmsm.edu.pe>  
- Alwin Dávila <alwin.davila@unmsm.edu.pe>  
- Shamir Mantilla <shamir.mantilla@unmsm.edu.pe>

# ---- PRELIMINARIES ----

**>> Please cite any offline or online sources you consulted while preparing your submission, other than the Pintos documentation, course text, lecture notes, and course staff.**

- [Pintos Documentation: https://www.scs.stanford.edu/24wi-cs212/pintos/pintos.html](https://www.scs.stanford.edu/24wi-cs212/pintos/pintos.html)  
- [Pintos Reference Manual: https://grail.eecs.csuohio.edu/~cis345s/PintosCSU_Ref.pdf](https://grail.eecs.csuohio.edu/~cis345s/PintosCSU_Ref.pdf)
- [Pintos Gitbook: https://pkuflyingpig.gitbook.io/pintos/project-description/lab1-threads/your-tasks](https://pkuflyingpig.gitbook.io/pintos/project-description/lab1-threads/your-tasks)

## ALARM CLOCK
===============

# ---- DATA STRUCTURES ----

**>> A1: Copy here the declaration of each new or changed `struct` or `struct` member, global or static variable, `typedef`, or enumeration. Identify the purpose of each in 25 words or less.**

```c
struct thread {
    int64_t wakeup_tick;
}
                //Almacena el valor del tick en el que un thread debe ser despertado.
static struct list sleep_list;
                //Contiene los threads dormidos que esperan ser despertados por wakeup_tick.
```
# ---- ALGORITHMS ----

**>> A2: Briefly describe what happens in a call to timer_sleep(), including the effects of the timer interrupt handler.**

- Desactiva las interrupciones para evitar race conditions.
- Calcula el wakeup_tick sumando los ticks actuales con el número de ticks que debe esperar el thread.
- Inserta el thread en la lista de threads dormidos (sleep_list), ordenada por wakeup_tick.
- Llama al scheduler para que otro thread se ejecute mientras el thread actual está bloqueado.
- En cada tick, el timer interrupt handler llama a wakeup_threads(), que revisa la sleep_list y despierta los threads cuyo wakeup_tick ha llegado.

# ---- SYNCHRONIZATION ----

**>> A3: What steps are taken to minimize the amount of time spent in the timer interrupt handler?**

- Se realizan solo las operaciones esenciales, como incrementar el contador de ticks y verificar si un thread debe despertarse.
- La sleep_list se mantiene ordenada por wakeup_tick, permitiendo una verificación rápida del thread que debe despertar.
- Los threads se despiertan en grupos si tienen el mismo wakeup_tick.

**>> A4: How are race conditions avoided when multiple threads call timer_sleep() simultaneously?**
- Las interrupciones se desactivan mientras se inserta el thread en sleep_list. Esto asegura que ningún otro thread ni el interrupt handler modifiquen la lista al mismo tiempo.

**>> A5: How are race conditions avoided when a timer interrupt occurs during a call to timer_sleep()?**
- Se deshabilitan las interrupciones al inicio de timer_sleep(). Esto previene que el interrupt handler interfiera en lo que se pone a dormir un thread.

===============================================================

# Pintos
This repo contains skeleton code for undergraduate Operating System course honor track at Peking University. 

[Pintos](http://pintos-os.org) is a teaching operating system for 32-bit x86, challenging but not overwhelming, small
but realistic enough to understand OS in depth (it can run on x86 machine and simulators 
including QEMU, Bochs and VMWare Player!). The main source code, documentation and assignments 
are developed by Ben Pfaff and others from Stanford (refer to its [LICENSE](./LICENSE)).

## Acknowledgement
This source code is adapted from professor ([Ryan Huang](huang@cs.jhu.edu)) at JHU, who also taught a similar undergraduate OS course. He made some changes to the original
Pintos labs (add lab0 and fix some bugs for MacOS). For students in PKU, please
download the release version skeleton code by `git clone git@github.com:PKU-OS/pintos.git`.
