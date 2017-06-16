#!/bin/bash

function feq() {
  echo $1'=='$2 | bc -l
}

function fle() {
  echo $1'<='$2 | bc -l
}

exe="$1"
dir="$2/radiometric_response_calibration"

methods=("debevec" "engel")
for method in "${methods[@]}"; do
  crf=`$exe $dir --verbosity 0 --no-visualization --print --method $method -o /tmp/crf`
  while read -r line; do
    values=($(echo $line))
    if [[ ${#values[@]} != 256 ]]; then
      echo "CRF should have 256 values ($method)"
      exit 1
    fi
    if [[ $(feq ${values[0]} 0) == 0 ]]; then
      echo "CRF should map 0 to 0 ($method)"
      exit 1
    fi
    if [[ $(feq ${values[255]} 1) == 0 ]]; then
      echo "CRF should map 255 to 1 ($method)"
      exit 1
    fi
    for i in "${!values[@]-1}"; do
      if [[ $(fle ${values[i]} ${values[i+1]}) == 0 ]]; then
        echo "CRF should be nondecreasing ($method, element $i)"
        exit 1
      fi
    done
  done <<< "$crf"
done

exit 0
