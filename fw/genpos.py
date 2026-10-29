#!/usr/bin/env python3

# +x    make x units wide gap
# =w    next key is w units wide
# ^y    next key is shifted upwards by y units

rows = """
   +1.25       ESC  +1        F1 F2 F3 F4 +0.5 F5 F6 F7 F8     +0.5 F9         F10                        F11 F12  +0.25 PrtSc ScrLk Pause +4.25
M5 +0.25       Grave 1         2  3  4  5     6  7  8   9     0     Minus      Equals                              +2.25 Ins   Home  PgUp  +0.25  Num KPDiv KPMul KPMin
M4 +0.25 =1.5  Tab    Q         W  E  R  T     Y  U  I   O     P      LBracket   RBracket +-0.5 =2    ^1 Backspace +2.25 Del   End   PgDn  +0.25  KP7 KP8   KP9   KPAdd
M3 +0.25 =1.75 Caps    A         S  D  F  G     H  J  K   L     Semi   Apos       Pound         =1.25 ^0.5   Enter +4.75                          KP4 KP5   KP6   +1
M2 +0.25 =1.25 LShift Backslash Z  X  C  V     B  N  M Comma Period Slash                            =2.75 RShift  +1.25       Up          +1.25  KP1 KP2   KP3   KPEnter
M1 +0.25 =1.25 LCtrl =1.25 LGUI =1.25 LAlt =6.25 Space  =1.25 RAlt =1.25 Fn                 =1.25 Menu =1.25 RCtrl +0.25 Left  Down  Right +0.25 =2 KP0       KPComma +1
""".strip()

rowpos = [ [] for _ in range(len(rows.splitlines())) ]

for i, row in enumerate(rows.splitlines()):
    xoff = 0
    yoff = 0
    width = 1

    for part in row.split(' '):
        if len(part) == 0:
            continue

        if part.startswith('+'):
            xoff += float(part[1:])
        elif part.startswith('^'):
            yoff = -float(part[1:])
        elif part.startswith('='):
            width = float(part[1:])
        else:
            rowpos[i].append((xoff + width / 2, yoff))
            if yoff == 0:
                xoff += width
            width = 1
            yoff = 0

    print(f'End at {xoff}')

for i, pos in enumerate(zip(rowpos, [ 0 ] + [ 1.5 + i for i in range(len(rows) - 1) ])):
    print(f'static const pos_t POS{i}[] = {{')

    for keypos in pos[0]:
        print(f'    {{ {keypos[0]}, {pos[1] + keypos[1]} }},')

    print('};')
