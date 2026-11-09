#!/bin/bash
# usage: check.sh <compiler> <asmc> <input.in> [x y z]...
MC=$1; AS=$2; IN=$3; shift 3; T=$(mktemp -d)
VALS=("$@"); [ ${#VALS[@]} -eq 0 ] && VALS=(2 3 5 7 -3 11 -9 4 -2)
asm=$("$MC" < "$IN" 2>/dev/null)
if grep -q "Compile Error!" <<<"$asm"; then
  printf '#include <stdio.h>\nint main(){int x=0,y=0,z=0;\n%s\n;return 0;}\n' "$(cat "$IN")" > $T/r.c
  if gcc -w -c $T/r.c -o $T/r.o 2>/dev/null; then echo "FAIL $IN: you print Compile Error but gcc accepts it"; else echo "ok   $IN (Compile Error, gcc agrees)"; fi
  exit; fi
for ((i=0;i<${#VALS[@]};i+=3)); do x=${VALS[i]} y=${VALS[i+1]} z=${VALS[i+2]}
  printf '#include <stdio.h>\nint main(){int x=%s,y=%s,z=%s;\n%s\nprintf("x, y, z = %%d, %%d, %%d\\n",x,y,z);}\n' $x $y $z "$(cat "$IN")" > $T/r.c
  gcc -w $T/r.c -o $T/r 2>/dev/null || { echo "FAIL $IN: gcc rejects it but you don't print Compile Error"; exit; }
  want=$($T/r 2>/dev/null); [ -z "$want" ] && { echo "skip $IN ($x,$y,$z): real C crashed (e.g. divide by zero) - pick other values"; continue; }; out=$(echo "$asm" | "$AS" $x $y $z); got=$(head -1 <<<"$out"); cyc=$(tail -1 <<<"$out")
  if [ "$want" == "$got" ]; then echo "ok   $IN ($x,$y,$z) $cyc"; else echo "FAIL $IN ($x,$y,$z): want [$want] got [$got]"; fi
done