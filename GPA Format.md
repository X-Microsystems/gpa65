#General-Purpose ASCII (GPA) Record Format Summary
Format
[SECTIONS]
section_name  start..end  attribute

[FUNCTIONS]
func_name  start..end

[USER]
sym_name   value                    base
sym_name   start_value   [size]     base
sym_name   start_value..end_value   base

[VARIABLES]
var_name   start [size]
var_name   start..end

[SOURCE LINES]
File: file_name
line#  address

[START ADDRESS]
address

\#comment text
Lines without a preceding header are assumed to be symbol definitions in one of the [USER] formats.

Example
This is an example GPA file that contains several different kinds of records.

[SECTIONS]
prog     00001000..0000101F
data     40002000..40009FFF
common   FFFF0000..FFFF1000

[FUNCTIONS]
main     00001000..00001009
test     00001010..0000101F

[USER]
bdontcare    00x1     binary
hvalue       00EF     hex
drange       0..99    decimal
srange      -23  20   signed decimal   # The 20 is a decimal value for size.

[VARIABLES]
total    40002000  4
value    40008000  4

[SOURCE LINES]
File: main.c
10       00001000
11       00001002
14       0000100A
22       0000101E

File: test.c
 5       00001010
 7       00001012
11       0000101A

##Usage Notes (Agilent LPA 5.90)
###Sections
- When at least one section is defined, source lines are only correlated within defined section address ranges
- When no sections are defined, all source line addresses are correlated.

###Source Lines
There is quite a bit of odd behavior here that was difficult to figure out. These are the most important points:

- Source lines must be presented in order of address. It appears that each new source line assumes all higher addresses are associated to it. Then, loading a higher address afterward will do the same. Therefore, following a source line with one of a lower address will cause the first line to be effectively overwritten within the LPA software.
- When the last line of a file definition is directly followed by a new file definition with the first line having the same address (such as when a macro is called), for some reason, the later address won't take. However, if the last two addresses of the first file definition are overwritten by the next file's first two lines, this doesn't happen. It is unclear exactly what conditions lead to this behavior. The workaround in the case of gpa65 is to just comment out superseded addresses so the LPA software doesn't have to decide what to do. The addresses are left in the file to allow the user to see the chain of sources when multiple lines are associated with an address.
