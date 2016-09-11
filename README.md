Implementation of Schorre's [META II](http://www.ibm-1401.info/Meta-II-schorre.pdf)
compiler writing system.

There are several things here:

 - A [META II compiler](META_II_compiler.c) used to bootstrap the system.
 - The [META II machine](META_II_machine.c).
 - Another [META II machine](META_II_machine_bt.c) that supports backtracking.
 - The [META II compiler](META_II.m2) written in its own language.
 - The [VALGOL I example compiler](VALGOL_I.m2) and its [virtual machine](VALGOL_I_machine.c).
