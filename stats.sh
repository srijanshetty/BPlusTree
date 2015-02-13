#!/bin/zsh

OUTFILE=$1

# Compute the maximum
MAX=$(awk '$1==0' "$OUTFILE" | sort -nr | head -1)
MIN=$(awk '$1==0' "$OUTFILE" | sort -n | head -1)
AVG=$(awk '$1==0' "$OUTFILE" | awk 'BEGIN {count=0} {count+=$0} END {print count/100}')
STD=$(awk '$1==0' "$OUTFILE" | awk -v AVG="$AVG" 'BEGIN {sum=0} {sum=($0-AVG)^2} END {print sqrt(sum/100)}')

# Row output
echo "0 $MAX $MIN $AVG $STD"

# Compute the maximum
MAX=$(awk '$1==1' "$OUTFILE" | sort -nr | head -1)
MIN=$(awk '$1==1' "$OUTFILE" | sort -n | head -1)
AVG=$(awk '$1==1' "$OUTFILE" | awk 'BEGIN {count=0} {count+=$0} END {print count/100}')
STD=$(awk '$1==1' "$OUTFILE" | awk -v AVG="$AVG" 'BEGIN {sum=0} {sum=($0-AVG)^2} END {print sqrt(sum/100)}')

# Row output
echo "1 $MAX $MIN $AVG $STD"

# Compute the maximum
MAX=$(awk '$1==2' "$OUTFILE" | sort -nr | head -1)
MIN=$(awk '$1==2' "$OUTFILE" | sort -n | head -1)
AVG=$(awk '$1==2' "$OUTFILE" | awk 'BEGIN {count=0} {count+=$0} END {print count/100}')
STD=$(awk '$1==2' "$OUTFILE" | awk -v AVG="$AVG" 'BEGIN {sum=0} {sum=($0-AVG)^2} END {print sqrt(sum/100)}')

# Row output
echo "2 $MAX $MIN $AVG $STD"

# Compute the maximum
MAX=$(awk '$1==3' "$OUTFILE" | sort -nr | head -1)
MIN=$(awk '$1==3' "$OUTFILE" | sort -n | head -1)
AVG=$(awk '$1==3' "$OUTFILE" | awk 'BEGIN {count=0} {count+=$0} END {print count/100}')
STD=$(awk '$1==3' "$OUTFILE" | awk -v AVG="$AVG" 'BEGIN {sum=0} {sum=($0-AVG)^2} END {print sqrt(sum/100)}')

# Row output
echo "3 $MAX $MIN $AVG $STD"

# Compute the maximum
MAX=$(awk '$1==4' "$OUTFILE" | sort -nr | head -1)
MIN=$(awk '$1==4' "$OUTFILE" | sort -n | head -1)
AVG=$(awk '$1==4' "$OUTFILE" | awk 'BEGIN {count=0} {count+=$0} END {print count/100}')
STD=$(awk '$1==4' "$OUTFILE" | awk -v AVG="$AVG" 'BEGIN {sum=0} {sum=($0-AVG)^2} END {print sqrt(sum/100)}')

# Row output
echo "4 $MAX $MIN $AVG $STD"
