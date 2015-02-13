#!/bin/zsh

OUTFILE=$1

# Row output
echo "QUERY\tMAX\tMIN\tAVG\tSTD"

# Compute the maximum
MAX=$(awk '$1==0 {print $2}' "$OUTFILE" | sort -nr | head -1)
MIN=$(awk '$1==0 {print $2}' "$OUTFILE" | sort -n | head -1)
AVG=$(awk '$1==0 {print $2}' "$OUTFILE" | awk 'BEGIN {count=0} {count+=$0} END {print count/100}')
STD=$(awk '$1==0 {print $2}' "$OUTFILE" | awk -v AVG="$AVG" 'BEGIN {sum=0} {sum=($0-AVG)^2} END {print sqrt(sum/100)}')

# Row output
echo "0\t$MAX\t$MIN\t$AVG\t$STD"

# Compute the maximum
MAX=$(awk '$1==1 {print $2}' "$OUTFILE" | sort -nr | head -1)
MIN=$(awk '$1==1 {print $2}' "$OUTFILE" | sort -n | head -1)
AVG=$(awk '$1==1 {print $2}' "$OUTFILE" | awk 'BEGIN {count=0} {count+=$0} END {print count/100}')
STD=$(awk '$1==1 {print $2}' "$OUTFILE" | awk -v AVG="$AVG" 'BEGIN {sum=0} {sum=($0-AVG)^2} END {print sqrt(sum/100)}')

# Row output
echo "1\t$MAX\t$MIN\t$AVG\t$STD"

# Compute the maximum
MAX=$(awk '$1==2 {print $2}' "$OUTFILE" | sort -nr | head -1)
MIN=$(awk '$1==2 {print $2}' "$OUTFILE" | sort -n | head -1)
AVG=$(awk '$1==2 {print $2}' "$OUTFILE" | awk 'BEGIN {count=0} {count+=$0} END {print count/100}')
STD=$(awk '$1==2 {print $2}' "$OUTFILE" | awk -v AVG="$AVG" 'BEGIN {sum=0} {sum=($0-AVG)^2} END {print sqrt(sum/100)}')

# Row output
echo "2\t$MAX\t$MIN\t$AVG\t$STD"

# Compute the maximum
MAX=$(awk '$1==3 {print $2}' "$OUTFILE" | sort -nr | head -1)
MIN=$(awk '$1==3 {print $2}' "$OUTFILE" | sort -n | head -1)
AVG=$(awk '$1==3 {print $2}' "$OUTFILE" | awk 'BEGIN {count=0} {count+=$0} END {print count/100}')
STD=$(awk '$1==3 {print $2}' "$OUTFILE" | awk -v AVG="$AVG" 'BEGIN {sum=0} {sum=($0-AVG)^2} END {print sqrt(sum/100)}')

# Row output
echo "3\t$MAX\t$MIN\t$AVG\t$STD"

# Compute the maximum
MAX=$(awk '$1==4 {print $2}' "$OUTFILE" | sort -nr | head -1)
MIN=$(awk '$1==4 {print $2}' "$OUTFILE" | sort -n | head -1)
AVG=$(awk '$1==4 {print $2}' "$OUTFILE" | awk 'BEGIN {count=0} {count+=$0} END {print count/100}')
STD=$(awk '$1==4 {print $2}' "$OUTFILE" | awk -v AVG="$AVG" 'BEGIN {sum=0} {sum=($0-AVG)^2} END {print sqrt(sum/100)}')

# Row output
echo "4\t$MAX\t$MIN\t$AVG\t$STD"
