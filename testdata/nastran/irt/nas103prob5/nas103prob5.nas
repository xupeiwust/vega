ID NAS103 WORKSHOP 5 SOLUTION
SOL 106
TIME 30
CEND
$
TITLE = SIMPLE TENSION
SUBTITLE = DISPLACEMENT CONTROL MPCs + 1 SPCD
$ $AUTO
ECHO=PUNCH
DISP=ALL
STRESS(PLOT)=ALL
GPSTRESS=ALL
FORCE=ALL
MPC=100
SUBCASE 100 $ UNIAXIAL TENSION
 NLPARM=10
 SPC=100
 LOAD=1000
BEGIN BULK
CHEXA   1       1       1       2       3       4       5       6        +00000P
++00000P7       8
GRID    1               0.      0.      0.              123456
GRID    2               1.      0.      0.              23456
GRID    3               1.      1.      0.              3456
GRID    4               0.      1.      0.              13456
GRID    5               0.      0.      -1.             12456
GRID    6               1.      0.      -1.             2456
GRID    7               1.      1.      -1.             456
GRID    8               0.      1.      -1.             1456
$2345678123456781234567812345678123456781234567812345678123456781234567812345678
$MATHP   1               1500.                                            +000001
$++000001        3       1                                                +000002
$++000002                                                                 +000003
$++000003                                                                 +000004
$++000004                                                                 +000005
$++000005                                                                 +000006
$++000006100     200             400
MATHP   1       1.52238 .25472  1777.1  0.0     0.0     0.00000          +000001
++000001        2       1                                                +000002
++000002-.001955

MPC     100     2       1       1.      7       1       -1.
MPC     100     3       1       1.      7       1       -1.
MPC     100     3       2       1.      7       2       -1.
MPC     100     4       2       1.      7       2       -1.
MPC     100     5       3       1.      7       3       -1.
MPC     100     6       1       1.      7       1       -1.
MPC     100     6       3       1.      7       3       -1.
MPC     100     8       2       1.      7       2       -1.
MPC     100     8       3       1.      7       3       -1.
NLPARM  10      48              AUTO    1                       YES
PARAM   LGDISP  1
PARAM   POST    -1
PLSOLID 1       1
SPC     100     7       1
SPCD    1000    7       1       6.
TABLES1 100                                                              +000008
++0000081.      0.      1.125   1.      1.25    2.      1.5     3.       +000009
++0000091.525   4.      1.875   5.      2.      6.      2.25    7.       +00000A
++00000A2.5     8.      3.      9.      3.625   10.     4.      12.5     +00000B
++00000B4.75    16.     5.25    20.     5.75    23.     6.      27.      +00000C
++00000C6.25    31.     6.5     34.     6.75    38.75   7.      42.5     +00000D
++00000DENDT
TABLES1 200                                                              +00000F
++00000F1.016   0.83    1.07    1.56    1.15    2.55    1.21    3.25     +00000G
++00000G1.335   4.31    1.44    5.28    1.66    6.65    1.93    7.88     +00000H
++00000H2.46    9.74    3.      12.64   3.4     14.61   3.77    17.33    +00000I
++00000I4.1     20.11   4.32    22.40   4.54    24.41   ENDT
TABLES1 400                                                              +00000K
++00000K1.01    0.24    1.1     1.12    1.2     1.92    1.31    2.87     +00000L
++00000L1.49    3.62    1.86    5.61    2.36    7.2     2.97    9.045    +00000M
++00000M3.45    10.8    3.93    12.45   4.4     14.23   4.714   15.826   +00000N
++00000N4.96    17.46   ENDT
ENDDATA
