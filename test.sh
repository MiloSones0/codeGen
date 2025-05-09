#!/bin/bash

testPrograms=("Seven" "Fraction" "HelloWorld" "Square" "Average" \
              "ArrayTest" "MathTest" "List" "ConvertToBin" "Pong")

normalize() {
  sed 's/[[:space:]]*$//' "$1" | tr -d '\r'
}

total_files=0
matched_files=0

for prog in "${testPrograms[@]}"; do
  echo "Checking program: $prog"

  student_dir="$prog"
  reference_dir="${prog}_compiled"

  if [[ ! -d "$student_dir" ]]; then
    echo "  ❌ Student output directory missing: $student_dir"
    continue
  fi

  if [[ ! -d "$reference_dir" ]]; then
    echo "  ❌ Reference compiled directory missing: $reference_dir"
    continue
  fi

  for ref_file in "$reference_dir"/*.vm; do
    ((total_files++))
    file_name=$(basename "$ref_file")
    student_file="$student_dir/$file_name"

    if [[ ! -f "$student_file" ]]; then
      echo "  ❌ Missing student file: $file_name"
      continue
    fi

    if diff <(normalize "$ref_file") <(normalize "$student_file") > /dev/null; then
      echo "  ✅ Match: $file_name"
      ((matched_files++))
    else
      echo "  ❌ Mismatch: $file_name"
    fi
  done

  echo ""
done

echo "=========================================="
echo "✔️  Final Score: $matched_files / $total_files files matched"
echo "=========================================="
